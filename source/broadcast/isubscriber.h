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


#ifndef PEBBLE_BROADCAST_SUBSCRIBER_INTERFACE_H
#define PEBBLE_BROADCAST_SUBSCRIBER_INTERFACE_H
#include <stdint.h>
#include <string>

namespace pebble {
namespace broadcast {

/// @brief 广播订阅者，屏蔽client连接等信息，用户界面接口
class ISubscriber {
public:
    ISubscriber();

    virtual ~ISubscriber();

    /// @brief 返回订阅者id const
    /// @return id
    int64_t Id() const;

    /// @brief 向订阅者推送广播消息
    /// @return 0 成功
    /// @return 非0 失败
    virtual int Push(const std::string& msg, int32_t encode_type) = 0;

    /// @brief 是否直连，如果直连方式，订阅者信息保存到本地\n
    ///                  如果经过pipe，订阅者信息保存至远端存储
    /// @return 1 直连
    /// @return 0 非直连
    /// @return < 0 错误
    virtual int IsDirectConnect() = 0;

    /// @brief 获取远端地址
    /// @return uint64_t 远端地址，值为0时为无效地址
    virtual uint64_t PeerAddress() const;

    /// @brief 获取会话id
    /// @return uint64_t 会话id，-1表示无会话
    /// @note 目前只有pipe连接支持会话
    virtual int64_t SessionId() const;

private:
    static int64_t GenID();

protected:
    int64_t m_id;
};

} // namespace broadcast
} // namespace pebble

#endif  // PEBBLE_BROADCAST_SUBSCRIBER_INTERFACE_H

