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


#ifndef PEBBLE_BROADCAST_CHANNEL_CONF_ADAPTER_H
#define PEBBLE_BROADCAST_CHANNEL_CONF_ADAPTER_H
#include <map>
#include <string>
#include "source/log/log.h"

namespace pebble {
namespace broadcast {

class AppConfig;
class ChannelConfManager;

/// @brief 频道信息存储接口
class ChannelConfAdapter {
protected:
    static ChannelConfAdapter* m_channel_conf_apt;

    ChannelConfAdapter();

public:
    static ChannelConfAdapter* Instance() {
        if (NULL == m_channel_conf_apt) {
            m_channel_conf_apt = new ChannelConfAdapter();
        }
        return m_channel_conf_apt;
    }

    virtual ~ChannelConfAdapter();

    int Init(const AppConfig& app_config);

    void Fini();

    int Update();

    /// @brief
    int OpenChannel(const std::string& name);

    int CloseChannel(const std::string& name);

    int AddSubInstance(const std::string& channel_name,
        const std::string& sub_name,
        const std::string& sub_value);

    int RmvSubInstance(const std::string& channel_name,
        const std::string& sub_name);

    int GetSubInstances(const std::string& channel_name,
        std::map<std::string, std::string>** instances);

private:
    // 需要看下bind传值过大问题
    void AsyncCb(int ret, int type, std::string channel_name,
        std::string sub_name, std::string sub_value);

    enum OpType {
        ADD2CHANNEL = 1,
        QUITCHANNEL = 2,
        WATCHCHANNEL = 3,
    };

    class InstanceAutoFree {
    public:
        ~InstanceAutoFree() {
            if (ChannelConfAdapter::m_channel_conf_apt) {
                delete ChannelConfAdapter::m_channel_conf_apt;
                ChannelConfAdapter::m_channel_conf_apt = NULL;
            }
        }
    };
    static InstanceAutoFree m_instance_auto_free;

private:
    ChannelConfManager* m_channel_conf_manager;
};


}  // namespace broadcast
}  // namespace pebble

#endif  // PEBBLE_BROADCAST_CHANNEL_CONF_ADAPTER_H

