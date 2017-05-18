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

#ifndef  _PEBBLE_CLIENT_PEBBLE_CLIENT_H_
#define  _PEBBLE_CLIENT_PEBBLE_CLIENT_H_

#include <vector>

#include "common/log.h"
#include "common/platform.h"
#include "framework/options.h"
#include "framework/pebble_rpc.h"
#include "framework/router.h"
#include "framework/when_all.h"


//////////////////////////////////////////////////////////////////////////////////////
namespace pebble {

// 前置声明
class CoroutineSchedule;
class IEventHandler;
class IProcessor;
class Naming;
class NamingFactory;
class RouterFactory;
class SessionMgr;
class Stat;
class StatManager;
class Timer;


//////////////////////////////////////////////////////////////////////////////////////

/// @brief 获取pebble版本号
const char* GetVersion();


//////////////////////////////////////////////////////////////////////////////////////

/// @brief 名字服务类型定义
typedef enum {
    kNAMING_TBUSPP = 0,
    kNAMING_ZOOKEEPER = 1,
    kNAMING_BUTT
} NamingType;

/// @brief 路由器类型定义
typedef enum {
    kROUTER_TBUSPP = 0,
    kROUTER_BUTT
} RouterType;

/// @brief Processor类型定义
typedef enum {
    kPEBBLE_RPC_BINARY = 0, // thrift binary编码的pebble rpc实例
    kPEBBLE_RPC_JSON,       // thrift json编码的pebble rpc实例
    kPEBBLE_RPC_PROTOBUF,   // protobuf编码的pebble rpc实例
    kPEBBLE_PIPE,           // pipe协议的processor实例
    kPROCESSOR_TYPE_BUTT
} ProcessorType;


//////////////////////////////////////////////////////////////////////////////////////


/// @brief PebbleApp的一个实现，本身为一个通用的Pebble server程序，聚合了Pebble的各功能模块
class PebbleClient {
public:
    PebbleClient();
    ~PebbleClient();

public:
    /// @brief 初始化接口，若需要特殊配置，请先设置Options @see GetOptions
    /// @return 0 成功
    /// @return <0 失败
    int32_t Init();

    int32_t Update();

    /// @brief 创建一个指定类型的RPC stub实例，只创建不回收，对象需要用户释放
    /// @param service_address 服务的地址，如"udp://127.0.0.1:8880" @see Connect(url)
    /// @param processor_type 使用的RPC类型
    /// @return RPC_CLIENT对象，为NULL时创建失败，新创建的对象需要用户释放
    template<class RPC_CLIENT>
    RPC_CLIENT* NewRpcClientByAddress(const std::string& service_address,
        ProcessorType processor_type = kPEBBLE_RPC_BINARY);

    /// @brief 创建一个指定类型的RPC stub实例，只创建不回收，对象需要用户释放
    /// @param service_name 服务的名字，默认根据名字寻址
    /// @param processor_type 使用的RPC类型
    /// @param router 路由器，使用名字寻址时默认采用轮询策略，用户可获取router实例来修改路由策略
    /// @return RPC_CLIENT对象，为NULL时创建失败，新创建的对象需要用户释放
    template<class RPC_CLIENT>
    RPC_CLIENT* NewRpcClientByName(const std::string& service_name,
        ProcessorType processor_type = kPEBBLE_RPC_BINARY, Router** router = NULL);

    /// @brief 返回Pebble的配置参数，用户可按需修改
    /// @return Pebble的配置参数对象
    /// @note 参数修改在init和reload时生效
    Options* GetOptions() {
        return &m_options;
    }

public:
    // 以下为非常用接口
    int64_t Connect(const std::string &url);

    int32_t Close(int64_t handle);

    IProcessor* GetProcessor(ProcessorType processor_type);

    int32_t Attach(int64_t handle, IProcessor* processor);

    int32_t Attach(Router* router, IProcessor* processor);

    PebbleRpc* GetPebbleRpc(ProcessorType processor_type);

    Naming* GetNaming(NamingType naming_type = kNAMING_TBUSPP);

    Router* GetRouter(const std::string& name, RouterType router_type = kROUTER_TBUSPP);

    Timer* GetTimer() {
        return m_timer;
    }

    SessionMgr* GetSessionMgr();

    Stat* GetStat();

    /// @brief 返回内置协程调度器对象
    /// @return 非NULL 成功
    /// @return NULL 失败
    CoroutineSchedule* GetCoroutineSchedule() {
        return m_coroutine_schedule;
    }

    /// @brief 创建一个协程，并决定是否立即执行
    /// @param routine 协程执行入口函数
    /// @param start_immediately 默认立即执行，设置为false时等下一次update时执行
    /// @return 0 成功
    /// @return <0 失败
    int32_t MakeCoroutine(const cxx::function<void()>& routine, bool start_immediately = true);

private:
    int32_t ProcessMessage();

    void InitLog();

    int32_t InitCoSchedule();

    int32_t InitStat();

    int32_t InitTimer();

    int32_t OnStatTimeout();

    void OnRouterAddressChanged(const std::vector<int64_t>& handles, IProcessor* processor);

    void StatCoroutine(Stat* stat);

    void StatProcessorResource(Stat* stat);

private:
    Options            m_options;
    CoroutineSchedule* m_coroutine_schedule;
    Naming*            m_naming_array[kNAMING_BUTT];
    IProcessor*        m_processor_array[kPROCESSOR_TYPE_BUTT];
    IEventHandler*     m_rpc_event_handler;
    StatManager*       m_stat_manager;
    Timer*             m_timer;
    uint32_t           m_stat_timer_ms; // 资源使用采样定时器，供统计用
    SessionMgr*        m_session_mgr;
    cxx::unordered_map<int64_t, IProcessor*> m_processor_map;
    cxx::unordered_map<std::string, Router*> m_router_map;
};

///////////////////////////////////////////////////////////////////////////////////////

template<class RPC_CLIENT>
RPC_CLIENT* PebbleClient::NewRpcClientByAddress(const std::string& service_address,
    ProcessorType processor_type) {
    PebbleRpc* pebble_rpc = GetPebbleRpc(processor_type);
    if (pebble_rpc == NULL) {
        PLOG_ERROR("GetPebbleRpc failed, processor_type: %d", processor_type);
        return NULL;
    }
    int64_t handle = Connect(service_address);
    if (handle < 0) {
        PLOG_ERROR("connect failed, handle: %ld, service_address: %s",
                   handle, service_address.c_str());
        return NULL;
    }
    if (Attach(handle, pebble_rpc) != 0) {
        PLOG_ERROR("Attach failed, handle: %ld, service_address: %s",
                   handle, service_address.c_str());
        Close(handle);
        return NULL;
    }
    RPC_CLIENT* client = new RPC_CLIENT(pebble_rpc);
    client->SetHandle(handle);
    return client;
}

template<class RPC_CLIENT>
RPC_CLIENT* PebbleClient::NewRpcClientByName(const std::string& service_name,
    ProcessorType processor_type, Router** router) {
    PebbleRpc* pebble_rpc = GetPebbleRpc(processor_type);
    if (pebble_rpc == NULL) {
        PLOG_ERROR("GetPebbleRpc failed, processor_type: %d", processor_type);
        return NULL;
    }
    Router* new_router = GetRouter(service_name);
    if (new_router == NULL) {
        PLOG_ERROR("GetRouter failed");
        return NULL;
    }
    if (Attach(new_router, pebble_rpc) != 0) {
        PLOG_ERROR("Attach failed.");
        return NULL;
    }
    RPC_CLIENT* client = new RPC_CLIENT(pebble_rpc);
    client->SetRouteFunction(cxx::bind(&Router::GetRoute, new_router, cxx::placeholders::_1));
    if (router != NULL) {
        *router = new_router;
    }
    return client;
}

}  // namespace pebble
#endif   //  _PEBBLE_CLIENT_PEBBLE_CLIENT_H_

