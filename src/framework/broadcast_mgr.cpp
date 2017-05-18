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

#include "common/log.h"
#include "framework/broadcast_mgr.h"
#include "framework/broadcast_mgr.inh"
#include "framework/event_handler.inh"
#include "framework/message.h"
#include "framework/naming.h"


namespace pebble {

BroadcastMgr::BroadcastMgr() {
    m_channel_mgr = NULL;
    m_naming      = NULL;
    m_processor   = NULL;
    m_overload    = 0;
    m_app_id      = 0;
    m_path        = "_broadcast";
    m_relay_client  = NULL;
    m_event_handler = NULL;
}

BroadcastMgr::~BroadcastMgr() {
    cxx::unordered_map<std::string, std::vector<Address> >::iterator it =
        m_relay_connection_map.begin();
    for (; it != m_relay_connection_map.end(); ++it) {
        CloseRelayConnections(it->first);
    }

    delete m_channel_mgr;
}

int32_t BroadcastMgr::Init(int64_t app_id, const std::string& app_key, Naming* naming) {
    if (naming == NULL) {
        PLOG_ERROR("naming is null");
        return -1;
    }

    if (m_relay_address.empty()) {
        PLOG_ERROR("broadcast relay address not set");
        return -1;
    }

    if (m_channel_mgr) {
        PLOG_ERROR("already inited.");
        return -1;
    }

    m_naming = naming;

    // 广播只在一个app中生效，跨app使用proxy解决
    std::ostringstream oss;
    oss << app_id;
    int ret = m_naming->SetAppInfo(oss.str(), app_key);
    if (ret != 0) {
        PLOG_ERROR("zk set app info(%ld:%s) failed(%d)", app_id, app_key.c_str(), ret);
        m_naming = NULL;
        return -1;
    }
    
    m_channel_mgr = new ChannelMgr();
    m_app_id      = app_id;
    m_app_key     = app_key;

    return 0;
}

int32_t BroadcastMgr::Update(uint32_t overload) {
    m_overload = overload;
    return 0;
}

int64_t BroadcastMgr::BindRelayAddress(const std::string& url) {
    int64_t handle = Message::Bind(url);
    if (handle < 0) {
        PLOG_ERROR("bind %s failed(%d:%s)", url.c_str(), handle, Message::GetLastError());
        return -1;
    }

    m_relay_address = url;

    return handle;
}

int32_t BroadcastMgr::OpenChannel(const std::string& channel) {
    if (!m_naming) {
        PLOG_ERROR("open %s failed, naming not init", channel.c_str());
        return -1;
    }

    std::vector<std::string> urls;
    urls.push_back(m_relay_address);

    std::ostringstream oss;
    oss << m_app_id << "/" << m_path << "/" << channel;

    int32_t ret = m_naming->Register(oss.str(), urls);
    if (ret != 0) {
        PLOG_ERROR("register %s failed(%d)", oss.str().c_str(), ret);
        return -1;
    }

    ret = m_naming->WatchName(oss.str(), cxx::bind(&BroadcastMgr::OnChannelChanged, this,
        oss.str(), cxx::placeholders::_1));
    PLOG_IF_ERROR(ret, "watch %s failed(%d)", oss.str().c_str(), ret);

    ret = m_channel_mgr->OpenChannel(channel);
    if (ret != 0) {
        PLOG_ERROR("open %s failed(%d)", ret);
        return -1;
    }

    return 0;
}

int32_t BroadcastMgr::OpenChannelAsync(const std::string& channel, const CbHandleChannel& cb) {
    if (!m_naming) {
        PLOG_ERROR("open %s failed, naming not init", channel.c_str());
        return -1;
    }

    std::vector<std::string> urls;
    urls.push_back(m_relay_address);

    std::ostringstream oss;
    oss << m_app_id << "/" << m_path << "/" << channel;

    int32_t ret = m_naming->RegisterAsync(oss.str(), urls,
        cxx::bind(&BroadcastMgr::OnOperateReturn, this, cxx::placeholders::_1, "open", channel, cb));
    if (ret != 0) {
        PLOG_ERROR("register %s failed(%d)", oss.str().c_str(), ret);
        return -1;
    }

    ret = m_naming->WatchNameAsync(oss.str(),
        cxx::bind(&BroadcastMgr::OnChannelChanged, this, oss.str(), cxx::placeholders::_1),
        cxx::bind(&BroadcastMgr::OnWatchReturn, this, cxx::placeholders::_1, channel));
    PLOG_IF_ERROR(ret, "watch %s failed(%d)", oss.str().c_str(), ret);

    ret = m_channel_mgr->OpenChannel(channel);
    if (ret != 0) {
        PLOG_ERROR("open %s failed(%d)", ret);
        return -1;
    }

    return 0;
}

int32_t BroadcastMgr::CloseChannel(const std::string& channel) {
    if (!m_naming) {
        PLOG_ERROR("open %s failed, naming not init", channel.c_str());
        return -1;
    }

    std::ostringstream oss;
    oss << m_app_id << "/" << m_path << "/" << channel;

    int32_t ret = m_naming->UnRegister(oss.str());
    if (ret != 0) {
        PLOG_ERROR("unregister %s failed(%d)", oss.str().c_str(), ret);
        return -1;
    }

    ret = m_channel_mgr->CloseChannel(channel);
    if (ret != 0) {
        PLOG_ERROR("open %s failed(%d)", ret);
        return -1;
    }

    CloseRelayConnections(channel);

    return 0;
}

int32_t BroadcastMgr::CloseChannelAsync(const std::string& channel, const CbHandleChannel& cb) {
    if (!m_naming) {
        PLOG_ERROR("open %s failed, naming not init", channel.c_str());
        return -1;
    }

    std::ostringstream oss;
    oss << m_app_id << "/" << m_path << "/" << channel;

    int32_t ret = m_naming->UnRegisterAsync(oss.str(),
        cxx::bind(&BroadcastMgr::OnOperateReturn, this, cxx::placeholders::_1, "close", channel, cb));
    if (ret != 0) {
        PLOG_ERROR("unregister %s failed(%d)", oss.str().c_str(), ret);
        return -1;
    }

    ret = m_channel_mgr->CloseChannel(channel);
    if (ret != 0) {
        PLOG_ERROR("open %s failed(%d)", ret);
        return -1;
    }

    CloseRelayConnections(channel);

    return 0;
}

bool BroadcastMgr::ChannelExist(const std::string& channel) {
    if (m_channel_mgr) {
        return m_channel_mgr->ChannelExist(channel);
    }
    return false;
}

int32_t BroadcastMgr::JoinChannel(const std::string& channel, Subscriber subscriber) {
    if (!m_channel_mgr) {
        PLOG_ERROR("join %ld to %s failed, not init", subscriber, channel.c_str());
        return -1;
    }

    return m_channel_mgr->JoinChannel(channel, subscriber);
}

int32_t BroadcastMgr::QuitChannel(const std::string& channel, Subscriber subscriber) {
    if (!m_channel_mgr) {
        PLOG_ERROR("quit %ld to %s failed, not init", subscriber, channel.c_str());
        return -1;
    }

    return m_channel_mgr->QuitChannel(channel, subscriber);
}

int32_t BroadcastMgr::QuitChannel(Subscriber subscriber) {
    if (!m_channel_mgr) {
        PLOG_ERROR("quit %ld failed, not init", subscriber);
        return -1;
    }

    return m_channel_mgr->QuitChannel(subscriber);
}

int32_t BroadcastMgr::Send(const std::string& channel, const uint8_t* buff, uint32_t buff_len, bool relay) {
    const uint8_t* msg_frag[] = { buff };
    uint32_t msg_frag_len[]   = { buff_len };
    return SendV(channel, 1, msg_frag, msg_frag_len, relay);
}

int32_t BroadcastMgr::SendV(const std::string& channel, uint32_t msg_frag_num,
    const uint8_t* msg_frag[], uint32_t msg_frag_len[], bool relay) {
    const SubscriberList* subscribers = m_channel_mgr->GetSubscriberList(channel);
    if (NULL == subscribers) {
        PLOG_ERROR("channel %s not exist", channel.c_str());
        return kCHANNEL_NOT_EXIST;
    }

    if (m_overload != 0) {
        return kRPC_SYSTEM_OVERLOAD_BASE - m_overload;
    }

    int32_t ret = 0;
    int32_t num = 0;
    
    cxx::unordered_map<Subscriber, int64_t>::const_iterator it = subscribers->begin();
    for (; it != subscribers->end(); ++it) {
        ret = Message::SendV(it->first, msg_frag_num, msg_frag, msg_frag_len);
        if (0 == ret) {
            ++num;
        }
        m_event_handler->RequestProcComplete(channel, ret, 0);
    }

    if (relay) {
        num += RelayV(channel, msg_frag_num, msg_frag, msg_frag_len);
    }

    return num;
}

int32_t BroadcastMgr::RelayV(const std::string& channel, uint32_t msg_frag_num,
        const uint8_t* msg_frag[], uint32_t msg_frag_len[]) {

    cxx::unordered_map<std::string, std::vector<Address> >::iterator it =
        m_relay_connection_map.find(channel);
    if (m_relay_connection_map.end() == it) {
        PLOG_ERROR("channel %s not in relay connection map", channel.c_str());
        return 0;
    }

    std::string message;
    for (uint32_t i = 0; i < msg_frag_num; i++) {
        message.append((const char*)(msg_frag[i]), msg_frag_len[i]);
    }

    int32_t ret = 0;
    int32_t num = 0;
    int64_t handle = -1;
    for (std::vector<Address>::iterator ait = it->second.begin(); ait != it->second.end(); ++ait) {
        if (ait->_handle < 0) {
            handle = Message::Connect(ait->_url);
            if (handle < 0) {
                PLOG_ERROR("connect %s failed(%d:%s)", ait->_url.c_str(), handle, Message::GetLastError());
                continue;
            }
            ait->_handle = handle;
        }

        m_relay_client->SetHandle(ait->_handle);
        ret = m_relay_client->OnRelay(channel, message);
        if (0 == ret) {
            ++num;
        }
        m_event_handler->RequestProcComplete(channel, ret, 0);
    }

    return num;
}

void BroadcastMgr::CloseRelayConnections(const std::string& channel) {
    cxx::unordered_map<std::string, std::vector<Address> >::iterator it =
        m_relay_connection_map.find(channel);
    if (m_relay_connection_map.end() == it) {
        PLOG_ERROR("channel %s not in relay connection map", channel.c_str());
        return;
    }

    int32_t ret = 0;
    for (std::vector<Address>::iterator ait = it->second.begin(); ait != it->second.end(); ++ait) {
        if (ait->_handle >= 0) {
            ret = Message::Close(ait->_handle);
            PLOG_IF_ERROR(ret, "close %ld failed(%d:%s)", ait->_handle, ret, Message::GetLastError());
        }
    }
    m_relay_connection_map.erase(it);
}

void BroadcastMgr::OnChannelChanged(const std::string& path,
                                const std::vector<std::string>& urls) {
    PLOG_INFO("channel changed path:%s urls size:%d", path.c_str(), urls.size());
    std::string channel = path.substr(path.find_last_of("/") + 1);
    std::vector<Address>& address_array = m_relay_connection_map[channel];

    int32_t ret = 0;

    // channel下无实例
    if (urls.empty()) {
        std::vector<Address>::iterator ait = address_array.begin();
        for (; ait != address_array.end(); ++ait) {
            if (ait->_handle >= 0) {
                ret = Message::Close(ait->_handle);
                PLOG_IF_ERROR(ret, "close %ld failed(%d:%s)", ait->_handle, ret, Message::GetLastError());
            }
        }
        address_array.clear();
        return;
    }

    // channel有实例
    int64_t handle = -1;
    std::vector<Address> new_address;

    std::vector<std::string>::const_iterator it = urls.begin();
    for (; it != urls.end(); ++it) {
        // 剔除自己
        if (*it == m_relay_address) {
            continue;
        }

        // 如果已经存在，记录handle(连接信息)
        handle = -1;
        std::vector<Address>::iterator ait = address_array.begin();
        for (; ait != address_array.end(); ++ait) {
            if (*it == ait->_url) {
                handle = ait->_handle;
                break;
            }
        }

        new_address.push_back(Address(*it, handle));
    }

    // 替换
    address_array = new_address;
}

void BroadcastMgr::OnOperateReturn(int32_t ret, const std::string& operate,
        const std::string& channel, const CbHandleChannel& cb) {
    PLOG_IF_ERROR(ret, "%s %s failed(%d)", operate.c_str(), channel.c_str(), ret);
    cb(ret, channel);
}

void BroadcastMgr::OnWatchReturn(int32_t ret, const std::string& channel) {
    PLOG_IF_ERROR(ret, "watch %s failed(%d)", channel.c_str(), ret);
}

void BroadcastRelayHandler::OnRelay(const std::string& channel, const std::string& message) {
    // 频道不存在，不处理，理论上不应该出现
    if (!m_broadcast_mgr->ChannelExist(channel)) {
        PLOG_ERROR("receive unexpect broadcast msg, channel=%s", channel.c_str());
        return;
    }

    // 分发给自己的广播消息处理程序
    // TODO: 目前默认支持固定协议，有接口(Attach)扩展
    // 过载在m_broadcast_mgr->send处处理
    m_broadcast_mgr->GetProcessor()->OnMessage(-1,
        (const uint8_t*)(message.data()), message.length(), NULL, 0);

    // 分发给接入本server的订阅者
    m_broadcast_mgr->Send(channel, (const uint8_t*)(message.data()), message.length(), false);
}


}  // namespace pebble

