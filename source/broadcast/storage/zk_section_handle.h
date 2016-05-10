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


#ifndef ZK_SECTION_HANDLE_H_
#define ZK_SECTION_HANDLE_H_

#include <map>
#include <set>
#include <string>
#include "source/common/pebble_common.h"
#include "source/log/log.h"

class ZookeeperClient;
class CoroutineSchedule;

struct Stat;
struct String_vector;

/// @brief pebble
namespace pebble {
/// @brief zk_section
namespace zk_section {

struct SectionInfo
{
    SectionInfo() : _version(0) {}

    bool operator < (const SectionInfo& rhs) const
    {
        return (_section_name.compare(rhs._section_name) < 0);
    }

    std::string _section_name;
    // _version :
    //     高32位为service_name节点的cversion（子节点更新版本号）
    //     低32位为service_name节点的dataVersion + service_name/instance节点的dataVersion之和
    int64_t _version;
    std::string _section_value;
    std::map<std::string, std::string> _items;
};

typedef cxx::function<void(int rc)> CbCodeReturn;

/// @brief 异步查询接口的执行结果回调
/// @param rc 执行结果返回值
typedef cxx::function<void(int rc, SectionInfo* ret_sec)> CbSectionReturn;

/// @brief 异步查询接口的执行结果回调
/// @param rc 执行结果返回值
typedef std::set<SectionInfo> SectionInfos;
typedef cxx::function<void(int rc, SectionInfos* ret_secs)> CbSectionsReturn;

class SectionHandle {
public:
    explicit SectionHandle(ZookeeperClient* zk_client)
        :   m_zk_client(zk_client) {
    }

    virtual ~SectionHandle() {}

    void Init(const std::string& path) {
        m_path = path;
    }

    void Init(const std::string& path, const std::string& digest_passwd) {
        m_path = path;
        m_digestpwd = digest_passwd;
    }

protected:
    ZookeeperClient*   m_zk_client;

    std::string m_path;
    std::string m_digestpwd;
};

class AddItemHandle : public SectionHandle {
public:
    explicit AddItemHandle(ZookeeperClient* zk_client);

    virtual ~AddItemHandle() {}

    int32_t Start(const std::string& section_name,
                  const std::string& section_value,
                  const std::string& item_name,
                  const std::string& item_value,
                  CbCodeReturn cob);

    int32_t Exit(int32_t rc);

    // 1-1.检查section节点是否存在
    void CheckSectionNode();

    void CbCheckSectionNode(int rc, const char *value, int value_len,
                            const Stat *stat);

    // 1-2.创建section节点
    void CreateSectionNode();

    void CbCreateSectionNode(int rc, const char *value);

    // 1-3.修改section的value
    void SetSectionNode(int32_t data_version);

    void CbSetSectionNode(int rc, const Stat *stat);

    // 2.创建item节点
    void CreateItemNode();

    void CbCreateItemNode(int rc, const char *value);

private:
    std::string         m_section_name;
    std::string         m_section_value;
    std::string         m_item_name;
    std::string         m_item_value;
    CbCodeReturn        m_return_callback;
};

class DelItemHandle : public SectionHandle {
public:
    explicit DelItemHandle(ZookeeperClient* zk_client);

    virtual ~DelItemHandle() {}

    int32_t Start(const std::string& section_name,
                  const std::string& item_name,
                  int32_t data_ver,
                  CbCodeReturn cob);

    int32_t Exit(int32_t rc);

    // 删除节点
    void DelItemNode(const char* path, int32_t data_ver);

    void CbDelItemNode(int rc);

private:
    std::string         m_item_path;
    CbCodeReturn        m_return_callback;
};

class GetSectionHandle : public SectionHandle {
public:
    explicit GetSectionHandle(ZookeeperClient* zk_client);

    virtual ~GetSectionHandle() {}

    int32_t Start(const std::string& section_name, bool is_watch, CbSectionReturn cob);

    int32_t Exit(int32_t rc);

    // 1.查询section节点
    void GetSectionNode();

    void CbGetSectionNode(int rc, const char *value, int value_len, const Stat *stat);

    // 2.查询section子节点
    void GetSectionChildren();

    void CbGetSectionChildren(int rc, const String_vector *strings,
                              const Stat* stat);

    // 3.查询item节点
    int32_t GetItemNode(char* item_name);

    void CbGetItemNode(std::string str_item, int rc, const char *value,
                       int value_len, const Stat *stat);

private:
    // 标记已经回调了，对任何一个item访问失败都失败，导致失败可能提前回调
    bool                m_has_cb;
    int32_t             m_inst_num;
    int32_t             m_is_watch;

    SectionInfo         m_get_result;
    CbSectionReturn     m_return_callback;
};

class GetAllSectionsHandle : public SectionHandle {
public:
    explicit GetAllSectionsHandle(ZookeeperClient* zk_client);

    virtual ~GetAllSectionsHandle() {}

    int32_t Start(bool is_watch, CbSectionsReturn cob);

    int32_t Exit(int32_t rc);

    // 1.查询根节点，获取sections
    void GetSectionsName();

    void CbGetSectionsName(int rc, const String_vector *strings,
                           const Stat* stat);

    // 2.分别查询各个section
    int32_t GetSection(const char* section_name);

    void CbGetSection(int rc, SectionInfo* ret_sec);

private:
    // 标记已经回调了，对任何一个section访问失败都失败，导致失败可能提前回调
    bool                m_has_cb;
    int32_t             m_is_watch;
    int32_t             m_inst_num;

    SectionInfos        m_get_all_result;
    CbSectionsReturn    m_return_callback;
};

#define UNCOMPLETE_VAL  999

struct SyncWaitAdaptor
{
    SyncWaitAdaptor()
        : _rc(UNCOMPLETE_VAL), _ret_section(NULL), _ret_sections(NULL) {}
    explicit SyncWaitAdaptor(SectionInfo* section_info)
        : _rc(UNCOMPLETE_VAL), _ret_section(section_info), _ret_sections(NULL) {}
    explicit SyncWaitAdaptor(SectionInfos* section_infos)
        : _rc(UNCOMPLETE_VAL), _ret_section(NULL), _ret_sections(section_infos) {}
    virtual ~SyncWaitAdaptor() {}

    int _rc;
    SectionInfo* _ret_section;
    SectionInfos* _ret_sections;

    virtual void WaitRsp() {}
};

struct BlockWaitAdaptor : public SyncWaitAdaptor
{
    explicit BlockWaitAdaptor(ZookeeperClient* zk_client)
        : SyncWaitAdaptor(), _zk_client(zk_client) {}
    BlockWaitAdaptor(ZookeeperClient* zk_client, SectionInfo* section_info)
        : SyncWaitAdaptor(section_info), _zk_client(zk_client) {}
    BlockWaitAdaptor(ZookeeperClient* zk_client, SectionInfos* section_infos)
        : SyncWaitAdaptor(section_infos), _zk_client(zk_client) {}

    virtual ~BlockWaitAdaptor() {}

    ZookeeperClient* _zk_client;

    virtual void WaitRsp();

    void OnCodeRsp(int rc);

    void OnSectionInfoRsp(int rc, SectionInfo* ret_sec);

    void OnSectionInfosRsp(int rc, SectionInfos* ret_secs);
};

struct CoroutineWaitAdaptor : public SyncWaitAdaptor
{
    explicit CoroutineWaitAdaptor(CoroutineSchedule* cor_sche)
        : SyncWaitAdaptor(), _cor_sche(cor_sche), _cor_id(-1) {}
    CoroutineWaitAdaptor(CoroutineSchedule* cor_sche, SectionInfo* section_info)
        : SyncWaitAdaptor(section_info), _cor_sche(cor_sche), _cor_id(-1) {}
    CoroutineWaitAdaptor(CoroutineSchedule* cor_sche, SectionInfos* section_infos)
        : SyncWaitAdaptor(section_infos), _cor_sche(cor_sche), _cor_id(-1) {}

    virtual ~CoroutineWaitAdaptor() {}

    CoroutineSchedule* _cor_sche;
    int64_t _cor_id;

    virtual void WaitRsp();

    void OnCodeRsp(int rc);

    void OnSectionInfoRsp(int rc, SectionInfo* ret_sec);

    void OnSectionInfosRsp(int rc, SectionInfos* ret_secs);
};

} // namespace zk_section
} // namespace pebble

#endif // ZK_SECTION_HANDLE_H_
