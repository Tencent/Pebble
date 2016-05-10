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
#include "source/common/pebble_common.h"
#include "source/common/string_utility.h"
#include "source/service/service_handler.h"

#define NODE_PATH_LEN 512           // 节点路径长度
#define NODE_DATA_LEN 1024          // 节点数据长度
#define SERVICE_NAME_MAX_LEN 128    // 服务名最大长度
#define ADDRESS_MAX_LEN 1000        // 地址列表的最大长度


using namespace cxx::placeholders;

namespace pebble {
namespace service {

struct WatcherValue
{
    ZookeeperClient* zk_client;

    int64_t     game_id;
    int32_t     unit_id;
    int32_t     server_id;
    int32_t     instance_id;

    std::string game_key;

    std::string service_name;
    ServiceRouteType service_route;
    ServiceAddressVec service_addr;
};

void InstanceExistsWatcher(zhandle_t *zh, int type, int state, const char *path, void *watcherCtx)
{
    if (NULL == watcherCtx)
    {
        return;
    }
    WatcherValue* watcher_value = static_cast<WatcherValue*>(watcherCtx);

    if (ZOO_SESSION_EVENT == type)
    {
        // session事件由下层处理
        PLOG_ERROR("ExistsWatcher: session evetn type:%d state:%d", type, state);
    }
    else if (ZOO_DELETED_EVENT == type) /* 被删除了，尝试恢复下 */
    {
        CreateInstHandle* handle = new CreateInstHandle(watcher_value->zk_client);
        handle->Init(watcher_value->game_id, watcher_value->game_key,
            watcher_value->unit_id, watcher_value->server_id, watcher_value->instance_id);
        int ret = handle->Start(watcher_value->service_name, watcher_value->service_route,
            watcher_value->service_addr, NULL);

        PLOG_INFO("ExistsWatcher: node has been deleted, resume node[%s] ret[%d]",
            watcher_value->service_name.c_str(), ret);
        delete watcher_value;
    }
    else /* ZOO_CHANGED_EVENT？节点被修改了？ */
    {
        // 这么神奇的监控被触发了，放弃治疗，不再继续监控了
        PLOG_ERROR("ExistsWatcher: unexpect type:%d state:%d path:%s", type, state, path);
        delete watcher_value;
    }

    return;
}

} // namespace service
} // namespace pebble

using namespace pebble::service;

void ServiceHandle::Init(int64_t game_id, const std::string game_key,
                         int32_t unit_id, int32_t server_id, int32_t inst_id, int32_t retry_times)
{
    m_game_id = game_id;
    m_unit_id = unit_id;
    m_server_id = server_id;
    m_instance_id = inst_id;
    m_retry_times = retry_times;

    char game_id_tmp[256];
    snprintf(game_id_tmp, sizeof(game_id_tmp), "%ld:%s", game_id, game_key.c_str());

    m_game_key = game_key;
    m_digestpwd = m_zk_client->DigestEncrypt(game_id_tmp);
}

int32_t ServiceHandle::AddressVec2String(const ServiceAddressVec& addr_vec, std::string* out_str)
{
    std::ostringstream o;
    for (ServiceAddressVec::const_iterator it = addr_vec.begin() ; it != addr_vec.end() ; ++it) {
        if (it != addr_vec.begin()) {
            o << ",";
        }
        o << it->protocol_type << "@" << it->address_url;
    }
    out_str->assign(o.str());
    return 0;
}

int32_t ServiceHandle::String2AddressVec(const std::string& out_str, ServiceAddressVec* addr_vec)
{
    std::vector<std::string> addr_str;
    StringUtility::Split(out_str, ",", &addr_str);
    for (std::vector<std::string>::iterator it = addr_str.begin() ;
        it != addr_str.end() ; ++it) {
            std::vector<std::string> info_item;
            StringUtility::Split(*it, "@", &info_item);
            if (2 == info_item.size()) {
                ServiceAddressInfo addr_item;
                addr_item.protocol_type = atoi(info_item[0].c_str());
                addr_item.address_url = StringUtility::Trim(info_item[1]);
                addr_vec->push_back(addr_item);
            }
    }
    return 0;
}


CreateInstHandle::CreateInstHandle(ZookeeperClient* zk_client)
    :   ServiceHandle(zk_client), m_service_route_type(kUnknownRoute)
{
}

int32_t CreateInstHandle::Start(const std::string& service_name,
                                ServiceRouteType service_route,
                                const ServiceAddressVec& service_addresses,
                                CbReturnCode cob)
{
    char inst_path[NODE_PATH_LEN];
    snprintf(inst_path, sizeof(inst_path), "/%ld/Runtime/UnitList/%d/ServiceList/%s/%d_%d",
        m_game_id, m_unit_id, service_name.c_str(), m_server_id, m_instance_id);
    m_inst_node_path = inst_path;
    AddressVec2String(service_addresses, &m_service_node_value);

    if (service_name.empty() ||
        service_name.size() > SERVICE_NAME_MAX_LEN ||
        kUnknownRoute >= service_route ||
        kMAXRouteVal <= service_route ||
        m_service_node_value.empty() ||
        m_service_node_value.size() > ADDRESS_MAX_LEN)
    {
        PLOG_ERROR("invaild param");
        return Exit(kSM_BADARGUMENTS);
    }

    m_service_name = service_name;
    m_service_route_type = service_route;
    m_service_address = service_addresses;
    m_return_callback = cob;

    // gaia环境，需要检查Server InstanceID是否创建
    static const std::string kGaiaEnvName("GAIA_CONTAINER_ID");
    if (NULL != getenv(kGaiaEnvName.c_str()))
    {
        CheckServerInst();
    }
    else
    {
        CheckServiceNameNode();
    }
    return 0;
}

int32_t CreateInstHandle::Exit(int32_t rc)
{
    if (m_return_callback != NULL)
    {
        m_return_callback(rc);
    }
    delete this;
    return rc;
}

void CreateInstHandle::CheckServerInst()
{
    char instance_path[NODE_PATH_LEN] = {0};
    snprintf(instance_path, sizeof(instance_path),
        "/%ld/Runtime/UnitList/%d/ServerList/%d/%d",
        m_game_id, m_unit_id, m_server_id, m_instance_id);

    ZkStatCompletionCb cob = cxx::bind(&CreateInstHandle::CbCheckServerInst,
                                       this, _1, _2);
    int32_t ret = m_zk_client->AExists(instance_path, 0, cob);
    if (0 != ret)
    {
        PLOG_ERROR("path[%s] aexists failed ret[%d]", instance_path, ret);
        Exit(ret);
    }
}

void CreateInstHandle::CbCheckServerInst(int rc, const struct Stat *stat)
{
    if (0 != rc)
    {
        PLOG_ERROR("server[%d] instance[%d] check failed ret[%d]", m_server_id, m_instance_id, rc);
        Exit(rc);
    }
    else
    {
        CheckServiceNameNode();
    }
}

void CreateInstHandle::CheckServiceNameNode()
{
    char service_path[NODE_PATH_LEN];
    char service_route_val[16];

    snprintf(service_path, sizeof(service_path), "/%ld/Runtime/UnitList/%d/ServiceList/%s",
        m_game_id, m_unit_id, m_service_name.c_str());
    snprintf(service_route_val, sizeof(service_route_val), "%d",
        static_cast<int>(m_service_route_type));

    char digest_scheme[8] = "digest";
    struct ACL acl;
    struct ACL_vector aclv;

    acl.perms = ZOO_PERM_ALL;
    acl.id.scheme = digest_scheme;
    acl.id.id = const_cast<char *>(m_digestpwd.c_str());

    aclv.count = 1;
    aclv.data = &acl;

    ZkStringCompletionCb cob = cxx::bind(&CreateInstHandle::CbCheckServiceNameNode,
                                         this, _1, _2);
    int32_t ret = m_zk_client->ACreate(service_path, service_route_val,
                                       strlen(service_route_val), &aclv, 0, cob);
    if (0 != ret)
    {
        PLOG_ERROR("path[%s] acreate failed ret[%d]", service_path, ret);
        Exit(ret);
    }
}

void CreateInstHandle::CbCheckServiceNameNode(int rc, const char *value)
{
    switch (rc)
    {
    case 0:
        CreateServiceInstNode();
        break;
    case ZNODEEXISTS:
        GetServiceRoute();
        break;
    default:
        PLOG_ERROR("service[%s] node check failed ret[%d]", m_inst_node_path.c_str(), rc);
        Exit(rc);
        break;
    }
}

void CreateInstHandle::GetServiceRoute()
{
    char service_path[NODE_PATH_LEN];
    snprintf(service_path, sizeof(service_path), "/%ld/Runtime/UnitList/%d/ServiceList/%s",
        m_game_id, m_unit_id, m_service_name.c_str());

    ZkDataCompletionCb cob = cxx::bind(&CreateInstHandle::CbGetServiceRoute,
                                       this, _1, _2, _3, _4);
    int32_t ret = m_zk_client->AGet(service_path, 0, cob);
    if (0 != ret)
    {
        PLOG_ERROR("path[%s] aget failed ret[%d]", service_path, ret);
        Exit(ret);
    }
}

void CreateInstHandle::CbGetServiceRoute(int rc, const char *value, int value_len,
                                         const struct Stat *stat)
{
    if (0 != rc)
    {
        PLOG_ERROR("service[%s] route check failed ret[%d]", m_inst_node_path.c_str(), rc);
        Exit(rc);
        return;
    }

    std::string str_route(value, value_len);
    int zk_route_type = atoi(str_route.c_str());
    if (m_service_route_type == zk_route_type)
    {
        CreateServiceInstNode();
        return;
    }
    if (0 == stat->numChildren)
    {
        SetServiceRoute(stat->version);
        return;
    }

    PLOG_ERROR("service[%s] route[%d] check failed zk_route_type[%d]",
        m_inst_node_path.c_str(), m_service_route_type, zk_route_type);
    Exit(kSM_ROUTECONFLICT);
}

void CreateInstHandle::SetServiceRoute(int32_t version)
{
    char service_path[NODE_PATH_LEN];
    char service_route_val[16];

    snprintf(service_path, sizeof(service_path), "/%ld/Runtime/UnitList/%d/ServiceList/%s",
        m_game_id, m_unit_id, m_service_name.c_str());
    snprintf(service_route_val, sizeof(service_route_val), "%d",
        static_cast<int>(m_service_route_type));

    ZkStatCompletionCb cob = cxx::bind(&CreateInstHandle::CbSetServiceRoute,
                                       this, _1, _2);
    int32_t ret = m_zk_client->ASet(service_path, service_route_val,
                                    strlen(service_route_val), version, cob);
    if (0 != ret)
    {
        PLOG_ERROR("path[%s] aset failed ret[%d]", service_path, ret);
        Exit(ret);
    }
}

void CreateInstHandle::CbSetServiceRoute(int rc, const struct Stat *stat)
{
    if (0 != rc)
    {
        PLOG_ERROR("service[%s] set route failed ret[%d]", m_inst_node_path.c_str(), rc);
        Exit(rc);
    }
    else
    {
        CreateServiceInstNode();
    }
}

void CreateInstHandle::CreateServiceInstNode()
{
    char digest_scheme[8] = "digest";
    struct ACL acl;
    struct ACL_vector aclv;

    acl.perms = ZOO_PERM_ALL;
    acl.id.scheme = digest_scheme;
    acl.id.id = const_cast<char *>(m_digestpwd.c_str());

    aclv.count = 1;
    aclv.data = &acl;

    ZkStringCompletionCb cob = cxx::bind(&CreateInstHandle::CbCreateServiceInstNode,
                                         this, _1, _2);
    int32_t ret = m_zk_client->ACreate(m_inst_node_path.c_str(), m_service_node_value.c_str(),
                                       m_service_node_value.length(), &aclv, ZOO_EPHEMERAL, cob);
    if (0 != ret)
    {
        PLOG_ERROR("path[%s] acreate failed ret[%d]", m_inst_node_path.c_str(), ret);
        Exit(ret);
    }
}

void CreateInstHandle::CbCreateServiceInstNode(int rc, const char *value)
{
    if (0 != rc)
    {
        PLOG_ERROR("create service[%s] inst node failed in %d", m_inst_node_path.c_str(), rc);
        Exit(rc);
        return;
    }
    else
    {
        CheckServiceAddrVaild();
    }
}

void CreateInstHandle::CheckServiceAddrVaild()
{
    CbGetReturn cob = cxx::bind(&CreateInstHandle::CbCheckServiceAddrVaild,
                                this, _1, _2);
    GetAddrHandle* get_addr_handler = new GetAddrHandle(m_zk_client);
    get_addr_handler->Init(m_game_id, m_game_key, m_unit_id, m_server_id, m_instance_id);
    int32_t ret = get_addr_handler->Start(m_service_name, cob, false);
    if (0 != ret)
    {
        PLOG_ERROR("service[%s] get address failed ret[%d]", m_inst_node_path.c_str(), ret);
        Exit(ret);
    }
}

void CreateInstHandle::CbCheckServiceAddrVaild(int rc, GetAddrReturn& ret_addr) // NOLINT
{
    if (0 != rc)
    {
        PLOG_ERROR("check service[%s] address failed in %d", m_inst_node_path.c_str(), rc);
        DelServiceInstNode(rc);
        return;
    }

    // 1、先检查路由策略类型是否一致
    if (m_service_route_type != ret_addr._route_type)
    {
        PLOG_ERROR("CheckServiceVaild expect route[%d] but real route[%d]",
            m_service_route_type, ret_addr._route_type);
        DelServiceInstNode(kSM_ROUTECONFLICT);
        return;
    }

    // 2、检查是否有url相同，但是协议格式不同的异常
    if (ret_addr._addr.size() > 1)
    {
        ServiceAddressVec::iterator prev = ret_addr._addr.begin(), next;
        for (next = prev + 1 ; next != ret_addr._addr.end() ; ++next)
        {
            if (prev->protocol_type != next->protocol_type &&
                prev->address_url == next->address_url)
            {
                PLOG_ERROR("CheckServiceVaild address_url[%s] has different proto[%d, %d]",
                    prev->address_url.c_str(), prev->protocol_type, next->protocol_type);
                DelServiceInstNode(kSM_ADDRESSCONFLICT);
                return;
            }
            prev = next;
        }
    }
    // 通过检查后设置watcher保证节点存在
    SetExistWatcher();
}

void CreateInstHandle::SetExistWatcher()
{
    char inst_path[NODE_PATH_LEN];
    snprintf(inst_path, sizeof(inst_path), "/%ld/Runtime/UnitList/%d/ServiceList/%s/%d_%d",
        m_game_id, m_unit_id, m_service_name.c_str(), m_server_id, m_instance_id);

    WatcherValue* watcher_ctx = new WatcherValue;
    watcher_ctx->zk_client = m_zk_client;
    watcher_ctx->game_id = m_game_id;
    watcher_ctx->unit_id = m_unit_id;
    watcher_ctx->server_id = m_server_id;
    watcher_ctx->instance_id = m_instance_id;
    watcher_ctx->game_key = m_game_key;
    watcher_ctx->service_name = m_service_name;
    watcher_ctx->service_route = m_service_route_type;
    watcher_ctx->service_addr = m_service_address;

    ZkStatCompletionCb cob = cxx::bind(&CreateInstHandle::CbSetExistWatcher,
                                       this, _1, _2, watcher_ctx);
    int32_t ret = m_zk_client->AWExists(inst_path, InstanceExistsWatcher, watcher_ctx, cob);
    if (0 != ret)
    {
        PLOG_ERROR("awexist service[%s] inst node failed in %d", m_service_name.c_str(), ret);
        delete watcher_ctx;
        Exit(0); // 不管监控是否设置成功，只要节点创建成功了，都返回为成功
    }
}

void CreateInstHandle::CbSetExistWatcher(int rc, const struct Stat *stat, void* data)
{
    if (0 != rc)
    {
        PLOG_ERROR("service[%s] inst node set watcher failed in %d", m_service_name.c_str(), rc);

        // 监控没有添加成功，删除数据防止内存泄漏
        WatcherValue* watch_data = reinterpret_cast<WatcherValue*>(data);
        delete watch_data;
    }
    // 不管监控是否设置成功，只要节点创建成功了，都返回为成功
    Exit(0);
}

void CreateInstHandle::DelServiceInstNode(int exit_code)
{
    ZkVoidCompletionCb cb = cxx::bind(&CreateInstHandle::CbDelServiceInstNode,
                                      this, exit_code, _1);
    int32_t ret = m_zk_client->ADelete(m_inst_node_path.c_str(), -1, cb);
    if (0 != ret)
    {
        PLOG_ERROR("adelete service[%s] failed in %d", m_inst_node_path.c_str(), ret);
        Exit(exit_code);
    }
}

void CreateInstHandle::CbDelServiceInstNode(int exit_code, int rc)
{
    // 没有删除成功，只能将ZK断开清理掉临时节点吧
    if (0 != rc)
    {
        PLOG_ERROR("del service[%s] node failed in %d", m_inst_node_path.c_str(), rc);
        m_zk_client->Close();
    }
    Exit(exit_code);
}


GetAddrHandle::GetAddrHandle(ZookeeperClient* zk_client)
    :   ServiceHandle(zk_client), m_has_cb(false), m_is_watch(0), m_inst_num(0)
{
}

int32_t GetAddrHandle::Start(const std::string& service_name, CbGetReturn cob,  bool is_watch)
{
    m_get_addr_result._service_name = service_name;
    m_return_callback = cob;
    m_is_watch = (is_watch ? 1 : 0);

    char service_path[NODE_PATH_LEN];
    snprintf(service_path, sizeof(service_path), "/%ld/Runtime/UnitList/%d/ServiceList/%s",
        m_game_id, m_unit_id, service_name.c_str());
    m_service_node_path.assign(service_path);
    GetServiceRoute();
    return 0;
}

int32_t GetAddrHandle::Exit(int32_t rc)
{
    // children下每个都可以失败触发，保证只回调一次
    if (false == m_has_cb)
    {
        m_has_cb = true;
        m_return_callback(rc, m_get_addr_result);
    }
    if (0 == m_inst_num)
    {
        delete this;
    }
    return rc;
}

void GetAddrHandle::GetServiceRoute()
{
    ZkDataCompletionCb cob = cxx::bind(&GetAddrHandle::CbGetServiceRoute,
                                       this, _1, _2, _3, _4);
    int32_t ret = m_zk_client->AGet(m_service_node_path.c_str(), m_is_watch, cob);
    if (0 != ret)
    {
        PLOG_ERROR("path[%s] aget failed ret[%d]", m_service_node_path.c_str(), ret);
        Exit(ret);
    }
}

void GetAddrHandle::CbGetServiceRoute(int rc, const char *value, int value_len,
                                      const struct Stat *stat)
{
    if (0 != rc)
    {
        PLOG_ERROR("service_name[%s] get route failed ret[%d]",
            m_get_addr_result._service_name.c_str(), rc);
        Exit(rc);
        return;
    }

    m_get_addr_result._version = (static_cast<int64_t>(stat->cversion) << 32);
    m_get_addr_result._version += stat->version;
    // 没有子节点，且不需要设置watch时，直接返回成功后退出
    if (0 == stat->numChildren && 0 == m_is_watch)
    {
        Exit(0);
        return;
    }

    std::string str_route(value, value_len);
    m_get_addr_result._route_type = ((0 == stat->numChildren) ? 0 : atoi(str_route.c_str()));
    GetServiceInst();
}

void GetAddrHandle::GetServiceInst()
{
    ZkStringsCompletionCb cob = cxx::bind(&GetAddrHandle::CbGetServiceInst,
                                          this, _1, _2, _3);
    int32_t ret = m_zk_client->AGetChildren(m_service_node_path.c_str(), m_is_watch, cob);
    if (0 != ret)
    {
        PLOG_ERROR("path[%s] aget_children failed ret[%d]", m_service_node_path.c_str(), ret);
        Exit(ret);
    }
}

void GetAddrHandle::CbGetServiceInst(int rc, const String_vector *strings, const Stat *stat)
{
    if (0 != rc)
    {
        PLOG_ERROR("get service[%s] children failed in %d", m_service_node_path.c_str(), rc);
        Exit(rc);
        return;
    }

    int exit_code = 0;
    for (m_inst_num = 0; m_inst_num < strings->count; ++m_inst_num)
    {
        exit_code = GetInstAddr(strings->data[m_inst_num]);
        if (0 != exit_code)
        {
            if (m_inst_num > 0)
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

int32_t GetAddrHandle::GetInstAddr(char* inst_name)
{
    char service_path[NODE_PATH_LEN];
    snprintf(service_path, sizeof(service_path), "%s/%s",
        m_service_node_path.c_str(), inst_name);

    ZkDataCompletionCb cob = cxx::bind(&GetAddrHandle::CbGetInstAddr,
                                       this, std::string(inst_name), _1, _2, _3, _4);
    int32_t ret = m_zk_client->AGet(service_path, m_is_watch, cob);
    if (0 != ret)
    {
        PLOG_ERROR("path[%s] aget failed in %d", service_path, ret);
    }
    return ret;
}

void GetAddrHandle::CbGetInstAddr(std::string inst_name, int rc, const char *value,
                                  int value_len, const struct Stat *stat)
{
    --m_inst_num;
    if (0 == rc)
    {
        std::string val_str(value, value_len);
        String2AddressVec(val_str, &m_get_addr_result._addr);
        m_get_addr_result._version += stat->version;
    }
    else
    {
        PLOG_ERROR("inst[%s] get failed in %d", inst_name.c_str(), rc);
    }

    // get_children获取到，但实际查询为不存在，则跳过
    if (0 == m_inst_num || (0 != rc && kZK_NONODE != rc))
    {
        // 返回去重地址信息
        std::sort(m_get_addr_result._addr.begin(), m_get_addr_result._addr.end());
        ServiceAddressVec::iterator end_unique = unique(m_get_addr_result._addr.begin(),
                                                        m_get_addr_result._addr.end());
        m_get_addr_result._addr.erase(end_unique, m_get_addr_result._addr.end());

        Exit(rc);
    }
}


GetAllAddrHandle::GetAllAddrHandle(ZookeeperClient* zk_client)
    :   ServiceHandle(zk_client), m_has_cb(false), m_is_watch(0), m_inst_num(0)
{
}

int32_t GetAllAddrHandle::Start(CbGetAllReturn cob, bool is_watch)
{
    m_is_watch = (is_watch ? 1 : 0);
    m_return_callback = cob;

    GetServiceName();
    return 0;
}

int32_t GetAllAddrHandle::Exit(int32_t rc)
{
    if (false == m_has_cb)
    {
        m_has_cb = true;
        m_return_callback(rc, m_get_all_addr_result);
    }
    if (0 == m_inst_num)
    {
        delete this;
    }
    return rc;
}

void GetAllAddrHandle::GetServiceName()
{
    char service_list[NODE_PATH_LEN];
    snprintf(service_list, sizeof(service_list),
        "/%ld/Runtime/UnitList/%d/ServiceList", m_game_id, m_unit_id);

    ZkStringsCompletionCb cob = cxx::bind(&GetAllAddrHandle::CbGetServiceName,
                                          this, _1, _2, _3);
    int32_t ret = m_zk_client->AGetChildren(service_list, m_is_watch, cob);
    if (0 != ret)
    {
        PLOG_ERROR("path[%s] aget_children failed ret[%d]", service_list, ret);
        Exit(ret);
    }
}

void GetAllAddrHandle::CbGetServiceName(int rc, const String_vector *strings, const Stat *stat)
{
    if (0 != rc)
    {
        PLOG_ERROR("get service children failed in %d", rc);
        Exit(rc);
        return;
    }

    int exit_code = 0;
    for (m_inst_num = 0; m_inst_num < strings->count; ++m_inst_num)
    {
        exit_code = GetServiceAddr(strings->data[m_inst_num]);
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

int32_t GetAllAddrHandle::GetServiceAddr(char* inst_name)
{
    GetAddrHandle* handle = new GetAddrHandle(m_zk_client);
    handle->Init(m_game_id, m_game_key, m_unit_id, m_server_id, m_instance_id);

    CbGetReturn cob = cxx::bind(&GetAllAddrHandle::CbGetServiceAddr,
                                this, _1, _2);
    int32_t ret = handle->Start(inst_name, cob, m_is_watch);
    if (0 != ret)
    {
        PLOG_ERROR("get service[%s] failed in %d", inst_name, ret);
    }
    return ret;
}

void GetAllAddrHandle::CbGetServiceAddr(int rc, GetAddrReturn& ret_addr) // NOLINT
{
    --m_inst_num;
    if (0 == rc && ret_addr._route_type != kUnknownRoute && ret_addr._addr.size() != 0)
    {
        m_get_all_addr_result.insert(ret_addr);
    }
    else
    {
        PLOG_INFO("service[%s] get addr failed in %d", ret_addr._service_name.c_str(), rc);
    }

    // get_children获取到，但实际查询为不存在，则跳过
    if (0 == m_inst_num || (0 != rc && kZK_NONODE != rc))
    {
        Exit(rc);
    }
}
