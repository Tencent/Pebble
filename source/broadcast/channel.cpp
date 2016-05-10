/*
 * Tencent is pleased to support the open source community by making Pebble available.
 * Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
 * Licensed under the MIT License (the "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 *
 */


#include <sstream>
#include "source/broadcast/channel.h"
#include "source/broadcast/channel_conf_adapter.h"
#include "source/broadcast/idl/broadcast_PebbleBroadcastService_if.h"
#include "source/broadcast/isubscriber.h"
#include "source/common/string_utility.h"
#include "source/rpc/common/rpc_define.h"


namespace pebble {
namespace broadcast {

Channel::Channel(const std::string& name, size_t max_msg_queue_size)
    : m_name(name), m_queue_size(max_msg_queue_size) {
}

Channel::~Channel() {
    Close();
}

int Channel::Subscribe(const ISubscriber* subscriber) {
    if (NULL == subscriber) {
        return -1;
    }
    if (m_subscribers.find(subscriber->Id()) != m_subscribers.end()) {
        PLOG_ERROR("subscriber is existed(%ld).", subscriber->Id());
        return -2;
    }

    // 对同一会话的用户限制，不能重复加入频道
    ISubscriber* sub = const_cast<ISubscriber*>(subscriber);
    int64_t session_id = sub->SessionId();
    if (session_id != -1
        && m_sessioned_subscribers.find(session_id) != m_sessioned_subscribers.end()) {
        PLOG_ERROR("session %ld is already existed.", session_id);
        return -3;
    }

    m_subscribers[sub->Id()] = sub;

    if (session_id != -1) {
        m_sessioned_subscribers[session_id] = const_cast<ISubscriber*>(subscriber);
    }

    return 0;
}

int Channel::UnSubscribe(const ISubscriber* subscriber) {
    if (NULL == subscriber) {
        return -1;
    }

    int ret = 0;
    std::map<int64_t, ISubscriber*>::iterator it = m_subscribers.find(subscriber->Id());
    if (it != m_subscribers.end()) {
        m_subscribers.erase(it);
    } else {
        ret = -2;
        PLOG_ERROR("subscriber unexisted(%ld).", subscriber->Id());
    }

    int64_t session_id = const_cast<ISubscriber*>(subscriber)->SessionId();
    if (-1 == session_id) {
        return ret;
    }

    std::map<int64_t, ISubscriber*>::iterator sit = m_sessioned_subscribers.find(session_id);
    if (sit == m_sessioned_subscribers.end()) {
        PLOG_DEBUG("sessioned(%ld) subscriber unexisted.", session_id);
        return -3;
    }

    m_sessioned_subscribers.erase(sit);

    return ret;
}

int Channel::UpdateSubscriber(const ISubscriber* old_subscriber,
        const ISubscriber* new_subscriber) {
    if (NULL == old_subscriber || NULL == new_subscriber) {
        return -1;
    }

    int ret = UnSubscribe(old_subscriber);
    if (ret != 0) {
        return -2;
    }

    ret = Subscribe(new_subscriber);
    return (ret == 0 ? 0 : -3);
}

int Channel::Publish(const char* msg, size_t len, int encode_type, bool forward) {
    if (NULL == msg) {
        PLOG_ERROR("msg is NULL.");
        return -1;
    }

    if (0 == len) {
        PLOG_DEBUG("msg len = 0.");
        return 0;
    }

    if (m_messages.size() >= m_queue_size) {
        PLOG_ERROR("msg queue is full.");
        return -2;
    }

    m_messages.push_back(BroadcastMessage(msg, len, encode_type, forward)); // NOLINT

    return 0;
}

int Channel::Update() {
    // 改为时间片发送广播? + 每轮发送n个?
    int num = Broadcast();

    return num;
}

std::string Channel::GetName() {
    return m_name;
}

int Channel::Open() {
    // do nothing.
    return 0;
}

void Channel::Close() {
    m_subscribers.clear();
    m_messages.clear();
}

int Channel::Broadcast() {
    if (m_messages.empty()) {
        return 0;
    }

    int num = 0;
    int ret = 0;
    for (std::map<int64_t, ISubscriber*>::iterator it = m_subscribers.begin();
        it != m_subscribers.end(); ++it) {
        // 向订阅者推送消息
        ret = (it->second)->Push(m_messages.front().m_message, m_messages.front().m_encode_type);
        if (ret != 0) {
            PLOG_ERROR("%s send message failed(%d).", m_name.c_str(), ret);
        }

        num++;
    }

    m_messages.erase(m_messages.begin());

    return num;
}

std::string GlobalChannel::m_appid;
std::string GlobalChannel::m_server_addr;
std::map<std::string, PebbleBroadcastServiceClient*> GlobalChannel::m_other_servers;

GlobalChannel::GlobalChannel(const std::string& name,
    int max_msg_queue_size)
    : Channel(name, max_msg_queue_size) {
}

GlobalChannel::~GlobalChannel() {
    Close();
}

int GlobalChannel::Subscribe(const ISubscriber* subscriber) {
    if (NULL == subscriber) {
        return -1;
    }
    ISubscriber* sub = const_cast<ISubscriber*>(subscriber);
    // 如果手机侧client，需要注册到zk，不保存本地，否则重复
    if (0 == sub->IsDirectConnect()) {
        // 对同一会话的用户限制，不能重复加入频道
        int64_t session_id = sub->SessionId();
        if (session_id != -1
            && m_sessioned_subscribers.find(session_id) != m_sessioned_subscribers.end()) {
            PLOG_ERROR("session %ld is already existed.", session_id);
            return -2;
        }

        ChannelConfAdapter* conf = ChannelConfAdapter::Instance();
        if (!conf) { return -3; }

        // 非直连节点存储信息格式: name: pipe:addr, value: addr
        std::ostringstream name;
        name << "pipe:" << sub->PeerAddress();
        std::ostringstream value;
        value << sub->PeerAddress();

        int ret = conf->AddSubInstance(m_name, name.str(), value.str());
        if (ret != 0) {
            PLOG_ERROR("add %s to %s failed(%d).", name.str().c_str(), m_name.c_str(), ret);
            return -4;
        }

        if (session_id != -1) {
            m_sessioned_subscribers[session_id] = sub;
        }

        return 0;
    }

    return Channel::Subscribe(subscriber);
}

int GlobalChannel::UnSubscribe(const ISubscriber* subscriber) {
    if (NULL == subscriber) {
        return -1;
    }
    ISubscriber* sub = const_cast<ISubscriber*>(subscriber);
    if (0 == sub->IsDirectConnect()) {
        // 优先清理m_sessioned_subscribers表
        int64_t session_id = sub->SessionId();
        if (session_id != -1) {
            std::map<int64_t, ISubscriber*>::iterator sit =
                m_sessioned_subscribers.find(session_id);
            if (sit == m_sessioned_subscribers.end()) {
                PLOG_DEBUG("sessioned(%ld) subscriber unexisted.", session_id);
            } else {
                m_sessioned_subscribers.erase(sit);
            }
        }

        // 清理zk上注册的节点
        ChannelConfAdapter* conf = ChannelConfAdapter::Instance();
        if (!conf) { return -2; }

        std::ostringstream name;
        name << "pipe:" << sub->PeerAddress();

        int ret = conf->RmvSubInstance(m_name, name.str());
        if (ret != 0) {
            PLOG_ERROR("rmv %s from %s failed(%d).", name.str().c_str(), m_name.c_str(), ret);
            return -3;
        }

        return 0;
    }

    return Channel::UnSubscribe(sub);
}

int GlobalChannel::Open() {
    int ret = Channel::Open();
    if (ret != 0) {
        PLOG_ERROR("%s open failed(%d).", m_name.c_str(), ret);
        return -1;
    }

    // 全局频道需要保存数据
    ChannelConfAdapter* conf = ChannelConfAdapter::Instance();
    if (!conf) {
        return -2;
    }

    ret = conf->OpenChannel(m_name);
    if (ret != 0) {
        PLOG_ERROR("%s open channel failed(%d).", m_name.c_str(), ret);
        return -3;
    }

    // 地址未配置也可以创建，适用手机客户端场景
    if (m_server_addr.empty()) {
        return 0;
    }

    // server打开频道后自然就加入了频道
    ret = conf->AddSubInstance(m_name, m_appid, m_server_addr);
    if (ret != 0) {
        PLOG_ERROR("%s add self(%s) failed(%d).", m_name.c_str(), m_appid.c_str(), ret);
        return -4;
    }

    return 0;
}

void GlobalChannel::Close() {
    Channel::Close();

    // 删除存储数据
    ChannelConfAdapter* conf = ChannelConfAdapter::Instance();
    if (!conf) {
        return;
    }

    int ret = conf->RmvSubInstance(m_name, m_appid);
    if (ret != 0) {
        PLOG_ERROR("%s rmv self(%s) failed(%d).", m_name.c_str(), m_appid.c_str(), ret);
    }

    // 是否需要删除其他client节点?

    ret = conf->CloseChannel(m_name);
    if (ret != 0) {
        PLOG_ERROR("%s close channel failed(%d).", m_name.c_str(), ret);
    }
}

int GlobalChannel::Broadcast() {
    if (m_messages.empty()) {
        return 0;
    }

    int num = 0;
    int ret = 0;
    for (std::map<int64_t, ISubscriber*>::iterator it = m_subscribers.begin();
        it != m_subscribers.end(); ++it) {
        // 向订阅者推送消息
        ret = (it->second)->Push(m_messages.front().m_message, m_messages.front().m_encode_type);
        if (ret != 0) {
            PLOG_ERROR("%s send message failed(%d).", m_name.c_str(), ret);
        }

        num++;
    }

    // 自己产生的广播消息要通知到其他server
    if (m_messages.front().m_forward) {
        num += Notify(m_messages.front().m_message, m_messages.front().m_encode_type);
    }

    m_messages.erase(m_messages.begin());

    return num;
}

int GlobalChannel::Notify(const std::string& msg, int32_t encode_type) {
    int num = 0;

    // 抽象连接对象，直接向其他连接发送消息即可
    ChannelConfAdapter* conf = ChannelConfAdapter::Instance();
    if (!conf) {
        PLOG_ERROR("channel conf adapter is null.");
        return num;
    }

    std::map<std::string, std::string>* sub_instances = NULL;
    int ret = conf->GetSubInstances(m_name, &sub_instances);
    if (ret != 0) {
        PLOG_ERROR("get sub instance from zk failed(%d).", ret);
        return num;
    }

    for (std::map<std::string, std::string>::iterator it = sub_instances->begin();
        it != sub_instances->end(); ++it) {
        if (m_appid == it->first) {
            // 不发给自己
            continue;
        }
        if ((it->second).empty()) {
            continue;
        }
        SendTo(it->first, it->second, msg, m_name, encode_type);
        ++num;
    }

    return num;
}

int GlobalChannel::SendTo(const std::string& dst_name, const std::string& dst_addr,
    const std::string& msg, const std::string& channel_name, int32_t encode_type) {
    int ret = 0;

    if (StringUtility::StartsWith(dst_name, "pipe:")) {
        ret = -1;
    } else {
        ret = SendToServer(dst_addr, msg, channel_name, encode_type);
    }

    return ret;
}

int GlobalChannel::SendToServer(const std::string& addr, const std::string& msg,
    const std::string& channel_name, int32_t encode_type) {

    PebbleBroadcastServiceClient* broadcast_client = NULL;

    std::map<std::string, PebbleBroadcastServiceClient*>::iterator it =
        m_other_servers.find(addr);
    if (it != m_other_servers.end()) {
        broadcast_client = it->second;
    } else {
        broadcast_client =
            new PebbleBroadcastServiceClient(addr, pebble::rpc::PROTOCOL_BINARY);
        if (broadcast_client) { m_other_servers[addr] = broadcast_client; }
    }

    if (!broadcast_client) {
        return -1;
    }

    // 需要对m_other_servers资源维护
    return broadcast_client->Broadcast(channel_name, msg, encode_type); // oneway 请求
}


} // namespace broadcast
}  // namespace pebble

