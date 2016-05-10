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


#ifndef PEBBLE_BROADCAST_PUBLISHER_H
#define PEBBLE_BROADCAST_PUBLISHER_H
#include <string>
#include <vector>
#include "source/common/pebble_common.h"
#include "source/log/log.h"


namespace pebble {

class PebbleServer;
class Subscriber;
class PebbleBroadcastServiceHandler;
class PebbleChannelMgrServiceHandler;

namespace broadcast {
class AppConfig;
class ChannelMgr;
}

/// @brief 加入频道事件回调，当有客户端申请加入某频道时系统会回调此函数
/// @param channel_name 频道名称
/// @param subscriber 系统创建的订阅者指针
/// @return 0 允许加入
/// @return 非0 不允许加入
typedef cxx::function<int(const std::string& channel_name, Subscriber* subscriber)> // NOLINT
    EventJoinChannel; // NOLINT

/// @brief 退出频道事件回调，当有客户端申请退出某频道时系统会回调此函数
/// @param channel_name 频道名称
/// @param subscriber 系统创建的订阅者指针
/// @return 0 允许退出
/// @return 非0 不允许退出
typedef cxx::function<int(const std::string& channel_name, Subscriber* subscriber)> // NOLINT
    EventQuitChannel; // NOLINT

/// @brief 连接断开事件，只有接入pipe的client(订阅者)支持此事件
/// @param subscriber 系统创建的订阅者指针
typedef cxx::function<void(Subscriber* subscriber)> EventDisconnect;

/// @brief 连接重连事件，只有接入pipe的client(订阅者)支持此事件
/// @param old_subscriber 系统创建的订阅者指针，重连前的信息，重连后不再使用
/// @param old_subscriber 系统创建的订阅者指针，重连后的信息
typedef cxx::function<void(Subscriber* old_subscriber, Subscriber* new_subscriber)> // NOLINT
    EventRelay; // NOLINT

/// @brief 网络事件回调
typedef struct tag_ChannelEventCallbacks {
    EventJoinChannel event_join_channel;
    EventQuitChannel event_quit_channel;
    EventDisconnect event_disconnect;
    EventRelay event_relay;
} ChannelEventCallbacks;


/// @brief 广播模块为pebble框架的内置基础服务，BroadcastService为用户界面
class BroadcastMgr {
    friend class pebble::PebbleServer;
public:
    /// @brief 创建本地广播频道，只在本server内有效
    /// @param name 频道名称
    /// @return 0 成功
    /// @return 非0 失败
    int OpenLocalChannel(const std::string& name);

    /// @brief 创建全局广播频道，频道会注册到zk，全局有效
    /// @param name 频道名称
    /// @return 0 成功
    /// @return 非0 失败
    int OpenGlobalChannel(const std::string& name);

    /// @brief 关闭广播频道，关闭频道后订阅者不会再收到此频道的广播消息
    /// @param name 频道名称
    /// @return 0 成功
    /// @return 非0 失败
    int CloseChannel(const std::string& name);

    /// @brief 向某广播频道中加入一个订阅者
    /// @param channel_name 频道名称
    /// @param subscriber 订阅者
    /// @return 0 成功
    /// @return 非0 失败
    int JoinChannel(const std::string& channel_name, const Subscriber* subscriber);

    /// @brief 取消某广播的订阅
    /// @param channel_name 频道名称
    /// @param subscriber 订阅者
    /// @return 0 成功
    /// @return 非0 失败
    int QuitChannel(const std::string& channel_name, const Subscriber* subscriber);

    /// @brief 注册频道事件处理回调
    /// @param cbs 频道事件处理回调接口
    /// @return 0 成功
    /// @return 非0 失败
    int RegisterChannelEventHandler(const ChannelEventCallbacks& cbs);

    virtual ~BroadcastMgr();

private:
    BroadcastMgr();

    explicit BroadcastMgr(pebble::PebbleServer* server);

    int RegisterPebbleChannelMgrService();

    /// @brief 初始化广播服务
    /// @return 0 初始化成功
    /// @return 非0 初始化失败
    int Init();

    /// @brief 配置应用参数
    /// @return 0 成功
    /// @return 非0 失败
    /// @note 在全局广播场景使用，若只提供本地广播，不需要设置
    int SetAppConfig(const broadcast::AppConfig& app_config);

    int Update();

    /// @brief 设置广播服务监听地址，用于server间广播、跨server转发广播消息
    /// @param listen_url 监听地址，支持tcp/http/tbus传输协议，如:\n
    ///   "tcp://127.0.0.1:6666"\n
    ///   "http://127.0.0.1:7777"\n
    ///   "tbus://1.1.1:8888"
    /// @return 0 成功
    /// @return 非0 失败
    /// @note 若部署多个server，需要设置监听地址，并在Init之前调用
    int SetListenAddress(const std::string& listen_url);

    void OpenChannels();

private:
    pebble::PebbleServer* m_server;
    std::string m_listen_addr;
    PebbleBroadcastServiceHandler* m_bc_handler;
    PebbleChannelMgrServiceHandler* m_cm_handler;
    broadcast::ChannelMgr* m_channel_mgr;
    broadcast::AppConfig* m_app_cfg;
    ChannelEventCallbacks m_channel_event_cbs;
    bool m_init;
    std::vector<std::string> m_local_channels;
    std::vector<std::string> m_global_channels;
};


}  // namespace pebble

#endif  // PEBBLE_BROADCAST_PUBLISHER_H

