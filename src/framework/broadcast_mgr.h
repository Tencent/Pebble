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

#ifndef _PEBBLE_EXTENSION_BROADCAST_MGR_H_
#define _PEBBLE_EXTENSION_BROADCAST_MGR_H_

#include <string>
#include <vector>

#include "common/platform.h"
#include "framework/channel_mgr.h"


namespace pebble {

class _PebbleBroadcastClient;
class IEventHandler;
class IProcessor;
class Naming;
class ZookeeperNaming;

/// @brief 异步操作(打开或关闭)频道的执行结果回调
/// @param ret_code 执行结果返回值
typedef std::tr1::function<void(int ret_code, const std::string& channel)> CbHandleChannel;


/// @brief 维护频道和订阅信息
class BroadcastMgr {
public:
    /// @brief 同步打开频道，并添加到名字服务
    /// @param channel_name 频道名称
    /// @return 0成功，<0失败
    int32_t OpenChannel(const std::string& channel);

    /// @brief 异步打开频道，并添加到名字服务
    /// @param channel_name 频道名称
    /// @param cb 异步操作回调 @see CbHandleChannel
    /// @return 0成功，<0失败
    int32_t OpenChannelAsync(const std::string& channel, const CbHandleChannel& cb);

    /// @brief 同步关闭频道，并从名字服务中注销
    /// @param channel_name 频道名称
    /// @return 0成功，<0失败
    int32_t CloseChannel(const std::string& channel);

    /// @brief 异步关闭频道，并从名字服务中注销
    /// @param channel_name 频道名称
    /// @param cb 异步操作回调 @see CbHandleChannel
    /// @return 0成功，<0失败
    int32_t CloseChannelAsync(const std::string& channel, const CbHandleChannel& cb);

    /// @brief 频道是否存在
    /// @param channel_name 频道名称
    /// @return true存在 false不存在
    bool ChannelExist(const std::string& channel);

    /// @brief 订阅频道
    /// @param channel_name 频道名称
    /// @param subscriber 订阅者
    /// @return 0成功，<0失败
    int32_t JoinChannel(const std::string& channel, Subscriber subscriber);

    /// @brief 退出频道
    /// @param channel_name 频道名称
    /// @param subscriber 订阅者
    /// @return 0成功，<0失败
    int32_t QuitChannel(const std::string& channel, Subscriber subscriber);

    /// @brief 退出所有关注的频道
    /// @param subscriber 订阅者
    /// @return 退出的频道数
    int32_t QuitChannel(Subscriber subscriber);

//-------------------------以下接口内部使用-------------------------
//---------------------------用户无需关注---------------------------
public:
    BroadcastMgr();
    ~BroadcastMgr();

    int32_t Init(int64_t app_id, const std::string& app_key, Naming* naming);

    int32_t Update(uint32_t overload = 0);

    int64_t BindRelayAddress(const std::string& url);

    int32_t Send(const std::string& channel, const uint8_t* buff, uint32_t buff_len, bool relay);

    int32_t SendV(const std::string& channel, uint32_t msg_frag_num,
        const uint8_t* msg_frag[], uint32_t msg_frag_len[], bool relay);

    /// 关联广播接收处理，收到的广播消息分发给此processor处理，handle填全F
    /// 要有默认值，不用用户设置
    int32_t Attach(IProcessor* processor) {
        m_processor = processor;
        return 0;
    }

    /// 给BroadcastRelayHandler使用
    IProcessor* GetProcessor() {
        return m_processor;
    }

    void SetRelayClient(_PebbleBroadcastClient* relay_client) {
        m_relay_client = relay_client;
    }

    void SetEventHandler(IEventHandler* event_handler) {
        m_event_handler = event_handler;
    }

private:
    void CloseRelayConnections(const std::string& channel);

    int32_t RelayV(const std::string& channel, uint32_t msg_frag_num,
        const uint8_t* msg_frag[], uint32_t msg_frag_len[]);

    void OnChannelChanged(const std::string& channel,
                                const std::vector<std::string>& urls);

    void OnOperateReturn(int32_t ret, const std::string& operate,
        const std::string& channel, const CbHandleChannel& cb);

    void OnWatchReturn(int32_t ret, const std::string& channel);

private:
    struct Address {
        Address() : _handle(-1) {}
        Address(const std::string& url, int64_t handle) : _url(url), _handle(handle) {}

        std::string _url;
        int64_t     _handle;
    };

private:
    ChannelMgr*         m_channel_mgr;
    IProcessor*         m_processor;
    Naming*             m_naming;
    uint32_t            m_overload;
    int64_t             m_app_id;
    std::string         m_app_key;
    std::string         m_relay_address;
    std::string         m_path;
    cxx::unordered_map<std::string, std::vector<Address> > m_relay_connection_map;
    _PebbleBroadcastClient* m_relay_client;
    IEventHandler*  m_event_handler;
};

} // namespace pebble

#endif // _PEBBLE_EXTENSION_BROADCAST_MGR_H_
