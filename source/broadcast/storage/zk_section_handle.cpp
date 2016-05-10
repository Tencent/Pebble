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


#include <string.h>
#include <algorithm>
#include <sstream>
#include "source/common/coroutine.h"
#include "source/common/pebble_common.h"
#include "source/common/string_utility.h"
#include "source/common/zookeeper_client.h"
#include "source/broadcast/storage/zk_section_handle.h"

#define NODE_PATH_LEN 512           // 节点路径长度
#define NODE_DATA_LEN 1024          // 节点数据长度
#define NAME_MAX_LEN 128            // 名字最大长度


using namespace cxx::placeholders;
using namespace pebble::zk_section;

AddItemHandle::AddItemHandle(ZookeeperClient* zk_client)
    :   SectionHandle(zk_client)
{
}

int32_t AddItemHandle::Start(const std::string& section_name,
                             const std::string& section_value,
                             const std::string& item_name,
                             const std::string& item_value,
                             CbCodeReturn cob)
{
    if (section_name.empty() ||
        section_name.size() > NAME_MAX_LEN ||
        item_name.empty() ||
        item_name.size() > NAME_MAX_LEN ||
        item_value.empty() ||
        item_value.size() > NODE_DATA_LEN)
    {
        PLOG_ERROR("invaild param");
        return Exit(kZK_BADARGUMENTS);
    }

    m_section_name = section_name;
    m_section_value = section_value;
    m_item_name = item_name;
    m_item_value = item_value;
    m_return_callback = cob;

    CheckSectionNode();
    return 0;
}

int32_t AddItemHandle::Exit(int32_t rc)
{
    if (m_return_callback != NULL)
    {
        m_return_callback(rc);
    }
    delete this;
    return rc;
}

void AddItemHandle::CheckSectionNode()
{
    char section_path[NODE_PATH_LEN];
    snprintf(section_path, sizeof(section_path), "%s/%s",
        m_path.c_str(), m_section_name.c_str());

    ZkDataCompletionCb cob = cxx::bind(&AddItemHandle::CbCheckSectionNode,
                                       this, _1, _2, _3, _4);
    int32_t ret = m_zk_client->AGet(section_path, 0, cob);
    if (0 != ret)
    {
        PLOG_ERROR("path[%s] aget failed ret[%d]", section_path, ret);
        Exit(ret);
    }
}

void AddItemHandle::CbCheckSectionNode(int rc, const char *value, int value_len, const Stat *stat)
{
    switch (rc)
    {
    case 0:
        // section value检查且不一致则判断是否为第一个item
        if (0 != m_section_value.size()
            && 0 != value_len
            && 0 != m_section_value.compare(0, std::string::npos, value, value_len))
        {
            if (0 == stat->numChildren)
            {
                SetSectionNode(stat->version);
            }
            else
            {
                std::string node_val(value, value_len);
                PLOG_ERROR("section[%s] value not equal, node_val[%s] set_val[%s]",
                    m_section_name.c_str(), node_val.c_str(), m_section_value.c_str());
                Exit(kZK_BADARGUMENTS);
            }
        }
        else
        {
            CreateItemNode();
        }
        break;
    case kZK_NONODE:
        CreateSectionNode();
        break;
    default:
        PLOG_ERROR("get section[%s] node failed in %d", m_section_name.c_str(), rc);
        Exit(rc);
        break;
    }
}

void AddItemHandle::CreateSectionNode()
{
    char section_path[NODE_PATH_LEN];
    snprintf(section_path, sizeof(section_path), "%s/%s",
        m_path.c_str(), m_section_name.c_str());

    char digest_scheme[8] = "digest";
    struct ACL acl;
    struct ACL_vector aclv;

    acl.perms = ZOO_PERM_ALL;
    acl.id.scheme = digest_scheme;
    acl.id.id = const_cast<char *>(m_digestpwd.c_str());

    aclv.count = 1;
    aclv.data = &acl;

    ZkStringCompletionCb cob = cxx::bind(&AddItemHandle::CbCreateSectionNode,
                                         this, _1, _2);
    int32_t ret = m_zk_client->ACreate(section_path, m_section_value.c_str(),
                                       m_section_value.size(), &aclv, 0, cob);
    if (0 != ret)
    {
        PLOG_ERROR("path[%s] acreate failed ret[%d]", section_path, ret);
        Exit(ret);
    }
}

void AddItemHandle::CbCreateSectionNode(int rc, const char *value)
{
    switch (rc)
    {
    case 0:
        CreateItemNode();
        break;
    case kZK_NODEEXISTS:
        // 节点已存在，说明有其它并发操作，再次尝试get操作
        CheckSectionNode();
        break;
    default:
        PLOG_ERROR("channel[%s] node check failed ret[%d]", m_section_name.c_str(), rc);
        Exit(rc);
        break;
    }
}

void AddItemHandle::SetSectionNode(int32_t data_version)
{
    char section_path[NODE_PATH_LEN];
    snprintf(section_path, sizeof(section_path), "%s/%s",
        m_path.c_str(), m_section_name.c_str());

    ZkStatCompletionCb cob = cxx::bind(&AddItemHandle::CbSetSectionNode,
                                       this, _1, _2);
    int32_t ret = m_zk_client->ASet(section_path, const_cast<char*>(m_section_value.c_str()),
                                    m_section_value.length(), data_version, cob);
    if (0 != ret)
    {
        PLOG_ERROR("path[%s] aset failed ret[%d]", section_path, ret);
        Exit(ret);
    }
}

void AddItemHandle::CbSetSectionNode(int rc, const Stat *stat)
{
    if (0 != rc)
    {
        PLOG_ERROR("section[%s] set value failed ret[%d]", m_section_name.c_str(), rc);
        Exit(rc);
    }
    else
    {
        CreateItemNode();
    }
}

void AddItemHandle::CreateItemNode()
{
    char item_path[NODE_PATH_LEN];
    snprintf(item_path, sizeof(item_path), "%s/%s/%s",
        m_path.c_str(), m_section_name.c_str(), m_item_name.c_str());

    char digest_scheme[8] = "digest";
    struct ACL acl;
    struct ACL_vector aclv;

    acl.perms = ZOO_PERM_ALL;
    acl.id.scheme = digest_scheme;
    acl.id.id = const_cast<char *>(m_digestpwd.c_str());

    aclv.count = 1;
    aclv.data = &acl;

    ZkStringCompletionCb cob = cxx::bind(&AddItemHandle::CbCreateItemNode,
                                         this, _1, _2);
    int32_t ret = m_zk_client->ACreate(item_path, m_item_value.c_str(),
                                       m_item_value.length(), &aclv, ZOO_EPHEMERAL, cob);
    if (0 != ret)
    {
        PLOG_ERROR("path[%s] acreate failed ret[%d]", item_path, ret);
        Exit(ret);
    }
}

void AddItemHandle::CbCreateItemNode(int rc, const char *value)
{
    if (0 != rc)
    {
        PLOG_ERROR("create section[%s] item[%s] failed in %d",
            m_section_name.c_str(), m_item_name.c_str(), rc);
    }
    Exit(rc);
}

DelItemHandle::DelItemHandle(ZookeeperClient* zk_client)
    :   SectionHandle(zk_client)
{
}

int32_t DelItemHandle::Start(const std::string& section_name,
                             const std::string& item_name,
                             int32_t data_ver,
                             CbCodeReturn cob)
{
    char item_path[NODE_PATH_LEN];
    snprintf(item_path, sizeof(item_path), "%s/%s/%s",
        m_path.c_str(), section_name.c_str(), item_name.c_str());
    m_item_path.assign(item_path);
    m_return_callback = cob;

    DelItemNode(m_item_path.c_str(), data_ver);
    return 0;
}

int32_t DelItemHandle::Exit(int32_t rc)
{
    if (m_return_callback != NULL)
    {
        m_return_callback(rc);
    }
    delete this;
    return rc;
}

void DelItemHandle::DelItemNode(const char* path, int32_t data_ver)
{
    ZkVoidCompletionCb cob = cxx::bind(&DelItemHandle::CbDelItemNode, this, _1);
    int32_t ret = m_zk_client->ADelete(path, data_ver, cob);
    if (0 != ret)
    {
        PLOG_ERROR("path[%s] adel failed ret[%d]", path, ret);
        Exit(ret);
    }
}

void DelItemHandle::CbDelItemNode(int rc)
{
    if (0 != rc)
    {
        PLOG_ERROR("path[%s] delete failed in %d", m_item_path.c_str(), rc);
    }
    Exit(rc);
}

GetSectionHandle::GetSectionHandle(ZookeeperClient* zk_client)
    :   SectionHandle(zk_client), m_has_cb(false), m_inst_num(0), m_is_watch(0)
{
}

int32_t GetSectionHandle::Start(const std::string& section_name, bool is_watch, CbSectionReturn cob)
{
    m_get_result._section_name = section_name;
    m_return_callback = cob;
    m_is_watch = (is_watch ? 1 : 0);

    GetSectionNode();
    return 0;
}

int32_t GetSectionHandle::Exit(int32_t rc)
{
    // children下每个都可以失败触发，保证只回调一次
    if (false == m_has_cb)
    {
        m_has_cb = true;
        m_return_callback(rc, &m_get_result);
    }
    if (0 == m_inst_num)
    {
        delete this;
    }
    return rc;
}

void GetSectionHandle::GetSectionNode()
{
    char section_path[NODE_PATH_LEN];
    snprintf(section_path, sizeof(section_path), "%s/%s",
        m_path.c_str(), m_get_result._section_name.c_str());

    ZkDataCompletionCb cob = cxx::bind(&GetSectionHandle::CbGetSectionNode,
                                       this, _1, _2, _3, _4);
    int32_t ret = m_zk_client->AGet(section_path, m_is_watch, cob);
    if (0 != ret)
    {
        PLOG_ERROR("path[%s] aget failed ret[%d]", section_path, ret);
        Exit(ret);
    }
}

void GetSectionHandle::CbGetSectionNode(int rc, const char *value, int value_len,
                                        const Stat *stat)
{
    if (0 != rc)
    {
        PLOG_ERROR("section[%s] get failed in %d", m_get_result._section_name.c_str(), rc);
        Exit(rc);
    }
    else
    {
        m_get_result._version = stat->version;
        m_get_result._section_value.assign(value, value_len);
        GetSectionChildren();
    }
}

void GetSectionHandle::GetSectionChildren()
{
    char section_path[NODE_PATH_LEN];
    snprintf(section_path, sizeof(section_path), "%s/%s",
        m_path.c_str(), m_get_result._section_name.c_str());

    ZkStringsCompletionCb cob = cxx::bind(&GetSectionHandle::CbGetSectionChildren,
                                          this, _1, _2, _3);
    int32_t ret = m_zk_client->AGetChildren(section_path, m_is_watch, cob);
    if (0 != ret)
    {
        PLOG_ERROR("path[%s] aget_children failed ret[%d]", section_path, ret);
        Exit(ret);
    }
}

void GetSectionHandle::CbGetSectionChildren(int rc, const String_vector *strings,
                                            const Stat* stat)
{
    if (0 != rc)
    {
        PLOG_ERROR("section[%s] get children failed in %d", m_get_result._section_name.c_str(), rc);
        Exit(rc);
        return;
    }

    int exit_code = 0;
    for (m_inst_num = 0; m_inst_num < strings->count; ++m_inst_num)
    {
        exit_code = GetItemNode(strings->data[m_inst_num]);
        if (0 != exit_code)
        {
            if (m_inst_num > 0)
            {
                Exit(exit_code);
            }
            break;
        }
    }
    m_get_result._version += (static_cast<int64_t>(stat->cversion) << 32);

    if (0 == m_inst_num)
    {
        Exit(exit_code);
    }
}

int32_t GetSectionHandle::GetItemNode(char* item_name)
{
    char item_path[NODE_PATH_LEN];
    snprintf(item_path, sizeof(item_path), "%s/%s/%s",
        m_path.c_str(), m_get_result._section_name.c_str(), item_name);

    std::string str_item(item_name);
    ZkDataCompletionCb cob = cxx::bind(&GetSectionHandle::CbGetItemNode,
                                       this, str_item, _1, _2, _3, _4);
    int32_t ret = m_zk_client->AGet(item_path, m_is_watch, cob);
    if (0 != ret)
    {
        PLOG_ERROR("path[%s] aget failed in %d", item_path, ret);
    }
    return ret;
}

void GetSectionHandle::CbGetItemNode(std::string str_item, int rc, const char *value,
                                     int value_len, const Stat *stat)
{
    --m_inst_num;
    if (0 == rc)
    {
        m_get_result._items[str_item].assign(value, value_len);
        m_get_result._version += stat->version;
    }

    if (0 == m_inst_num || (0 != rc && kZK_NONODE != rc))
    {
        Exit(rc);
    }
}

GetAllSectionsHandle::GetAllSectionsHandle(ZookeeperClient* zk_client)
    :   SectionHandle(zk_client), m_has_cb(false), m_is_watch(0), m_inst_num(0)
{
}

int32_t GetAllSectionsHandle::Start(bool is_watch, CbSectionsReturn cob)
{
    m_is_watch = (is_watch ? 1 : 0);
    m_return_callback = cob;

    GetSectionsName();
    return 0;
}

int32_t GetAllSectionsHandle::Exit(int32_t rc)
{
    if (false == m_has_cb)
    {
        m_has_cb = true;
        m_return_callback(rc, &m_get_all_result);
    }
    if (0 == m_inst_num)
    {
        delete this;
    }
    return rc;
}

void GetAllSectionsHandle::GetSectionsName()
{
    ZkStringsCompletionCb cob = cxx::bind(&GetAllSectionsHandle::CbGetSectionsName,
                                          this, _1, _2, _3);
    int32_t ret = m_zk_client->AGetChildren(m_path.c_str(), m_is_watch, cob);
    if (0 != ret)
    {
        PLOG_ERROR("path[%s] aget_children failed ret[%d]", m_path.c_str(), ret);
        Exit(ret);
    }
}

void GetAllSectionsHandle::CbGetSectionsName(int rc, const String_vector *strings,
                                             const Stat *stat)
{
    if (0 != rc)
    {
        PLOG_ERROR("path[%s] get children failed in %d", m_path.c_str(), rc);
        Exit(rc);
        return;
    }

    int exit_code = 0;
    for (m_inst_num = 0; m_inst_num < strings->count; ++m_inst_num)
    {
        exit_code = GetSection(strings->data[m_inst_num]);
        if (0 != exit_code)
        {
            if (m_inst_num > 0) // 只有在不为0时才Exit，防止double free
            {
                Exit(exit_code);
            }
            break;
        }
    }

    if (0 == m_inst_num)
    {
        Exit(exit_code);
    }
}

int32_t GetAllSectionsHandle::GetSection(const char* section_name)
{
    GetSectionHandle* handle = new GetSectionHandle(m_zk_client);
    handle->Init(m_path);

    CbSectionReturn cob = cxx::bind(&GetAllSectionsHandle::CbGetSection,
                                this, _1, _2);
    int32_t ret = handle->Start(section_name, m_is_watch, cob);
    if (0 != ret)
    {
        PLOG_ERROR("get service[%s] failed in %d", section_name, ret);
    }
    return ret;
}

void GetAllSectionsHandle::CbGetSection(int rc, SectionInfo* ret_sec)
{
    --m_inst_num;
    if (0 == rc && ret_sec->_items.size() != 0)
    {
        m_get_all_result.insert(*ret_sec);
    }

    if (0 == m_inst_num || 0 != rc)
    {
        Exit(rc);
    }
}

//////////////////////////////////////////////////////////////////////////

void BlockWaitAdaptor::WaitRsp()
{
    while (_rc == UNCOMPLETE_VAL)
    {
        _zk_client->Update(true);
    }
}

void BlockWaitAdaptor::OnCodeRsp(int rc)
{
    _rc = rc;
}

void BlockWaitAdaptor::OnSectionInfoRsp(int rc, SectionInfo* ret_sec)
{
    _rc = rc;
    if (NULL != _ret_section)
    {
        _ret_section->_section_name = ret_sec->_section_name;
        _ret_section->_section_value = ret_sec->_section_value;
        _ret_section->_version = ret_sec->_version;
        _ret_section->_items.swap(ret_sec->_items);
    }
}

void BlockWaitAdaptor::OnSectionInfosRsp(int rc, SectionInfos* ret_secs)
{
    _rc = rc;
    if (NULL != _ret_sections)
    {
        _ret_sections->swap(*ret_secs);
    }
}

void CoroutineWaitAdaptor::WaitRsp()
{
    _cor_id = _cor_sche->CurrentTaskId();
    _cor_sche->Yield();
}

void CoroutineWaitAdaptor::OnCodeRsp(int rc)
{
    _rc = rc;
    _cor_sche->Resume(_cor_id);
}

void CoroutineWaitAdaptor::OnSectionInfoRsp(int rc, SectionInfo* ret_sec)
{
    _rc = rc;
    if (NULL != _ret_section)
    {
        _ret_section->_section_name = ret_sec->_section_name;
        _ret_section->_section_value = ret_sec->_section_value;
        _ret_section->_version = ret_sec->_version;
        _ret_section->_items.swap(ret_sec->_items);
    }
    _cor_sche->Resume(_cor_id);
}

void CoroutineWaitAdaptor::OnSectionInfosRsp(int rc, SectionInfos* ret_secs)
{
    _rc = rc;
    if (NULL != _ret_sections)
    {
        _ret_sections->swap(*ret_secs);
    }
    _cor_sche->Resume(_cor_id);
}
