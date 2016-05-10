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


#ifndef PEBBLE_BROADCAST_CHANNEL_MGR_H
#define PEBBLE_BROADCAST_CHANNEL_MGR_H
#include <map>
#include <string>
#include "source/log/log.h"

namespace pebble {
namespace broadcast {

class Channel;
class ISubscriber;
class ChannelConfAdapter;

enum ChannelType {
    CHANNEL_LOCAL = 0,
    CHANNEL_GLOBAL = 1,
};

/// @brief 频道管理，维护频道列表
/// @note 内部使用接口
class ChannelMgr {
protected:
    static ChannelMgr* m_channel_mgr;

    ChannelMgr();

public:
    static ChannelMgr* Instance() {
        if (NULL == m_channel_mgr) {
            m_channel_mgr = new ChannelMgr();
        }
        return m_channel_mgr;
    }

    virtual ~ChannelMgr();

    void Fini();

    /// @brief 消息驱动，驱动频道广播消息
    int Update();

    /// @brief 设置server广播服务的监听地址(地址用于其他server向自己广播消息或通知自己广播事件)
    int SetServerInfo(const std::string& appid, const std::string& address);

    /// @brief 打开频道，可打开本地频道和全局频道
    int OpenChannel(const std::string& name, int type);

    /// @brief 关闭频道
    int CloseChannel(const std::string& name);

    /// @brief 添加订阅者到某个频道
    int JoinChannel(const std::string& channel_name, const ISubscriber* subscriber);

    /// @brief 删除某个频道的一个订阅者
    int QuitChannel(const std::string& channel_name, const ISubscriber* subscriber);

    /// @brief 把某个订阅者从所有频道中删除
    /// @return 退出的频道数
    int QuitChannel(const ISubscriber* subscriber);

    /// @brief 用户连接信息发生变化，注销老用户，添加新用户
    /// @return 更新的频道数
    int UpdateSubscriber(const ISubscriber* old_subscriber, const ISubscriber* new_subscriber);

    /// @brief 频道是否存在
    bool ChannelExist(const std::string& channel_name);

    /// @brief 向某个频道发布消息
    int Publish(std::string channel_name, const char* msg, size_t len,
            int encode_type, bool forward = true);

private:
    class InstanceAutoFree {
    public:
        ~InstanceAutoFree() {
            if (ChannelMgr::m_channel_mgr) {
                delete ChannelMgr::m_channel_mgr;
                ChannelMgr::m_channel_mgr = NULL;
            }
        }
    };
    static InstanceAutoFree m_instance_auto_free;

private:
    std::map<std::string, Channel*> m_channels;
};


}  // namespace broadcast
}  // namespace pebble

#endif  // PEBBLE_BROADCAST_CHANNEL_MGR_H

