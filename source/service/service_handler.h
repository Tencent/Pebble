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


#ifndef SERVICE_SERVICE_HANDLER_H_
#define SERVICE_SERVICE_HANDLER_H_

#include <set>
#include <string>
#include "source/common/zookeeper_client.h"
#include "source/service/service_manage.h"

/// @brief pebble
namespace pebble {
/// @brief service
namespace service {

struct GetAddrReturn
{
    GetAddrReturn() : _route_type(kUnknownRoute), _version(0) {}

    bool operator < (const GetAddrReturn& rhs) const
    {
        return (_service_name.compare(rhs._service_name) < 0);
    }

    std::string _service_name;
    mutable int32_t _route_type;
    // _version :
    //     高32位为service_name节点的cversion（子节点更新版本号）
    //     低32位为service_name/instance节点的dataVersion之和
    mutable int64_t _version;
    mutable ServiceAddressVec _addr;
};

/// @brief 异步查询接口的执行结果回调
/// @param rc 执行结果返回值
typedef std::tr1::function<void(int rc, GetAddrReturn& ret_addr)> CbGetReturn; // NOLINT

/// @brief 异步查询接口的执行结果回调
/// @param rc 执行结果返回值
typedef std::set<GetAddrReturn> GetAllAddrReturn;
typedef std::tr1::function<void(int rc, GetAllAddrReturn& ret_addrs)> CbGetAllReturn; // NOLINT

class ServiceHandle {
public:
    explicit ServiceHandle(ZookeeperClient* zk_client)
        :   m_zk_client(zk_client),
            m_game_id(0), m_unit_id(-1), m_server_id(-1), m_instance_id(-1), m_retry_times(3) {
    }

    virtual ~ServiceHandle() {}

    void Init(int64_t game_id, const::std::string game_key,
        int32_t unit_id, int32_t server_id, int32_t inst_id, int32_t retry_times = 3);

    int32_t AddressVec2String(const ServiceAddressVec& addr_vec, std::string* out_str);

    int32_t String2AddressVec(const std::string& out_str, ServiceAddressVec* addr_vec);

protected:
    ZookeeperClient*   m_zk_client;

    std::string m_game_key;
    std::string m_digestpwd;

    int64_t     m_game_id;
    int32_t     m_unit_id;
    int32_t     m_server_id;
    int32_t     m_instance_id;
    int32_t     m_retry_times;
};

class CreateInstHandle : public ServiceHandle {
public:
    explicit CreateInstHandle(ZookeeperClient* zk_client);

    virtual ~CreateInstHandle() {}

    int32_t Start(const std::string& service_name,
                  ServiceRouteType service_route,
                  const ServiceAddressVec& service_addresses,
                  CbReturnCode cob);

    int32_t Exit(int32_t rc);

    // 1.检查server instance节点是否存在
    void CheckServerInst();

    void CbCheckServerInst(int rc, const Stat *stat);

    // 2.检查service_name节点是否存在
    void CheckServiceNameNode();

    void CbCheckServiceNameNode(int rc, const char *value);

    // 3-1.获取并检查service_name中设定的路由类型
    void GetServiceRoute();

    void CbGetServiceRoute(int rc, const char *value, int value_len, const Stat *stat);

    // 3-2.如果路由类型没有设置过，则设置service_name中设定的路由类型
    void SetServiceRoute(int32_t version);

    void CbSetServiceRoute(int rc, const Stat *stat);

    // 4.创建service instance节点
    void CreateServiceInstNode();

    void CbCreateServiceInstNode(int rc, const char *value);

    // 5.获取service_name下的地址信息并检查正确性
    void CheckServiceAddrVaild();

    void CbCheckServiceAddrVaild(int rc, GetAddrReturn& ret_addr); // NOLINT

    // 6-1.检查正常建立exits监控保证节点存在性
    void SetExistWatcher();

    void CbSetExistWatcher(int rc, const Stat *stat, void* data);

    // 6-2.检查不通过则删除节点，防止影响其它
    void DelServiceInstNode(int exit_code);

    void CbDelServiceInstNode(int exit_code, int rc);

private:
    std::string         m_service_name;
    ServiceRouteType    m_service_route_type;
    ServiceAddressVec   m_service_address;

    std::string         m_inst_node_path;
    std::string         m_service_node_value;
    CbReturnCode        m_return_callback;
};

class GetAddrHandle : public ServiceHandle {
public:
    explicit GetAddrHandle(ZookeeperClient* zk_client);

    virtual ~GetAddrHandle() {}

    int32_t Start(const std::string& service_name,
                  CbGetReturn cob,
                  bool is_watch);

    int32_t Exit(int32_t rc);

    // 获取service_name中设定的路由类型
    void GetServiceRoute();

    void CbGetServiceRoute(int rc, const char *value, int value_len, const Stat *stat);

    void GetServiceInst();

    void CbGetServiceInst(int rc, const String_vector *strings,
                          const Stat *stat);

    int32_t GetInstAddr(char* inst_name);

    void CbGetInstAddr(std::string inst_name, int rc, const char *value,
                       int value_len, const Stat *stat);

private:
    // 标记已经回调了，对任何一个inst访问失败都失败，导致失败可能提前回调
    bool                m_has_cb;
    int32_t             m_is_watch;
    int32_t             m_inst_num;

    std::string         m_service_node_path;

    GetAddrReturn       m_get_addr_result;
    CbGetReturn         m_return_callback;
};

class GetAllAddrHandle : public ServiceHandle {
public:
    explicit GetAllAddrHandle(ZookeeperClient* zk_client);

    virtual ~GetAllAddrHandle() {}

    int32_t Start(CbGetAllReturn cob, bool is_watch);

    int32_t Exit(int32_t rc);

    void CheckCommonUnit();

    void CbCheckCommonUnit(int rc, const char *value, int value_len, const Stat *stat);

    void GetServiceName();

    void CbGetServiceName(int rc, const String_vector *strings,
                          const Stat *stat);

    // 分别查询各个service的地址
    int32_t GetServiceAddr(char* inst_name);

    void CbGetServiceAddr(int rc, GetAddrReturn& ret_addr); // NOLINT

private:
    // 标记已经回调了，对任何一个service name访问失败都失败，导致失败可能提前回调
    bool                m_has_cb;
    int32_t             m_is_watch;
    int32_t             m_inst_num;

    GetAllAddrReturn    m_get_all_addr_result;
    CbGetAllReturn      m_return_callback;
};

} // namespace service
} // namespace pebble

#endif // SERVICE_SERVICE_HANDLER_H_
