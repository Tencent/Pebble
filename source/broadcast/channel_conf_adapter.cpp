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


#include "source/broadcast/app_config.h"
#include "source/broadcast/channel_conf_adapter.h"
#include "source/broadcast/storage/channel_conf_manage.h"

namespace pebble {
namespace broadcast {

ChannelConfAdapter* ChannelConfAdapter::m_channel_conf_apt = NULL;
ChannelConfAdapter::InstanceAutoFree ChannelConfAdapter::m_instance_auto_free;


ChannelConfAdapter::ChannelConfAdapter() {
    m_channel_conf_manager = NULL;
}

ChannelConfAdapter::~ChannelConfAdapter() {
}

int ChannelConfAdapter::Init(const AppConfig& app_config) {
    if (NULL == m_channel_conf_manager) {
        m_channel_conf_manager = new ChannelConfManager();
    }

    if (NULL == m_channel_conf_manager) {
        return -1;
    }

    int ret = m_channel_conf_manager->Init(app_config.zk_host, app_config.timeout_ms);
    if (ret != 0) {
        PLOG_ERROR("ChannelConfMgr init failed(%d)", ret);
        return -2;
    }

    ret = m_channel_conf_manager->SetAppInfo(app_config.game_id,
        app_config.game_key, (const_cast<AppConfig&>(app_config)).UnitId());
    if (ret != 0) {
        PLOG_ERROR("SetAppInfo failed(%d).", ret);
        return -3;
    }

    return 0;
}

void ChannelConfAdapter::Fini() {
    if (m_channel_conf_manager) {
        delete m_channel_conf_manager;
        m_channel_conf_manager = NULL;
    }
}

int ChannelConfAdapter::Update() {
    int num = 0;
    if (m_channel_conf_manager) {
        num += m_channel_conf_manager->Update();
    }
    return num;
}

int ChannelConfAdapter::OpenChannel(const std::string& name) {
    if (m_channel_conf_manager) {
        // channel manager实现是open时只是设置watch，节点创建在Add2Channel完成
        cxx::function<void(int ret)> cb = cxx::bind(&ChannelConfAdapter::AsyncCb,
            this, cxx::placeholders::_1, WATCHCHANNEL, name, "", "");
        return m_channel_conf_manager->WatchChannelAsync(name, cb);
    }

    return -1;
}

int ChannelConfAdapter::CloseChannel(const std::string& name) {
    if (m_channel_conf_manager) {
        return m_channel_conf_manager->UnWatchChannel(name);
    }

    return -1;
}

int ChannelConfAdapter::AddSubInstance(const std::string& channel_name,
        const std::string& sub_name,
        const std::string& sub_value) {
    if (m_channel_conf_manager) {
        cxx::function<void(int ret)> cb = cxx::bind(&ChannelConfAdapter::AsyncCb,
            this, cxx::placeholders::_1, ADD2CHANNEL, channel_name, sub_name, sub_value);
        return m_channel_conf_manager->Add2ChannelAsync(channel_name, sub_name, sub_value, cb);
    }

    return -1;
}

int ChannelConfAdapter::RmvSubInstance(const std::string& channel_name,
        const std::string& sub_name) {
    if (m_channel_conf_manager) {
        cxx::function<void(int ret)> cb = cxx::bind(&ChannelConfAdapter::AsyncCb,
            this, cxx::placeholders::_1, QUITCHANNEL, channel_name, sub_name, "");
        return m_channel_conf_manager->QuitChannelAsync(channel_name, sub_name, cb);
    }

    return -1;
}

int ChannelConfAdapter::GetSubInstances(const std::string& channel_name,
        std::map<std::string, std::string>** instances) {
    if (NULL == instances) {
        return -1;
    }

    if (m_channel_conf_manager) {
        *instances = const_cast<std::map<std::string, std::string>*>(
            m_channel_conf_manager->GetChannel(channel_name));
    }

    return ((*instances) ? 0 : -1);
}

void ChannelConfAdapter::AsyncCb(int ret, int type, std::string channel_name,
        std::string sub_name, std::string sub_value) {
    if (0 == ret) {
        return;
    }

    PLOG_DEBUG("async call failed:ret(%d) type(%d) channelname(%s) subname(%s) subvalue(%s)",
        ret, type, channel_name.c_str(), sub_name.c_str(), sub_value.c_str());
}


} // namespace broadcast
}  // namespace pebble

