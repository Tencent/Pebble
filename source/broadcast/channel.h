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


#ifndef PEBBLE_BROADCAST_CHANNEL_H
#define PEBBLE_BROADCAST_CHANNEL_H
#include <map>
#include <string>
#include <vector>
#include "source/log/log.h"

namespace pebble {
namespace broadcast {

class PebbleBroadcastServiceClient;
class ISubscriber;

/// @brief 广播频道接口，管理加入自己的订阅者，并发送广播消息
/// @note 内部使用接口
class Channel {
public:
    Channel(const std::string& name, size_t max_msg_queue_size = 10240);

    virtual ~Channel();

    /// @brief 订阅此频道
    virtual int Subscribe(const ISubscriber* subscriber);

    /// @brief 取消订阅此频道
    virtual int UnSubscribe(const ISubscriber* subscriber);

    int UpdateSubscriber(const ISubscriber* old_subscriber, const ISubscriber* new_subscriber);

    /// @brief 向频道发送消息
    /// @param msg 消息指针
    /// @param len 消息长度
    /// @param encode_type 消息编码协议
    /// @param forward 是否需要转发给其他server
    int Publish(const char* msg, size_t len, int encode_type, bool forward);

    /// @brief 消息驱动，控制消息的广播速率
    int Update();

    /// @brief 获取频道名称
    std::string GetName();

    /// @brief 打开频道
    virtual int Open();

    /// @brief 关闭频道，关闭后会清除所有订阅者和待广播消息
    virtual void Close();

protected:
    /// @brief 向订阅自己的所有订阅者广播消息
    virtual int Broadcast();

    class BroadcastMessage {
    public:
        int32_t m_encode_type;
        std::string m_message;
        bool m_forward;
        BroadcastMessage(const char* msg, int32_t len, int32_t encode_type, bool forward)
            : m_encode_type(encode_type)
            , m_message(msg, len)
            , m_forward(forward) {}
        ~BroadcastMessage() {}
    };

    // 订阅者id - 订阅者对象
    std::map<int64_t, ISubscriber*> m_subscribers;
    // 会话id - 订阅者对象，无会话不存入列表，订阅者为m_subscribers的一个子集
    std::map<int64_t, ISubscriber*> m_sessioned_subscribers;
    std::vector<BroadcastMessage> m_messages;
    std::string m_name;
    size_t m_queue_size;
};


/// @brief 全局广播频道，除了向本server的订阅者广播外，还向其他server通知广播事件
/// @note 内部使用接口
class GlobalChannel : public Channel {
public:
    GlobalChannel(const std::string& name,
        int max_msg_queue_size = 10240);

    virtual ~GlobalChannel();

    /// @brief 订阅此频道
    virtual int Subscribe(const ISubscriber* subscriber);

    /// @brief 取消订阅此频道
    virtual int UnSubscribe(const ISubscriber* subscriber);

    /// @brief 打开频道，并向zk注册频道信息
    virtual int Open();

    /// @brief 关闭频道，关闭后会清除所有订阅者和待广播消息，并取消zk注册的信息
    virtual void Close();

protected:
    /// @brief 向订阅自己的所有订阅者广播消息，并向其他打开了相同频道的server通知广播事件
    virtual int Broadcast();

private:
    int Notify(const std::string& msg, int32_t encode_type);

    static int SendTo(const std::string& dst_name, const std::string& dst_addr,
        const std::string& msg, const std::string& channel_name, int32_t encode_type);

    static int SendToServer(const std::string& addr, const std::string& msg,
        const std::string& channel_name, int32_t encode_type);

public:
    // 每个server的appid和中转地址唯一，同时需记录其他server、pipe信息(对象复用)
    static std::string m_appid;
    static std::string m_server_addr;

private:
    static std::map<std::string, PebbleBroadcastServiceClient*> m_other_servers;
};

}  // namespace broadcast
}  // namespace pebble

#endif  // PEBBLE_BROADCAST_CHANNEL_H

