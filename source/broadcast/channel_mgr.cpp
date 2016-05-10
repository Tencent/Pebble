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


#include "source/broadcast/channel.h"
#include "source/broadcast/channel_conf_adapter.h"
#include "source/broadcast/channel_mgr.h"
#include "source/broadcast/isubscriber.h"


namespace pebble {
namespace broadcast {

ChannelMgr* ChannelMgr::m_channel_mgr = NULL;
ChannelMgr::InstanceAutoFree ChannelMgr::m_instance_auto_free;


ChannelMgr::ChannelMgr() {
}

ChannelMgr::~ChannelMgr() {
}

void ChannelMgr::Fini() {
    for (std::map<std::string, Channel*>::iterator it = m_channels.begin();
        it != m_channels.end(); ++it) {
        delete it->second;
        it->second = NULL;
    }
}

int ChannelMgr::Update() {
    int num = 0;
    for (std::map<std::string, Channel*>::iterator it = m_channels.begin();
        it != m_channels.end(); ++it) {
        num += (it->second)->Update();
    }

    ChannelConfAdapter* channel_conf = ChannelConfAdapter::Instance();
    if (channel_conf) {
        channel_conf->Update();
    }

    return num;
}

int ChannelMgr::SetServerInfo(const std::string& appid, const std::string& address) {
    GlobalChannel::m_appid = appid;
    GlobalChannel::m_server_addr = address;
    return 0;
}

int ChannelMgr::OpenChannel(const std::string& name, int type) {
    if (name.empty()) {
        PLOG_ERROR("channel name is empty.");
        return -1;
    }

    if (m_channels.find(name) != m_channels.end()) {
        PLOG_ERROR("channel name is existed(%s).", name.c_str());
        return -2;
    }

    Channel* channel = NULL;
    switch (type) {
        case CHANNEL_LOCAL:
            channel = new Channel(name);
            break;

        case CHANNEL_GLOBAL:
            channel = new GlobalChannel(name);
            break;

        default:
            break;
    }

    if (NULL == channel) {
        PLOG_ERROR("new channel failed(%d)", type);
        return -1;
    }

    int ret = channel->Open();
    if (ret != 0) {
        PLOG_ERROR("open channel failed(%d)", ret);

        delete channel;
        return -2;
    }

    m_channels[name] = channel;

    return 0;
}

int ChannelMgr::CloseChannel(const std::string& name) {
    std::map<std::string, Channel*>::iterator it = m_channels.find(name);
    if (m_channels.end() == it) {
        PLOG_ERROR("channel name is unexist(%s).", name.c_str());
        return -1;
    }

    (it->second)->Close();

    delete it->second;
    it->second = NULL;
    m_channels.erase(it);

    return 0;
}

int ChannelMgr::JoinChannel(const std::string& channel_name, const ISubscriber* subscriber) {
    if (NULL == subscriber) {
        return -1;
    }

    std::map<std::string, Channel*>::iterator it = m_channels.find(channel_name);
    if (m_channels.end() == it) {
        PLOG_ERROR("channel name is unexist(%s).", channel_name.c_str());
        return -2;
    }

    return (it->second)->Subscribe(subscriber);
}

int ChannelMgr::QuitChannel(const std::string& channel_name, const ISubscriber* subscriber) {
    if (NULL == subscriber) {
        return -1;
    }

    std::map<std::string, Channel*>::iterator it = m_channels.find(channel_name);
    if (m_channels.end() == it) {
        PLOG_ERROR("channel name is unexist(%s).", channel_name.c_str());
        return -2;
    }

    return (it->second)->UnSubscribe(subscriber);
}

int ChannelMgr::QuitChannel(const ISubscriber* subscriber) {
    if (NULL == subscriber) {
        return -1;
    }

    int num = 0;
    for (std::map<std::string, Channel*>::iterator it = m_channels.begin();
            it != m_channels.end(); ++it) {
        if (QuitChannel(it->first, subscriber) == 0) {
            ++num;
        }
    }

    return num;
}

int ChannelMgr::UpdateSubscriber(const ISubscriber* old_subscriber,
        const ISubscriber* new_subscriber) {
    if (NULL == old_subscriber || NULL == new_subscriber) {
        return -1;
    }

    int num = 0;
    for (std::map<std::string, Channel*>::iterator it = m_channels.begin();
            it != m_channels.end(); ++it) {
        if ((it->second)->UpdateSubscriber(old_subscriber, new_subscriber) == 0) {
            ++num;
        }
    }

    return num;
}

bool ChannelMgr::ChannelExist(const std::string& channel_name) {
    return (m_channels.find(channel_name) != m_channels.end());
}

int ChannelMgr::Publish(std::string channel_name,
    const char* msg, size_t len, int encode_type, bool forward) {
    std::map<std::string, Channel*>::iterator it = m_channels.find(channel_name);
    if (m_channels.end() == it) {
        PLOG_DEBUG("channel name is unexist(%s).", channel_name.c_str());
        return -1;
    }

    return (it->second)->Publish(msg, len, encode_type, forward);
}


} // namespace broadcast
}  // namespace pebble

