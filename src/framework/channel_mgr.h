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

#ifndef _PEBBLE_APP_CHANNEL_MGR_H_
#define _PEBBLE_APP_CHANNEL_MGR_H_

#include <string>

#include "common/error.h"
#include "common/platform.h"

namespace pebble {

/// @brief 频道管理模块错误码定义
typedef enum {
    kCHANNEL_ERROR_BASE              = CHANNEL_ERROR_CODE_BASE,
    kCHANNEL_INVALID_PARAM           = kCHANNEL_ERROR_BASE - 1, // 参数错误
    kCHANNEL_NOT_EXIST               = kCHANNEL_ERROR_BASE - 2, // 频道不存在
    kCHANNEL_NOT_SUBSCIRBED          = kCHANNEL_ERROR_BASE - 3, // 没有订阅
} ChannelErrorCode;

class ChannelErrorStringRegister {
public:
    static void RegisterErrorString() {
        SetErrorString(kCHANNEL_INVALID_PARAM, "invalid paramater");
        SetErrorString(kCHANNEL_NOT_EXIST, "channel not exist");
        SetErrorString(kCHANNEL_NOT_SUBSCIRBED, "un subscribed");
    }
};


/// @brief 订阅者只是一个网络连接(handle)
typedef int64_t Subscriber;
typedef cxx::unordered_map<Subscriber, int64_t> SubscriberList; // 暂使用map做订阅者列表


/// @brief 维护频道和订阅信息
class ChannelMgr {
public:
    ChannelMgr();
    ~ChannelMgr();

    /// @brief 打开频道
    /// @param channel_name 频道名称
    /// @return 0成功，<0失败
    int32_t OpenChannel(const std::string& name);

    /// @brief 关闭频道
    /// @param channel_name 频道名称
    /// @return 0成功，<0失败
    int32_t CloseChannel(const std::string& name);

    /// @brief 频道是否存在
    /// @param channel_name 频道名称
    /// @return true存在 false不存在
    bool ChannelExist(const std::string& name);

    /// @brief 订阅频道
    /// @param channel_name 频道名称
    /// @param subscriber 订阅者
    /// @return 0成功，<0失败
    int32_t JoinChannel(const std::string& name, Subscriber subscriber);

    /// @brief 退出频道
    /// @param channel_name 频道名称
    /// @param subscriber 订阅者
    /// @return 0成功，<0失败
    int32_t QuitChannel(const std::string& name, Subscriber subscriber);

    /// @brief 退出所有关注的频道
    /// @param subscriber 订阅者
    /// @return 退出的频道数
    int32_t QuitChannel(Subscriber subscriber);

    /// @brief 获取订阅者列表
    /// @param channel_name 频道名称
    /// @return NULL失败，非NULL成功
    const SubscriberList* GetSubscriberList(const std::string& channel_name);

private:
    // 频道本质上是一个名字
    cxx::unordered_map<std::string, SubscriberList> m_channels; // 频道列表
};

} // namespace pebble

#endif // _PEBBLE_APP_CHANNEL_MGR_H_