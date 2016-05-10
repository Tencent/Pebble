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


#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <sstream>

#include "source/common/base64.h"
#include "source/common/coroutine.h"
#include "source/common/mutex.h"
#include "source/common/sha1.h"
#include "source/common/string_utility.h"
#include "source/common/zookeeper_client.h"
#include "source/log/log.h"
#include "source/service/address_cache.h"
#include "source/service/service_handler.h"
#include "source/service/service_manage.h"


#define NODE_PATH_LEN 512           // 节点路径长度
#define NODE_DATA_LEN 1024          // 节点数据长度
#define SERVICE_NAME_MAX_LEN 128    // 服务名最大长度
#define ADDRESS_MAX_LEN 1000        // 地址列表的最大长度
#define DIGEST_IP_ACL 1         // DIGEST_IP_ACL代表ACL设置既有IP的ALl权限又有digest的Read权限;
#define DIGEST_ACL 2            // DIGEST_ACL代表只设置了digest权限
#define DIGEST_IP_ALL_ACL 3     // DIGEST_IP_ALL_ACL代表ACL设置既有IP的ALl权限又有digest的ALL权限;


using namespace cxx::placeholders;

namespace pebble
{
namespace service
{

struct SyncWaitAdaptor
{
    SyncWaitAdaptor() : _rc(999) {}
    virtual ~SyncWaitAdaptor() {}

    int _rc;

    virtual void WaitRsp() { assert(false); }
};

struct BlockWaitAdaptor : public SyncWaitAdaptor
{
    explicit BlockWaitAdaptor(ZookeeperClient* zk_client)
        : SyncWaitAdaptor(), _zk_client(zk_client) {}

    virtual ~BlockWaitAdaptor() {}

    ZookeeperClient* _zk_client;

    virtual void WaitRsp()
    {
        while (_rc == 999)
        {
            _zk_client->Update(true);
        }
    }

    void OnCodeRsp(int rc)
    {
        _rc = rc;
    }

    void OnAddrRsp(int rc, const ServicesAddress& addr)
    {
        _rc = rc;
    }
};

struct CoroutineWaitAdaptor : public SyncWaitAdaptor
{
    explicit CoroutineWaitAdaptor(CoroutineSchedule* cor_sche)
        : SyncWaitAdaptor(), _cor_sche(cor_sche), _cor_id(-1) {}

    virtual ~CoroutineWaitAdaptor() {}

    CoroutineSchedule* _cor_sche;
    int64_t _cor_id;

    virtual void WaitRsp()
    {
        _cor_id = _cor_sche->CurrentTaskId();
        _cor_sche->Yield();
    }

    void OnCodeRsp(int rc)
    {
        _rc = rc;
        _cor_sche->Resume(_cor_id);
    }

    void OnAddrRsp(int rc, const ServicesAddress& addr)
    {
        _rc = rc;
        _cor_sche->Resume(_cor_id);
    }
};

} // namespace service
} // namespace pebble


using namespace pebble::service;

ServiceManager::ServiceManager()
    :   m_time_out(3000), m_zk_path("/"),
        m_app_id("-1.-1.-1"), m_game_key(""), m_game_id(-1),
        m_unit_id(-1), m_server_id(-1), m_instance_id(-1),
        m_use_cache(true), m_zk_client(NULL),
        m_cor_schedule(NULL), m_address_cache(NULL)
{
    m_zk_client = new ZookeeperClient;
    m_address_cache = new ServiceAddressCache(this);
}

ServiceManager::~ServiceManager()
{
    Fini();

    if (NULL != m_zk_client)
    {
        delete m_zk_client;
        m_zk_client = NULL;
    }
    if (NULL != m_address_cache)
    {
        delete m_address_cache;
        m_address_cache = NULL;
    }
}

int ServiceManager::Init(const std::string& host, int time_out)
{
    m_time_out = time_out;

    zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);  // 设置日志级别,避免出现一些其他信息
    zoo_set_log_stream(stderr);

    m_zk_client->Init(host, time_out, m_zk_path);
    int ret = m_zk_client->Connect();
    if (kSM_OK != ret)
    {
        PLOG_ERROR("zookeeper connect failed ret:%d", ret);
    }

    m_zk_client->SetWatchCallback(cxx::bind(&ServiceManager::WatcherFunc, this, _1, _2));
    return ret;
}

void ServiceManager::RegisterWatchFunc(CbServiceAddrChanged func)
{
    m_address_cache->SetWatchFunc(func);
}

int ServiceManager::SetAppInfo(int64_t game_id, const std::string& game_key,
                               const std::string& appid)
{
    m_game_id = game_id;
    m_game_key = game_key;
    m_app_id = appid;

    std::vector<std::string> vec;
    StringUtility::Split(m_app_id, ".", &vec);
    if (vec.size() != 3) {
        PLOG_ERROR("invalid app_id %s format , must be x.x.x", m_app_id.c_str());
        return -1;
    }
    m_unit_id       = atoi(vec[0].c_str());
    m_server_id     = atoi(vec[1].c_str());
    m_instance_id   = atoi(vec[2].c_str());
    return AddDigestAuth(m_game_id, m_game_key);
}

int ServiceManager::SetAppInfo(int64_t game_id, const std::string& game_key, int unit_id)
{
    m_game_id = game_id;
    m_game_key = game_key;
    m_unit_id = unit_id;
    return AddDigestAuth(m_game_id, m_game_key);
}

int ServiceManager::SetCorSchedule(CoroutineSchedule* cor_sche)
{
    m_cor_schedule = cor_sche;
    return 0;
}

int ServiceManager::SetCache(bool use_cache, int refresh_time_ms, int invaild_time_ms)
{
    if (refresh_time_ms < 5000 ||
        invaild_time_ms < 10000 ||
        invaild_time_ms <= refresh_time_ms)
    {
        return kSM_BADARGUMENTS;
    }

    m_use_cache = use_cache;
    m_address_cache->SetConfig(refresh_time_ms, invaild_time_ms);
    return 0;
}

int ServiceManager::CreateServiceInstance(const std::string& service_name,
                                          ServiceRouteType service_route,
                                          const ServiceAddressVec& service_addresses)
{
    if (service_name.empty())
    {
        return kSM_BADARGUMENTS;
    }

    SyncWaitAdaptor *sync_adaptor = NULL;
    CbReturnCode cob = NULL;
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

    CreateInstHandle* op_handle = new CreateInstHandle(m_zk_client);
    op_handle->Init(m_game_id, m_game_key, m_unit_id, m_server_id, m_instance_id);
    int ret = op_handle->Start(service_name, service_route, service_addresses, cob);
    if (0 == ret)
    {
        sync_adaptor->WaitRsp();
        ret = sync_adaptor->_rc;
    }

    delete sync_adaptor;
    return ret;
}

int ServiceManager::CreateServiceInstanceAsync(const std::string& service_name,
                                               ServiceRouteType service_route,
                                               const ServiceAddressVec& service_addresses,
                                               CbReturnCode cob)
{
    if (service_name.empty() || cob == NULL)
    {
        return kSM_BADARGUMENTS;
    }

    CreateInstHandle* op_handle = new CreateInstHandle(m_zk_client);
    op_handle->Init(m_game_id, m_game_key, m_unit_id, m_server_id, m_instance_id);
    return op_handle->Start(service_name, service_route, service_addresses, cob);
}

int ServiceManager::GetServiceAddress(const std::string& service_name,
                                      int* route_type,
                                      ServiceAddressVec* address)
{
    if (service_name.empty() || !route_type || !address)
    {
        return kSM_BADARGUMENTS;
    }
    if (true == m_use_cache &&
        0 == m_address_cache->GetServiceAddress(service_name, route_type, address))
    {
        return 0;
    }

    SyncWaitAdaptor *sync_adaptor = NULL;
    CbReturnAddr ret_cob = NULL;
    if (NULL != m_cor_schedule && m_cor_schedule->CurrentTaskId() >= 0)
    {
        CoroutineWaitAdaptor *coroutine_adaptor = new CoroutineWaitAdaptor(m_cor_schedule);
        ret_cob = cxx::bind(&CoroutineWaitAdaptor::OnAddrRsp, coroutine_adaptor, _1, _2);
        sync_adaptor = coroutine_adaptor;
    }
    else
    {
        BlockWaitAdaptor *block_adaptor = new BlockWaitAdaptor(m_zk_client);
        ret_cob = cxx::bind(&BlockWaitAdaptor::OnAddrRsp, block_adaptor, _1, _2);
        sync_adaptor = block_adaptor;
    }

    CbGetReturn get_cb = cxx::bind(&ServiceAddressCache::OnGetAddrCb,
                                   m_address_cache, _1, _2, ret_cob);

    GetAddrHandle* op_handle = new GetAddrHandle(m_zk_client);
    op_handle->Init(m_game_id, m_game_key, m_unit_id, m_server_id, m_instance_id);
    int ret = op_handle->Start(service_name, get_cb, false);
    if (0 == ret)
    {
        sync_adaptor->WaitRsp();
        ret = sync_adaptor->_rc;
        m_address_cache->GetServiceAddress(service_name, route_type, address);
    }

    delete sync_adaptor;
    return ret;
}

int ServiceManager::GetServiceAddressAsync(const std::string& service_name, CbReturnAddr cob)
{
    if (service_name.empty())
    {
        return kSM_BADARGUMENTS;
    }

    int route_type = kUnknownRoute;
    ServiceAddressVec addr_vec;
    if (true == m_use_cache &&
        cob != NULL &&
        0 == m_address_cache->GetServiceAddress(service_name, &route_type, &addr_vec))
    {
        ServicesAddress addr;
        addr.insert(std::pair<const std::string,
            std::pair< int, ServiceAddressVec > >(service_name,
                std::pair< int, ServiceAddressVec >(route_type, addr_vec)));
        cob(0, addr);
        return 0;
    }

    CbGetReturn get_cb = cxx::bind(&ServiceAddressCache::OnGetAddrCb,
                                   m_address_cache, _1, _2, cob);

    GetAddrHandle* op_handle = new GetAddrHandle(m_zk_client);
    op_handle->Init(m_game_id, m_game_key, m_unit_id, m_server_id, m_instance_id);
    return op_handle->Start(service_name, get_cb, false);
}

int ServiceManager::WatchServiceAddress(const std::string& service_name)
{
    if (service_name.empty())
    {
        return kSM_BADARGUMENTS;
    }

    SyncWaitAdaptor *sync_adaptor = NULL;
    CbReturnCode ret_cob = NULL;
    if (NULL != m_cor_schedule && m_cor_schedule->CurrentTaskId() >= 0)
    {
        CoroutineWaitAdaptor *coroutine_adaptor = new CoroutineWaitAdaptor(m_cor_schedule);
        ret_cob = cxx::bind(&CoroutineWaitAdaptor::OnCodeRsp, coroutine_adaptor, _1);
        sync_adaptor = coroutine_adaptor;
    }
    else
    {
        BlockWaitAdaptor *block_adaptor = new BlockWaitAdaptor(m_zk_client);
        ret_cob = cxx::bind(&BlockWaitAdaptor::OnCodeRsp, block_adaptor, _1);
        sync_adaptor = block_adaptor;
    }

    CbGetReturn get_cb = cxx::bind(&ServiceAddressCache::OnWatchAddrCb,
                                   m_address_cache, _1, _2, ret_cob);

    GetAddrHandle* op_handle = new GetAddrHandle(m_zk_client);
    op_handle->Init(m_game_id, m_game_key, m_unit_id, m_server_id, m_instance_id);
    int ret = op_handle->Start(service_name, get_cb, true);
    if (0 == ret)
    {
        sync_adaptor->WaitRsp();
        ret = sync_adaptor->_rc;
    }

    delete sync_adaptor;
    return ret;
}

int ServiceManager::WatchServiceAddressAsync(const std::string& service_name, CbReturnCode cob)
{
    if (service_name.empty())
    {
        return kSM_BADARGUMENTS;
    }

    CbGetReturn get_cob = cxx::bind(&ServiceAddressCache::OnWatchAddrCb,
                                    m_address_cache, _1, _2, cob);

    GetAddrHandle* op_handle = new GetAddrHandle(m_zk_client);
    op_handle->Init(m_game_id, m_game_key, m_unit_id, m_server_id, m_instance_id);
    int ret = op_handle->Start(service_name, get_cob, true);
    return ret;
}

int ServiceManager::GetAllServiceAddress(ServicesAddress* address)
{
    if (!address)
    {
        return kSM_BADARGUMENTS;
    }
    if (m_use_cache == true &&
        0 == m_address_cache->GetAllServiceAddress(address))
    {
        return 0;
    }

    SyncWaitAdaptor *sync_adaptor = NULL;
    CbReturnAddr cob = NULL;
    if (NULL != m_cor_schedule && m_cor_schedule->CurrentTaskId() >= 0)
    {
        CoroutineWaitAdaptor *coroutine_adaptor = new CoroutineWaitAdaptor(m_cor_schedule);
        cob = cxx::bind(&CoroutineWaitAdaptor::OnAddrRsp, coroutine_adaptor, _1, _2);
        sync_adaptor = coroutine_adaptor;
    }
    else
    {
        BlockWaitAdaptor *block_adaptor = new BlockWaitAdaptor(m_zk_client);
        cob = cxx::bind(&BlockWaitAdaptor::OnAddrRsp, block_adaptor, _1, _2);
        sync_adaptor = block_adaptor;
    }

    CbGetAllReturn get_cb = cxx::bind(&ServiceAddressCache::OnGetAllAddrCb,
                                      m_address_cache, _1, _2, cob);

    GetAllAddrHandle* op_handle = new GetAllAddrHandle(m_zk_client);
    op_handle->Init(m_game_id, m_game_key, m_unit_id, m_server_id, m_instance_id);
    int ret = op_handle->Start(get_cb, false);
    if (0 == ret)
    {
        sync_adaptor->WaitRsp();
        ret = sync_adaptor->_rc;
        m_address_cache->GetAllServiceAddress(address);
    }

    delete sync_adaptor;
    return ret;
}

int ServiceManager::GetAllServiceAddressAsync(CbReturnAddr cob)
{
    ServicesAddress addr;
    if (m_use_cache == true &&
        cob != NULL &&
        0 == m_address_cache->GetAllServiceAddress(&addr))
    {
        cob(0, addr);
        return 0;
    }

    CbGetAllReturn get_cb = cxx::bind(&ServiceAddressCache::OnGetAllAddrCb,
                                      m_address_cache, _1, _2, cob);

    GetAllAddrHandle* op_handle = new GetAllAddrHandle(m_zk_client);
    op_handle->Init(m_game_id, m_game_key, m_unit_id, m_server_id, m_instance_id);
    return op_handle->Start(get_cb, false);
}

int ServiceManager::WatchAllServiceAddress()
{
    SyncWaitAdaptor *sync_adaptor = NULL;
    CbReturnCode cob = NULL;
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

    CbGetAllReturn get_cb = cxx::bind(&ServiceAddressCache::OnWatchAllAddrCb,
                                      m_address_cache, _1, _2, cob);

    GetAllAddrHandle* op_handle = new GetAllAddrHandle(m_zk_client);
    op_handle->Init(m_game_id, m_game_key, m_unit_id, m_server_id, m_instance_id);
    int ret = op_handle->Start(get_cb, true);
    if (0 == ret)
    {
        sync_adaptor->WaitRsp();
        ret = sync_adaptor->_rc;
    }

    delete sync_adaptor;
    return ret;
}

int ServiceManager::WatchAllServiceAddressAsync(CbReturnCode cob)
{
    CbGetAllReturn get_cb = cxx::bind(&ServiceAddressCache::OnWatchAllAddrCb,
                                      m_address_cache, _1, _2, cob);

    GetAllAddrHandle* op_handle = new GetAllAddrHandle(m_zk_client);
    op_handle->Init(m_game_id, m_game_key, m_unit_id, m_server_id, m_instance_id);
    int ret = op_handle->Start(get_cb, true);
    return ret;
}

int ServiceManager::Fini()
{
    if (NULL != m_zk_client)
    {
        m_zk_client->Close(true);
    }
    if (NULL != m_address_cache)
    {
        m_address_cache->Clear();
    }

    return 0;
}

void ServiceManager::Update(bool is_block)
{
    if (NULL != m_zk_client)
    {
        m_zk_client->Update(is_block);
        m_address_cache->Update();
    }
}

int ServiceManager::AddDigestAuth(int64_t game_id, const std::string& game_key)
{
    char game_id_tmp[256];
    snprintf(game_id_tmp, sizeof(game_id_tmp), "%ld:%s", game_id,
             game_key.c_str());
    std::string user_auth = game_id_tmp;

    int ret = m_zk_client->AddDigestAuth(user_auth);
    if (kSM_OK != ret)
    {
        PLOG_ERROR("zoo_add_auth error!ret:%d", ret);
    }
    return ret;
}

/* 
    1. ZOO_CHILD_EVENT: 下面路径孩子节点发生变化(变多、变少)
    /game_id/Runtime/UnitList/unit_id/ServiceList
    增加service或删除service，需记录上次记录的service对比

    /game_id/Runtime/UnitList/unit_id/ServiceList/service_name
    service_name孩子节点变化实际和ZOO_DELETED_EVENT处理等效

    2. ZOO_DELETED_EVENT/ZOO_CHANGED_EVENT: 下面路径节点发生变化(修改、删除)
    /game_id/Runtime/UnitList/unit_id/ServiceList/service_name
    路由策略信息变化

    /game_id/Runtime/UnitList/unit_id/ServiceList/service_name/instance_id
    service address节点内容变化
*/
void ServiceManager::WatcherFunc(int type, const char* path)
{
    if (!path)
    {
        return;
    }
    PLOG_INFO("ServiceManager Watcher : path[%s] type[%d]", path, type);

    if (ZOO_DELETED_EVENT != type && ZOO_CHANGED_EVENT != type && ZOO_CHILD_EVENT != type) {
        return;
    }

    std::string change_path(path);
    size_t split_pos = change_path.find("/ServiceList");
    if (split_pos == std::string::npos)
    {
        return;
    }

    // 13 = sizeof("/ServiceList/")
    // 没有service name段落，为节点增加或删除
    if (split_pos + 13 >= change_path.length())
    {
        // 需要再次WatchAllServiceAddressAsync设置监控点
        WatchAllServiceAddressAsync(NULL);
    }
    else
    {
        // service_path : service_name or service_name/instance_id
        std::string service_path = change_path.substr(split_pos + 13);
        size_t service_end = service_path.find("/");
        std::string service_name = service_path.substr(0, service_end);
        // 需要再次WatchServiceAddressAsync设置监控点
        WatchServiceAddressAsync(service_name, NULL);
    }
}
