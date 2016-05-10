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


#include <stdio.h>
#include "source/common/coroutine.h"
#include "source/common/pebble_common.h"
#include "source/common/zookeeper_client.h"
#include "source/broadcast/storage/channel_conf_manage.h"


using namespace cxx::placeholders;
using namespace pebble::zk_section;
using namespace pebble::broadcast;

ChannelConfManager::ChannelConfManager()
    :   m_zk_client(NULL), m_cor_schedule(NULL)
{
    m_zk_client = new ::ZookeeperClient();
}

ChannelConfManager::~ChannelConfManager()
{
    if (NULL != m_zk_client)
    {
        m_zk_client->Close(true);
        delete m_zk_client;
        m_zk_client = NULL;
    }
}

int32_t ChannelConfManager::Init(const std::string& zk_host,
                                 int32_t timeout_ms)
{
    m_zk_client->SetWatchCallback(cxx::bind(&ChannelConfManager::ChannelWatchCallback,
                                            this, _1, _2));
    m_zk_client->Init(zk_host, timeout_ms, "/");
    return m_zk_client->Connect();
}

int32_t ChannelConfManager::SetAppInfo(int64_t game_id,
                                       const std::string& game_key,
                                       int32_t unit_id)
{
    char zk_digest[256];
    char zk_channel_path[512];
    snprintf(zk_digest, sizeof(zk_digest), "%ld:%s", game_id, game_key.c_str());
    snprintf(zk_channel_path, sizeof(zk_channel_path),
        "/%ld/Runtime/UnitList/%d/ChannelList", game_id, unit_id);

    m_zk_channel_path.assign(zk_channel_path);
    std::string raw_digest(zk_digest);
    m_zk_digest_passwd = ::ZookeeperClient::DigestEncrypt(raw_digest);
    return m_zk_client->AddDigestAuth(raw_digest);
}

int32_t ChannelConfManager::Add2Channel(const std::string& channel_name,
                                        const std::string& sub_id,
                                        const std::string& sub_addr)
{
    SyncWaitAdaptor *sync_adaptor = NULL;
    CbCodeReturn cob = NULL;
    if (NULL != m_cor_schedule && m_cor_schedule->CurrentTaskId() >= 0)
    {
        CoroutineWaitAdaptor *coroutine_adaptor = new CoroutineWaitAdaptor(m_cor_schedule);
        cob = cxx::bind(&CoroutineWaitAdaptor::OnCodeRsp, coroutine_adaptor, _1);
        sync_adaptor = coroutine_adaptor;
    }
    else
    {
        BlockWaitAdaptor *block_adaptor = new BlockWaitAdaptor(m_zk_client);
        cob = cxx::bind(&BlockWaitAdaptor::OnCodeRsp, block_adaptor, _1);
        sync_adaptor = block_adaptor;
    }

    int32_t ret = Add2ChannelAsync(channel_name, sub_id, sub_addr, cob);
    if (0 == ret)
    {
        sync_adaptor->WaitRsp();
        ret = sync_adaptor->_rc;
    }

    delete sync_adaptor;
    return ret;
}

int32_t ChannelConfManager::Add2ChannelAsync(const std::string& channel_name,
                                             const std::string& sub_id,
                                             const std::string& sub_addr,
                                             CbCodeReturn cb)
{
    AddItemHandle *op_handle = new AddItemHandle(m_zk_client);
    op_handle->Init(m_zk_channel_path, m_zk_digest_passwd);
    return op_handle->Start(channel_name, std::string(), sub_id, sub_addr, cb);
}

int32_t ChannelConfManager::QuitChannel(const std::string& channel_name,
                                        const std::string& sub_id)
{
    SyncWaitAdaptor *sync_adaptor = NULL;
    CbCodeReturn cob = NULL;
    if (NULL != m_cor_schedule && m_cor_schedule->CurrentTaskId() >= 0)
    {
        CoroutineWaitAdaptor *coroutine_adaptor = new CoroutineWaitAdaptor(m_cor_schedule);
        cob = cxx::bind(&CoroutineWaitAdaptor::OnCodeRsp, coroutine_adaptor, _1);
        sync_adaptor = coroutine_adaptor;
    }
    else
    {
        BlockWaitAdaptor *block_adaptor = new BlockWaitAdaptor(m_zk_client);
        cob = cxx::bind(&BlockWaitAdaptor::OnCodeRsp, block_adaptor, _1);
        sync_adaptor = block_adaptor;
    }

    int32_t ret = QuitChannelAsync(channel_name, sub_id, cob);
    if (0 == ret)
    {
        sync_adaptor->WaitRsp();
        ret = sync_adaptor->_rc;
    }

    delete sync_adaptor;
    return ret;
}

int32_t ChannelConfManager::QuitChannelAsync(const std::string& channel_name,
                                             const std::string& sub_id,
                                             CbCodeReturn cb)
{
    DelItemHandle *op_handle = new DelItemHandle(m_zk_client);
    op_handle->Init(m_zk_channel_path);
    return op_handle->Start(channel_name, sub_id, -1, cb);
}

int32_t ChannelConfManager::WatchChannel(const std::string& channel_name)
{
    SyncWaitAdaptor *sync_adaptor = NULL;
    CbCodeReturn cob = NULL;
    if (NULL != m_cor_schedule && m_cor_schedule->CurrentTaskId() >= 0)
    {
        CoroutineWaitAdaptor *coroutine_adaptor = new CoroutineWaitAdaptor(m_cor_schedule);
        cob = cxx::bind(&CoroutineWaitAdaptor::OnCodeRsp, coroutine_adaptor, _1);
        sync_adaptor = coroutine_adaptor;
    }
    else
    {
        BlockWaitAdaptor *block_adaptor = new BlockWaitAdaptor(m_zk_client);
        cob = cxx::bind(&BlockWaitAdaptor::OnCodeRsp, block_adaptor, _1);
        sync_adaptor = block_adaptor;
    }

    CbSectionReturn sec_cob = cxx::bind(&ChannelConfManager::OnChannelUpdate,
                                        this, _1, _2, cob);
    GetSectionHandle *op_handle = new GetSectionHandle(m_zk_client);
    op_handle->Init(m_zk_channel_path);
    int32_t ret = op_handle->Start(channel_name, true, sec_cob);
    if (0 == ret)
    {
        sync_adaptor->WaitRsp();
        ret = sync_adaptor->_rc;
    }

    delete sync_adaptor;
    return ret;
}

int32_t ChannelConfManager::WatchChannelAsync(const std::string& channel_name, CbCodeReturn cb)
{
    SectionInfo sec_info;
    sec_info._section_name = channel_name;
    m_channel_infos.insert(sec_info);

    CbSectionReturn sec_cob = cxx::bind(&ChannelConfManager::OnChannelUpdate,
                                        this, _1, _2, cb);
    GetSectionHandle *op_handle = new GetSectionHandle(m_zk_client);
    op_handle->Init(m_zk_channel_path);
    return op_handle->Start(channel_name, true, sec_cob);
}

const std::map<std::string, std::string>*
    ChannelConfManager::GetChannel(const std::string& channel_name)
{
    SectionInfo sec_info;
    sec_info._section_name = channel_name;
    SectionInfos::const_iterator it = m_channel_infos.find(sec_info);
    if (it != m_channel_infos.end())
    {
        return &(it->_items);
    }
    return NULL;
}

int32_t ChannelConfManager::UnWatchChannel(const std::string& channel_name)
{
    SectionInfo sec_info;
    sec_info._section_name = channel_name;
    SectionInfos::iterator it = m_channel_infos.find(sec_info);
    if (it != m_channel_infos.end())
    {
        m_channel_infos.erase(it);
    }
    return 0;
}

int32_t ChannelConfManager::Update()
{
    if (NULL == m_zk_client)
    {
        return 0;
    }

    for (SectionInfos::iterator it = m_channel_infos.begin() ;
         it != m_channel_infos.end() ; ++it)
    {
        // 一直失败的，自动继续重试
        if (0 == it->_version)
        {
            WatchChannelAsync(it->_section_name, NULL);
        }
    }
    m_zk_client->Update(false);
    return 0;
}

void ChannelConfManager::ChannelWatchCallback(int type, const char* path)
{
    if (NULL == path || 0 == path[0] ||
        (ZOO_DELETED_EVENT != type && ZOO_CHANGED_EVENT != type && ZOO_CHILD_EVENT != type))
    {
        return;
    }

    std::string change_path(path);
    size_t split_pos = change_path.find("/ChannelList");
    // 13 = sizeof("/ChannelList/")
    // 没有channel name段落，为channel增加或删除
    if (split_pos == std::string::npos || split_pos + 13 >= change_path.length())
    {
        return;
    }

    // channel_path : channel_name or channel_name/client_id
    std::string channel_path = change_path.substr(split_pos + 13);
    size_t channel_end = channel_path.find("/");
    std::string channel_name = channel_path.substr(0, channel_end);

    // 判断是否需要再次设置watch
    SectionInfo sec_info;
    sec_info._section_name = channel_name;
    if (0 != m_channel_infos.count(sec_info))
    {
        WatchChannelAsync(channel_name, NULL);
    }
}

void ChannelConfManager::OnChannelUpdate(int rc, SectionInfo* ret_sec, CbCodeReturn cob)
{
    SectionInfos::iterator it = m_channel_infos.find(*ret_sec);
    if (0 == rc && it != m_channel_infos.end())
    {
        // 仅当版本号有更新后才更新
        if (it->_version < ret_sec->_version)
        {
            m_channel_infos.erase(it);
            m_channel_infos.insert(*ret_sec);
        }
    }
    if (NULL != cob)
    {
        cob(rc);
    }
}
