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

#include "framework/channel_mgr.h"


namespace pebble {


// ChannelMgr本身逻辑简单，暂时不提供last error支持

ChannelMgr::ChannelMgr() {}

ChannelMgr::~ChannelMgr() {}

int32_t ChannelMgr::OpenChannel(const std::string& name) {
    if (name.empty()) {
        return kCHANNEL_INVALID_PARAM;
    }

    m_channels[name];
    return 0;
}

int32_t ChannelMgr::CloseChannel(const std::string& name) {
    int32_t cnt = m_channels.erase(name);
    return cnt > 0 ? 0 : kCHANNEL_NOT_EXIST;
}

bool ChannelMgr::ChannelExist(const std::string& name) {
    return m_channels.find(name) != m_channels.end();
}

int32_t ChannelMgr::JoinChannel(const std::string& name, Subscriber subscriber) {
    cxx::unordered_map<std::string, SubscriberList>::iterator it = m_channels.find(name);
    if (m_channels.end() == it) {
        return kCHANNEL_NOT_EXIST;
    }

    (it->second)[subscriber] = 0;
    return 0;
}

int32_t ChannelMgr::QuitChannel(const std::string& name, Subscriber subscriber) {
    cxx::unordered_map<std::string, SubscriberList>::iterator it = m_channels.find(name);
    if (m_channels.end() == it) {
        return kCHANNEL_NOT_EXIST;
    }

    int32_t cnt = it->second.erase(subscriber);
    return cnt > 0 ? 0 : kCHANNEL_NOT_SUBSCIRBED;
}

int32_t ChannelMgr::QuitChannel(Subscriber subscriber) {
    int32_t ret = 0;
    int32_t num = 0;
    cxx::unordered_map<std::string, SubscriberList>::iterator it = m_channels.begin();
    for (; it != m_channels.end(); ++it) {
        ret = QuitChannel(it->first, subscriber);
        if (0 == ret) {
            ++num;
        }
    }

    return num;
}

const SubscriberList* ChannelMgr::GetSubscriberList(const std::string& name) {
    cxx::unordered_map<std::string, SubscriberList>::iterator it = m_channels.find(name);
    if (m_channels.end() == it) {
        return NULL;
    }

    return &(it->second);
}

} // namespace pebble

