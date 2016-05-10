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


#include "source/app/broadcast_mgr.h"
#include "source/app/coroutine_rpc.h"
#include "source/app/pebble_server.h"
#include "source/app/subscriber.h"
#include "source/broadcast/app_config.h"
#include "source/broadcast/channel_mgr.h"
#include "source/broadcast/channel_conf_adapter.h"
#include "source/broadcast/idl/broadcast_PebbleBroadcastService_if.h"
#include "source/broadcast/idl/broadcast_PebbleChannelMgrService_if.h"
#include "source/common/string_utility.h"



namespace pebble {


class PebbleBroadcastServiceHandler :
    public broadcast::PebbleBroadcastServiceCobSvIf {
public:
    PebbleBroadcastServiceHandler(
        pebble::PebbleServer* server, BroadcastMgr* broadcast_mgr)
        : m_server(server), m_broadcast_mgr(broadcast_mgr) {}
    ~PebbleBroadcastServiceHandler() {}

    /// @brief 用于server间传递广播事件
    /// @param channel_name 频道名称
    /// @param message 需要广播出去的消息，已经经过RPC编码
    /// @param encode_type message的编码格式
    void Broadcast(const std::string& channel_name,
        const std::string& message,
        const int32_t encode_type) {

        if (!broadcast::ChannelMgr::Instance()->ChannelExist(channel_name)) {
            return;
        }

        // 发给自己注册的服务
        pebble::rpc::ConnectionObj conn;
        if (m_server->GetCurConnectionObj(&conn) == 0) {
            static_cast<pebble::rpc::CoroutineRpc*>(pebble::rpc::CoroutineRpc::Instance())
                ->PendingMsg(conn.GetHandle(), conn.GetPeerAddr(), encode_type, message);
        }

        // 发给挂在自己身上的client
        if (m_broadcast_mgr) {
            broadcast::ChannelMgr::Instance()->Publish(
                channel_name, message.data(), message.length(), encode_type, false);
        }
    }

private:
    pebble::PebbleServer* m_server;
    BroadcastMgr* m_broadcast_mgr;
};

class PebbleChannelMgrServiceHandler :
    public broadcast::PebbleChannelMgrServiceCobSvIf {
public:
    PebbleChannelMgrServiceHandler(
        pebble::PebbleServer* server, BroadcastMgr* broadcast_mgr)
        : m_server(server), m_broadcast_mgr(broadcast_mgr), m_event_cbs(NULL) {}

    ~PebbleChannelMgrServiceHandler() {}

    void SetChannelEventHandler(ChannelEventCallbacks* cbs) {
        m_event_cbs = cbs;
    }

    /// @brief 用于客户端申请加入广播频道
    /// @param channel_name 频道名称
    /// @return 订阅的id，<0表示加入失败。退出频道时使用
    void JoinChannel(const std::string& channel_name,
        cxx::function<void(int32_t const& _return)> cob);

    /// @brief 用于客户端申请退出广播频道
    /// @param channel_name 频道名称
    /// @param id 加人频道时返回的id
    /// @return <0 退出失败
    /// @return 0 退出成功
    void QuitChannel(const std::string& channel_name,
        cxx::function<void(int32_t const& _return)> cob);

    /// @brief client连接中断，需要清理此连接相关的订阅者
    void OnDisconnect(int64_t session_id);

    /// @brief client连接重置，需要刷新此连接相关的订阅者
    void OnRelay(int64_t session_id);

private:
    pebble::PebbleServer* m_server;
    BroadcastMgr* m_broadcast_mgr;
    // 会话id - 订阅者对象 (同一会话的订阅者对象复用)
    std::map<int64_t, cxx::shared_ptr<Subscriber> > m_session_subscriber_map;
    ChannelEventCallbacks* m_event_cbs;
};

void PebbleChannelMgrServiceHandler::JoinChannel(const std::string& channel_name,
        cxx::function<void(int32_t const& _return)> cob) {
    if (!m_server || !m_broadcast_mgr) {
        cob(-1);
        return;
    }

    cxx::shared_ptr<Subscriber> subscriber(new Subscriber(m_server));
    int64_t session_id = subscriber->SessionId();
    if (-1 == session_id) {
        cob(-2);
        return;
    }

    bool exist = false;
    std::map<int64_t, cxx::shared_ptr<Subscriber> >::iterator it =
        m_session_subscriber_map.find(session_id);
    if (it != m_session_subscriber_map.end()) {
        exist = true;
        subscriber = it->second;
    }

    if (m_event_cbs && m_event_cbs->event_join_channel) {
        if (m_event_cbs->event_join_channel(channel_name, subscriber.get()) != 0) {
            cob(-3);
            return;
        }
    }

    if (m_broadcast_mgr->JoinChannel(channel_name, subscriber.get()) != 0) {
        cob(-4);
        return;
    }

    if (!exist) {
        m_session_subscriber_map[session_id] = subscriber;
        PLOG_INFO("join session(%ld)", session_id);
    }

    cob(0);
}

void PebbleChannelMgrServiceHandler::QuitChannel(const std::string& channel_name,
        cxx::function<void(int32_t const& _return)> cob) {
    if (!m_server || !m_broadcast_mgr) {
        cob(-1);
        return;
    }

    cxx::shared_ptr<Subscriber> subscriber(new Subscriber(m_server));
    int64_t session_id = subscriber->SessionId();
    if (-1 == session_id) {
        cob(-2);
        return;
    }

    std::map<int64_t, cxx::shared_ptr<Subscriber> >::iterator it =
        m_session_subscriber_map.find(session_id);
    if (m_session_subscriber_map.end() == it) {
        cob(-3);
        return;
    }
    subscriber = it->second;

    if (m_event_cbs && m_event_cbs->event_quit_channel) {
        if (m_event_cbs->event_quit_channel(channel_name, subscriber.get()) != 0) {
            cob(-4);
            return;
        }
    }

    if (m_broadcast_mgr->QuitChannel(channel_name, subscriber.get()) != 0) {
        cob(-5);
        return;
    }

    cob(0);
}

void PebbleChannelMgrServiceHandler::OnDisconnect(int64_t session_id) {
    if (-1 == session_id || !m_broadcast_mgr) {
        return;
    }

    std::map<int64_t, cxx::shared_ptr<Subscriber> >::iterator it =
        m_session_subscriber_map.find(session_id);
    if (m_session_subscriber_map.end() == it) {
        PLOG_TRACE("session(%ld) not found", session_id);
        return;
    }

    int num = broadcast::ChannelMgr::Instance()->QuitChannel((it->second).get());
    PLOG_DEBUG("quit channel num(%d)", num);

    if (m_event_cbs && m_event_cbs->event_disconnect) {
        m_event_cbs->event_disconnect((it->second).get());
    }

    m_session_subscriber_map.erase(it);
}

void PebbleChannelMgrServiceHandler::OnRelay(int64_t session_id) {
    if (-1 == session_id) {
        PLOG_ERROR("unkown session in relay");
        return;
    }

    std::map<int64_t, cxx::shared_ptr<Subscriber> >::iterator it =
        m_session_subscriber_map.find(session_id);
    if (m_session_subscriber_map.end() == it) {
        PLOG_TRACE("session(%ld) not found", session_id);
        return;
    }

    cxx::shared_ptr<Subscriber> subscriber(new Subscriber(m_server));
    if (subscriber->SessionId() != session_id) {
        PLOG_ERROR("new session error(%ld != %ld)", subscriber->SessionId(), session_id);
        return;
    }

    // 通知用户
    if (m_event_cbs && m_event_cbs->event_relay) {
        m_event_cbs->event_relay((it->second).get(), subscriber.get());
    }

    // 注销旧的，注册新的
    int num = broadcast::ChannelMgr::Instance()->UpdateSubscriber(
        (it->second).get(), subscriber.get());
    PLOG_DEBUG("update channel num(%d)", num);

    m_session_subscriber_map.erase(it);
    m_session_subscriber_map[session_id] = subscriber;
}

BroadcastMgr::BroadcastMgr() {
    m_server = NULL;
    m_bc_handler = NULL;
    m_cm_handler = NULL;
    m_channel_mgr = NULL;
    m_app_cfg = NULL;
    m_init = false;
}

BroadcastMgr::BroadcastMgr(pebble::PebbleServer* server) {
    m_server = server;
    m_bc_handler = NULL;
    m_cm_handler = NULL;
    m_channel_mgr = NULL;
    m_app_cfg = NULL;
    m_init = false;
}

BroadcastMgr::~BroadcastMgr() {
    if (m_bc_handler) {
        delete m_bc_handler;
        m_bc_handler = NULL;
    }

    if (m_cm_handler) {
        delete m_cm_handler;
        m_cm_handler = NULL;
    }

    if (m_app_cfg) {
        delete m_app_cfg;
        m_app_cfg = NULL;
    }

    if (m_channel_mgr) {
        m_channel_mgr->Fini();
    }

    broadcast::ChannelConfAdapter* channel_conf = broadcast::ChannelConfAdapter::Instance();
    if (channel_conf) {
        channel_conf->Fini();
    }
}

int BroadcastMgr::RegisterPebbleChannelMgrService() {
    if (NULL == m_server) {
        return -1;
    }

    if (NULL == m_cm_handler) {
        m_cm_handler = new PebbleChannelMgrServiceHandler(m_server, this);
    }
    if (NULL == m_cm_handler) {
        return -2;
    }

    // 频道管理服务需要提前注册，此服务对外公开
    return m_server->RegisterService(m_cm_handler);
}

int BroadcastMgr::Init() {
    if (NULL == m_server) {
        return -1;
    }

    if (NULL == m_app_cfg) {
        m_app_cfg = new broadcast::AppConfig();
    }

    // 初始化频道管理器
    if (NULL == m_channel_mgr) {
        m_channel_mgr = broadcast::ChannelMgr::Instance();
    }
    if (NULL == m_channel_mgr) {
        return -2;
    }
    m_channel_mgr->SetServerInfo(m_app_cfg->app_id, m_listen_addr);

    // 初始化频道数据存储适配器
    int ret = 0;
    broadcast::ChannelConfAdapter* channel_conf = broadcast::ChannelConfAdapter::Instance();
    if (channel_conf) {
        ret = channel_conf->Init(*m_app_cfg);
        if (ret != 0) {
            PLOG_ERROR("ChannelConfAdapter init failed(%d)", ret);
            return -3;
        }
    }

    // 注册内置广播服务
    if (NULL == m_bc_handler) {
        m_bc_handler = new PebbleBroadcastServiceHandler(m_server, this);
    }
    if (NULL == m_bc_handler) {
        return -4;
    }

    // server间传递广播事件服务不对外暴露，在rpc初始化完后注册
    ret = pebble::rpc::Rpc::Instance()->RegisterService(m_bc_handler);
    if (ret != 0) {
        return -5;
    }

    if (!m_listen_addr.empty()) {
        ret = pebble::rpc::Rpc::Instance()->AddServiceManner(
            m_listen_addr, pebble::rpc::PROTOCOL_BINARY, false);
    }
    if (ret != 0) {
        return -6;
    }

    m_init = true;

    return 0;
}

int BroadcastMgr::SetAppConfig(const broadcast::AppConfig& app_config) {
    if (NULL == m_app_cfg) {
        m_app_cfg = new broadcast::AppConfig();
    }
    if (NULL == m_app_cfg) {
        return -1;
    }
    m_app_cfg->game_id  = app_config.game_id;
    m_app_cfg->game_key = app_config.game_key;
    m_app_cfg->app_id   = app_config.app_id;
    m_app_cfg->zk_host  = app_config.zk_host;
    m_app_cfg->timeout_ms = app_config.timeout_ms;

    return 0;
}

int BroadcastMgr::Update() {
    int num = 0;
    if (m_channel_mgr) {
        num += m_channel_mgr->Update();
    }
    return num;
}

int BroadcastMgr::SetListenAddress(const std::string& listen_url) {
    // 监听地址只支持tbus tcp http
    // 待和rpc定义前缀统一
    if (!StringUtility::StartsWith(listen_url, "tbus://")
        && !StringUtility::StartsWith(listen_url, "tcp://")
        && !StringUtility::StartsWith(listen_url, "http://")) {
        PLOG_ERROR("invalid listen address(%s)", listen_url.c_str());
        return -1;
    }

    m_listen_addr = listen_url;
    return 0;
}

int BroadcastMgr::OpenLocalChannel(const std::string& name) {
    if (!m_init) {
        if (name.empty()) {
            return -1;
        }
        m_local_channels.push_back(name);
        return 0;
    }

    if (m_channel_mgr) {
        return m_channel_mgr->OpenChannel(name, broadcast::CHANNEL_LOCAL);
    }

    return -2;
}

int BroadcastMgr::OpenGlobalChannel(const std::string& name) {
    if (!m_init) {
        if (name.empty()) {
            return -1;
        }
        m_global_channels.push_back(name);
        return 0;
    }

    if (m_channel_mgr) {
        return m_channel_mgr->OpenChannel(name, broadcast::CHANNEL_GLOBAL);
    }

    return -2;
}

int BroadcastMgr::CloseChannel(const std::string& name) {
    if (m_channel_mgr) {
        return m_channel_mgr->CloseChannel(name);
    }

    return -1;
}

int BroadcastMgr::JoinChannel(const std::string& channel_name, const Subscriber* subscriber) {
    if (NULL == subscriber) {
        return -1;
    }

    if (m_channel_mgr) {
        return m_channel_mgr->JoinChannel(channel_name, subscriber);
    }

    return -2;
}

int BroadcastMgr::QuitChannel(const std::string& channel_name, const Subscriber* subscriber) {
    if (NULL == subscriber) {
        return -1;
    }

    if (m_channel_mgr) {
        return m_channel_mgr->QuitChannel(channel_name, subscriber);
    }

    return -2;
}

int BroadcastMgr::RegisterChannelEventHandler(const ChannelEventCallbacks& cbs) {
    m_channel_event_cbs = cbs;
    if (m_cm_handler) {
        m_cm_handler->SetChannelEventHandler(&m_channel_event_cbs);
    }
    return 0;
}

void BroadcastMgr::OpenChannels() {
    std::vector<std::string>::iterator it;
    for (it = m_local_channels.begin(); it != m_local_channels.end(); ++it) {
        OpenLocalChannel(*it);
    }
    for (it = m_global_channels.begin(); it != m_global_channels.end(); ++it) {
        OpenGlobalChannel(*it);
    }
    m_local_channels.clear();
    m_global_channels.clear();
}


}  // namespace pebble

