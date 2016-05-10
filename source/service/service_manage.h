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


#ifndef SERVICE_SERVICE_MANAGE_H_
#define SERVICE_SERVICE_MANAGE_H_

#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <map>
#include <set>
#include <string>
#include <vector>
#include <tr1/functional>

#include "source/log/log.h"

// 前置声明
class CoroutineSchedule;
class ZookeeperClient;

/// @brief pebble
namespace pebble {
/// @brief service
namespace service {

// 前置声明
class ServiceAddressCache;

/// @brief 服务管理接口调用返回码
enum ServiceManageErrorCode {
    kSM_OK = 0, ///< Everything is OK
    kSM_SYSTEMERROR = -1, ///< System and server-side errors.
    kSM_RUNTIMEINCONSISTENCY = -2, ///< A runtime inconsistency was found
    kSM_DATAINCONSISTENCY = -3, ///< A data inconsistency was found
    kSM_CONNECTIONLOSS = -4, ///< Connection to the server has been lost
    kSM_MARSHALLINGERROR = -5, ///< Error while marshalling or unmarshalling data
    kSM_UNIMPLEMENTED = -6, ///< Operation is unimplemented
    kSM_OPERATIONTIMEOUT = -7, ///< Operation timeout
    kSM_BADARGUMENTS = -8, ///< Invalid arguments
    kSM_INVALIDSTATE = -9, ///< Invliad zhandle state
    kSM_DNSFAILURE = -10, ///< Error occurs when dns lookup

    kSM_NONODE = -101, ///< Node does not exist
    kSM_NOAUTH = -102, ///< Not authenticated
    kSM_BADVERSION = -103, ///< Version conflict
    kSM_NOCHILDRENFOREPHEMERALS = -108, ///< Ephemeral nodes may not have children
    kSM_NODEEXISTS = -110, ///< The node already exists
    kSM_NOTEMPTY = -111, ///< The node has children
    kSM_SESSIONEXPIRED = -112, ///< The session has been expired by the server
    kSM_INVALIDCALLBACK = -113, ///< Invalid callback specified
    kSM_INVALIDACL = -114, ///< Invalid ACL specified
    kSM_AUTHFAILED = -115, ///< Client authentication failed
    kSM_CLOSING = -116, ///< ZooKeeper is closing
    kSM_NOTHING = -117, ///< (not error) no server responses to process
    kSM_SESSIONMOVED = -118, ///< session moved to another server, so operation is ignored
    kSM_NOQUOTA = -119, ///< quota is not enough.
    kSM_SERVEROVERLOAD = -120, ///< server overload.

    kSM_ENCRYPTFAILED = -200, ///< Digest Encrypt failed.
    kSM_NOTCOMMONUNIT = -204, ///< 分区不是公共分区
    kSM_ROUTECONFLICT = -205, ///< 路由策略类型冲突
    kSM_ADDRESSCONFLICT = -206, ///< 服务的地址信息存在冲突
    kSM_NOTADDRESS = -207, ///< 地址格式不符合
};

/// @brief 服务路由策略类型
enum ServiceRouteType {
    kUnknownRoute = 0,  ///< 未知路由策略，可能是service还没有注册实例
    kRoundRoute = 1,    ///< 轮询路由
    kHashRoute = 2,     ///< 哈希路由
    kModRoute = 3,      ///< 取模路由
    kMAXRouteVal
};

/// @brief 地址信息数组元素
struct ServiceAddressInfo {
    /// @brief 缺省构造函数
    ServiceAddressInfo()
        :   address_url(""), protocol_type(-1) {
    }
    /// @brief 赋值构造函数
    ServiceAddressInfo(const std::string& url, int proto)
        :   address_url(url), protocol_type(proto) {
    }

    /// @brief 重载 <
    bool operator < (const ServiceAddressInfo& rhs) const {
        if (address_url != rhs.address_url) {
            return (address_url < rhs.address_url);
        }
        return (protocol_type < rhs.protocol_type);
    }
    /// @brief 重载 ==
    bool operator == (const ServiceAddressInfo& rhs) const {
        return (protocol_type == rhs.protocol_type) &&
            (address_url == rhs.address_url);
    }

    std::string     address_url;        ///< 监听地址URL
    int             protocol_type;      ///< 地址支持的协议类型
};
/// @brief 地址信息数组
typedef std::vector<ServiceAddressInfo> ServiceAddressVec;

/// @brief 服务地址信息Map，格式如下：\n
///                key：service name\n
///                pair.first：路由策略方式，详细值意义见@ref ServiceRouteType\n
///                pair.second：地址数组，地址字符串已排序，去掉前后空格，无重复
typedef std::map< std::string, std::pair< int, ServiceAddressVec > > ServicesAddress;

/// @brief 服务节点地址信息变化通知回调函数
/// @param service_name 服务名称
/// @param address 获取服务的地址
/// @param service_route service的路由策略，详细值意义见@ref ServiceRouteType
typedef std::tr1::function<void(const std::string& service_name,
                                int service_route,
                                const ServiceAddressVec& address)> CbServiceAddrChanged;

/// @brief 异步注册接口的执行结果回调
/// @param rc 执行结果返回值
typedef std::tr1::function<void(int rc)> CbReturnCode;

/// @brief 异步查询接口的执行结果回调
/// @param rc 执行结果返回值
/// @param addr 查询结果的返回值
typedef std::tr1::function<void(int rc, const ServicesAddress& addr)> CbReturnAddr;

/// @brief 服务管理类
class ServiceManager {
public:
    ServiceManager();

    virtual ~ServiceManager();

    /// @brief 初始化zookeeper连接
    /// @param host zk地址，如 "127.0.0.1:1888,127.0.0.1:2888"
    /// @param time_out 连接超时时间，单位ms
    /// @param log_handle 日志句柄
    /// @return 0成功，其他失败，错误码意义见@ref ServiceManageErrorCode
    int Init(const std::string& host, int time_out);

    /// @brief 注册服务地址变化监听回调
    /// @param func 服务地址变化通知回调函数
    void RegisterWatchFunc(CbServiceAddrChanged func);

    /// @brief 设置app信息和gamekey，Init调用后调用此接口设置信息。
    /// @param game_id 游戏ID
    /// @param game_key 游戏在运营系统注册后返回的key
    /// @param app_id 应用的app信息，如"1.1.1"。
    /// @return 0成功，其他失败，错误码意义见@ref ServiceManageErrorCode
    /// @see 对于客户端等不知道appid，则调用SetAppInfo(game_id, game_key, unit_id)
    int SetAppInfo(int64_t game_id, const std::string& game_key, const std::string& app_id);

    /// @brief 设置gameid和gamekey，Init调用后调用此接口设置信息。
    /// @param game_id 游戏ID
    /// @param game_key 游戏在运营系统注册后返回的key
    /// @param unit_id 分区ID
    /// @return 0成功，其他失败，错误码意义见@ref ServiceManageErrorCode
    /// @see 对于服务端需要注册服务，则调用SetAppInfo(game_id, game_key, app_id)
    int SetAppInfo(int64_t game_id, const std::string& game_key, int unit_id);

    /// @brief 设置协程调度器句柄。
    /// @param cor_sche 协程调度器句柄
    /// @note 对于同步接口调用如果不设置协程调度器或不在协程内调用会直接返回而不阻塞等待
    /// @return 0成功
    int SetCorSchedule(CoroutineSchedule* cor_sche);

    /// @brief 设置查询的缓存
    /// @param use_cache 是否启动查询的缓存
    /// @param refresh_time_ms 缓存的自动刷新时间，最小为5000
    /// @param invaild_time_ms 缓存的失效时间，最小为10000且大于刷新时间
    /// @return 0成功
    int SetCache(bool use_cache = true, int refresh_time_ms = 300000, int invaild_time_ms = 330000);

    /// @brief 同步添加服务实例信息，需先调用SetAppInfo接口设置游戏信息
    /// @param service_name 服务名称
    /// @param service_route 服务的路由策略方式
    /// @param service_addresses 服务地址信息
    /// @return 0成功，其他失败，错误码意义见@ref ServiceManageErrorCode
    /// @note 对于同步接口调用如果不设置协程调度器或不在协程内调用会一直阻塞等待
    int CreateServiceInstance(const std::string& service_name,
                              ServiceRouteType service_route,
                              const ServiceAddressVec& service_addresses);

    /// @brief 异步添加服务实例信息，需先调用SetAppInfo接口设置游戏信息
    /// @param service_name 服务名称
    /// @param service_route 服务的路由策略方式
    /// @param service_addresses 服务地址信息
    /// @param cob 异步操作结果的回调返回
    /// @return 0成功，其他失败，错误码意义见@ref ServiceManageErrorCode
    int CreateServiceInstanceAsync(const std::string& service_name,
                                   ServiceRouteType service_route,
                                   const ServiceAddressVec& service_addresses,
                                   CbReturnCode cob);

    /// @brief 同步获取单个服务的地址
    /// @param service_name 服务名
    /// @param route_type 服务的路由策略方式，详细值意义见@ref ServiceRouteType
    /// @param address 服务地址列表: 地址字符串已排序，去掉前后空格，无重复
    /// @return 0成功，其他失败，错误码意义见@ref ServiceManageErrorCode
    int GetServiceAddress(const std::string& service_name,
                          int* route_type, ServiceAddressVec* address);

    /// @brief 异步获取单个服务的地址
    /// @param service_name 服务名
    /// @param cob 异步操作结果的回调返回
    /// @return 0成功，其他失败，错误码意义见@ref ServiceManageErrorCode
    int GetServiceAddressAsync(const std::string& service_name,
                               CbReturnAddr cob);

    /// @brief 同步监控单个服务的地址
    /// @param service_name 服务名
    /// @return 0成功，其他失败，错误码意义见@ref ServiceManageErrorCode
    /// @note  注册监控成功时会在此接口中回调CbServiceAddrChanged通知地址信息。\n
    ///        即使当前节点不存在或其它原因失败导致zk节点上没有添加成功监控点，在恢复后也可以监控
    int WatchServiceAddress(const std::string& service_name);

    /// @brief 异步监控单个服务的地址
    /// @param service_name 服务名
    /// @param cob 异步操作结果的回调返回
    /// @return 0成功，其他失败，错误码意义见@ref ServiceManageErrorCode
    /// @note  注册监控成功时，在cob回调前，会通过CbServiceAddrChanged通知地址信息。\n
    ///        即使当前节点不存在或其它原因失败导致zk节点上没有添加成功监控点，在恢复后也可以监控
    int WatchServiceAddressAsync(const std::string& service_name,
                                 CbReturnCode cob);

    /// @brief 同步获取分区下所有服务地址
    /// @param address 分区下所有服务的地址信息
    /// @return 0成功，其他失败，错误码意义见@ref ServiceManageErrorCode
    int GetAllServiceAddress(ServicesAddress* address);

    /// @brief 异步获取所有服务地址
    /// @param cob 异步操作结果的回调返回
    /// @return 0成功，其他失败，错误码意义见@ref ServiceManageErrorCode
    int GetAllServiceAddressAsync(CbReturnAddr cob);

    /// @brief 同步监控分区下所有服务地址
    /// @return 0成功，其他失败，错误码意义见@ref ServiceManageErrorCode
    /// @note  注册监控成功时会在此接口中回调CbServiceAddrChanged通知地址信息。\n
    ///        即使当前zk未连接或其它原因失败导致zk节点上没有添加成功监控点，在恢复后也可以监控
    int WatchAllServiceAddress();

    /// @brief 异步监控分区下所有服务地址
    /// @param cob 异步操作结果的回调返回
    /// @return 0成功，其他失败，错误码意义见@ref ServiceManageErrorCode
    /// @note  注册监控成功时，在cob回调前，会通过CbServiceAddrChanged通知地址信息。\n
    ///        即使当前zk未连接或其它原因失败导致zk节点上没有添加成功监控点，在恢复后也可以监控
    int WatchAllServiceAddressAsync(CbReturnCode cob);

    /// @brief 结束调用回收资源
    /// @return 0成功
    int Fini();

    /// @brief 驱动异步更新
    /// @param is_block 是否阻塞，默认为false即可
    /// @note  is_block为true时一定会等待驱动更新一次\n
    ///        除非调用线程只驱动service_manage，否则不要设置为true
    void Update(bool is_block = false);

private:
    /// @brief 权限验证
    /// @param game_id 游戏id
    /// @param game_key 游戏key
    int AddDigestAuth(int64_t game_id, const std::string& game_key);

    /// @brief 注册给zkclient的监控回调函数
    /// @param type 通知事件类型
    /// @param path zk节点路径
    void WatcherFunc(int type, const char* path);

private:
    int m_time_out;
    std::string m_zk_path;

    // 游戏系统相关配置信息
    std::string m_app_id;
    std::string m_game_key;
    int64_t m_game_id;
    int m_unit_id;
    int m_server_id;
    int m_instance_id;
    bool m_use_cache;

    ZookeeperClient* m_zk_client;
    CoroutineSchedule* m_cor_schedule;
    ServiceAddressCache* m_address_cache;
};

}  // namespace service
}  // namespace pebble

#endif  // SERVICE_SERVICE_MANAGE_H_

