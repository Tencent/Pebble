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


#ifndef CHANNEL_CONF_MANAGE_H
#define CHANNEL_CONF_MANAGE_H

#include <map>
#include <string>
#include "source/broadcast/storage/zk_section_handle.h"

namespace pebble {
namespace broadcast {

typedef ::pebble::zk_section::CbCodeReturn CbCodeReturn;

class ChannelConfManager
{
public:
    ChannelConfManager();
    ~ChannelConfManager();

    int32_t Init(const std::string& zk_host, int32_t timeout_ms);

    int32_t SetAppInfo(int64_t game_id, const std::string& game_key, int32_t unit_id);

    // 如果需要用协程来调用，则设置
    void SetCorSchedule(CoroutineSchedule* cor_schedule) {
        m_cor_schedule = cor_schedule;
    }

    // 将sub_id以指定的sub_addr加入到由channel_name指定的频道中
    int32_t Add2Channel(const std::string& channel_name,
                        const std::string& sub_id,
                        const std::string& sub_addr);

    int32_t Add2ChannelAsync(const std::string& channel_name,
                             const std::string& sub_id,
                             const std::string& sub_addr,
                             CbCodeReturn cb);

    // 将sub_id退出由channel_name指定的频道中
    int32_t QuitChannel(const std::string& channel_name, const std::string& sub_id);

    int32_t QuitChannelAsync(const std::string& channel_name,
                             const std::string& sub_id,
                             CbCodeReturn cb);

    // 监听频道，对于监听的频道会自动监听频道地址信息变化
    int32_t WatchChannel(const std::string& channel_name);

    int32_t WatchChannelAsync(const std::string& channel_name, CbCodeReturn cb);

    // 查询频道的地址信息，本地读取，不存在异步
    // client_info->first : sub_id
    // client_info->second : sub_addr
    const std::map<std::string, std::string>* GetChannel(const std::string& channel_name);

    // 停止监听频道，不再关注频道的变化，本地修改，不存在异步
    int32_t UnWatchChannel(const std::string& channel_name);

    // 异步更新驱动
    int32_t Update();

    // 频道信息变化的回调通知，使用者不用关心
    void ChannelWatchCallback(int type, const char* path);
    void OnChannelUpdate(int rc, ::pebble::zk_section::SectionInfo* ret_sec, CbCodeReturn cob);

private:
    ZookeeperClient *m_zk_client;
    CoroutineSchedule *m_cor_schedule;

    std::string m_zk_channel_path;
    std::string m_zk_digest_passwd;
    ::pebble::zk_section::SectionInfos m_channel_infos;
};

} // namespace broadcast
} // namespace pebble

#endif // CHANNEL_CONF_MANAGE_H
