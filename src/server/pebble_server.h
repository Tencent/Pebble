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


#ifndef  _PEBBLE_EXTENSION_PEBBLE_SERVER_H_
#define  _PEBBLE_EXTENSION_PEBBLE_SERVER_H_

#include <vector>

#include "common/platform.h"
#include "framework/options.h"
#include "framework/pebble_rpc.h"
#include "framework/router.h"
#include "common/log.h"


//////////////////////////////////////////////////////////////////////////////////////
namespace pebble {

// 前置声明
class _PebbleBroadcastClient;
class BroadcastMgr;
class BroadcastRelayHandler;
class CoroutineSchedule;
class IEventHandler;
class INIReader;
class IProcessor;
class MessageExpireMonitor;
class MonitorCenter;
class Naming;
class NamingFactory;
class PebbleControlHandler;
class PebbleServer;
class RouterFactory;
class SessionMgr;
class Stat;
class StatManager;
class TaskMonitor;
class Timer;


//////////////////////////////////////////////////////////////////////////////////////

/// @brief 获取pebble版本号
const char* GetVersion();


//////////////////////////////////////////////////////////////////////////////////////

/// @brief 程序事件处理基类
class AppEventHandler {
public:
    AppEventHandler() {}
    virtual ~AppEventHandler() {}

    /// @brief Init事件回调处理
    /// @return <0 失败
    /// @return  0 成功
    virtual int32_t OnInit(PebbleServer* pebble_server) { return 0; }

    /// @brief Stop事件回调处理
    /// @return <0 不允许stop
    /// @return  0 允许stop
    virtual int32_t OnStop() { return 0; }

    /// @brief 在主循环中添加自己的事件处理
    /// @return <=0 无事件处理
    /// @return >0  处理的事件数
    virtual int32_t OnUpdate() { return 0; }

    /// @brief Reload事件回调处理
    /// @return <0 失败
    /// @return  0 成功
    virtual int32_t OnReload() { return 0; }

    /// @brief Idle事件回调处理
    /// @return <=0 Pebble sleep 10ms
    /// @return >0 不sleep
    virtual int32_t OnIdle() { return 0; }
};

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


/// @brief 控制命令处理函数，不能有阻塞操作
/// @param options 参数列表
/// @param ret_code 输出参数，处理结果
/// @param data 输出参数，返回给client的数据(当处理失败时也可返回错误描述)
/// @note 此回调函数在协程中执行，注意避免使用过大的栈上变量
typedef cxx::function<void(const std::vector<std::string>& options,
    int32_t* ret_code, std::string* data)> OnControlCommand;

//////////////////////////////////////////////////////////////////////////////////////


/// @brief PebbleApp的一个实现，本身为一个通用的Pebble server程序，聚合了Pebble的各功能模块
class PebbleServer {
public:
    PebbleServer();
    ~PebbleServer();

public:
    /// @brief 自动解析ini文件，并设置到options，如果调用需要在Init前调用
    /// @param file_name 配置文件 如 "../cfg/pebble.ini"
    /// @return 0 成功
    /// @return <0 失败
    int32_t LoadOptionsFromIni(const std::string& file_name);

    /// @brief 初始化接口，若使用ini配置，请先调用 @see LoadOptionsFromIni
    ///     若使用其他配置，请先设置Options @see GetOptions
    /// @param event_handler APP事件处理回调
    /// @return 0 成功
    /// @return <0 失败
    /// @note 若对pebble配置有修改，需要在Init调用前调用配置相关设置
    int32_t Init(AppEventHandler* event_handler = NULL);

    /// @brief （服务端）把一个句柄绑定到指定url
    /// @param url 指定url，形式类似：
    ///     "tbuspp://1000.unit_wx.query/inst0",
    ///     "tbus://11.0.0.1",
    ///     "http://127.0.0.1:8880[/service]"
    ///     "tcp://127.0.0.1:8880"
    ///     "udp://127.0.0.1:8880"
    /// @param register_name bind成功后是否同时注册名字，注意只对tbuspp地址生效
    /// @return >=0 表示成功
    /// @return <0 表示失败，错误码@see MessageErrorCode
    /// @note 原生tcp/udp与tbuspp协议只能二选一
    int64_t Bind(const std::string &url, bool register_name = false);

    /// @brief (客户端)连接到指定url
    /// @param url 指定url，形式类似：
    ///     "tbuspp://1000.unit_wx.query[/inst0]",
    ///     "tbus://11.0.0.1",
    ///     "http://127.0.0.1:8880[/service]"
    ///     "tcp://127.0.0.1:8880"
    ///     "udp://127.0.0.1:8880"
    /// @return >=0 表示成功
    /// @return <0 表示失败，错误码@see MessageErrorCode
    /// @note 原生tcp/udp与tbuspp协议只能二选一
    int64_t Connect(const std::string &url);

    /// @brief 启动服务，此调用会阻塞当前线程(循环处理事件，若无事件处理会Idle)
    void Serve();

    /// @brief 返回Pebble预定义的Processor
    /// @param processor_type @see ProcesserType
    /// @return 非NULL 成功
    /// @return NULL 失败
    IProcessor* GetProcessor(ProcessorType processor_type);

    /// @brief pebble支持processor嵌套，返回包含嵌套Processor的实例，如PipeProcessor(嵌套PebbleRpc Binary)
    /// @param processor_type @see ProcesserType
    /// @param nest_processor_type @see ProcesserType 嵌套的processor类型
    /// @return 非NULL 成功
    /// @return NULL 失败
    IProcessor* GetProcessor(ProcessorType processor_type, ProcessorType nest_processor_type);

    /// @brief 指定传输接入的业务处理程序
    /// @param handle 传输通道句柄
    /// @param processor: pebble支持的或用户自行实现的processor对象
    ///     Pebble内置的Processor由GetProcessor接口获取
    /// @return 0 成功
    /// @return <0 失败
    int32_t Attach(int64_t handle, IProcessor* processor);

    /// @brief 指定路由器服务的业务处理程序
    /// @param router 路由器
    /// @param processor: pebble支持的或用户自行实现的processor对象
    ///     Pebble内置的Processor由GetProcessor接口获取
    /// @return 0 成功
    /// @return <0 失败
    int32_t Attach(Router* router, IProcessor* processor);

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

    /// @brief 创建一个指定类型的RPC stub实例，只创建不回收，对象需要用户释放
    /// @param channel_name 广播频道的名字
    /// @param processor_type 使用的RPC类型
    /// @return RPC_CLIENT对象，为NULL时创建失败，新创建的对象需要用户释放
    template<class RPC_CLIENT>
    RPC_CLIENT* NewRpcClientByChannel(const std::string& channel_name,
        ProcessorType processor_type = kPEBBLE_RPC_BINARY);

    /// @brief 返回PebbleRpc实例，实际是GetProcessor的另一种易用法
    /// @param processor_type @see ProcesserType
    /// @return 非NULL 成功
    /// @return NULL 失败
    PebbleRpc* GetPebbleRpc(ProcessorType processor_type);

    /// @brief 返回内置名字服务对象
    /// @param naming_type 名字服务类型，@see NamingType
    /// @return 非NULL 成功
    /// @return NULL 失败
    Naming* GetNaming(NamingType naming_type = kNAMING_TBUSPP);

    /// @brief 返回一个Router对象(默认使用kNAMING_TBUSPP名字服务)，用户可使用此Router对象生成路由
    /// @param name 名字的绝对路径，使用Naming接口注册的名字 @see Naming::Register
    /// @param router_type 路由器类型，@see RouterType
    /// @return 非NULL 成功
    /// @return NULL 失败
    /// @note 如果Router使用其他名字服务，可以重新Init
    Router* GetRouter(const std::string& name, RouterType router_type = kROUTER_TBUSPP);

    /// @brief 返回内置协程调度器对象
    /// @return 非NULL 成功
    /// @return NULL 失败
    CoroutineSchedule* GetCoroutineSchedule() {
        return m_coroutine_schedule;
    }

    MonitorCenter* GetMonitorCenter() {
        return m_monitor_centor;
    }

    /// @brief 获取INI解析器，用户可以使用它解析INI文件
    /// @param config_file 配置文件 如 "../cfg/pebble.ini"
    /// @return 非NULL 成功
    /// @return NULL 失败
    INIReader* GetINIReader();

    /// @brief 获取顺序定时器实例
    /// @return 非NULL 成功
    /// @return NULL 失败
    Timer* GetTimer() {
        return m_timer;
    }

    /// @brief 获取会话管理实例
    /// @return 非NULL 成功
    /// @return NULL 失败
    SessionMgr* GetSessionMgr();

    /// @brief 获取统计实例
    /// @return 非NULL 成功
    /// @return NULL 失败
    Stat* GetStat();

    /// @brief 获取广播管理实例
    /// @return 非NULL 成功
    /// @return NULL 失败
    BroadcastMgr* GetBroadcastMgr();

    /// @brief 创建一个协程，并决定是否立即执行
    /// @param routine 协程执行入口函数
    /// @param start_immediately 默认立即执行，设置为false时等下一次update时执行
    /// @return 0 成功
    /// @return <0 失败
    int32_t MakeCoroutine(const cxx::function<void()>& routine, bool start_immediately = true);

    /// @brief 注册控制命令
    /// @param cmd 用户自定义命令
    /// @param desc 命令使用描述
    /// @param on_cmd 命令处理回调
    /// @return 0 成功
    /// @return <0 失败
    /// @note 控制命令不能有阻塞操作，建议只实现简单查询功能
    int32_t RegisterControlCommand(const OnControlCommand& on_cmd,
        const std::string& cmd, const std::string& desc);

    /// @brief 返回Pebble的配置参数，用户可按需修改
    /// @return Pebble的配置参数对象
    /// @note 参数修改在init和reload时生效
    Options* GetOptions() {
        return &m_options;
    }

    int32_t Close(int64_t handle);

public:
    // 下面接口由使用pebble的框架来调用
    int32_t Update();

    int32_t Stop();

    int32_t Reload();

    void Idle();

private:
    int32_t ProcessMessage();

    void InitLog();

    int32_t InitCoSchedule();

    void InitMonitor();

    int32_t InitStat();

    int32_t InitTimer();

    int32_t InitControlService();

    int32_t OnStatTimeout();

    void OnRouterAddressChanged(const std::vector<int64_t>& handles, IProcessor* processor);

    void StatCpu(Stat* stat);

    void StatMemory(Stat* stat);

    void StatCoroutine(Stat* stat);

    void StatProcessorResource(Stat* stat);

    void OnControlReload(const std::vector<std::string>& options, int32_t* ret_code, std::string* data);

    void OnControlPrint(const std::vector<std::string>& options, int32_t* ret_code, std::string* data);

    void OnControlLog(const std::vector<std::string>& options, int32_t* ret_code, std::string* data);

private:
    Options            m_options;
    CoroutineSchedule* m_coroutine_schedule;
    INIReader*         m_ini_reader;
    MonitorCenter*     m_monitor_centor;
    TaskMonitor*       m_task_monitor;
    MessageExpireMonitor* m_message_expire_monitor;
    Naming*            m_naming_array[kNAMING_BUTT];
    IProcessor*        m_processor_array[kPROCESSOR_TYPE_BUTT];
    IEventHandler*     m_rpc_event_handler;
    IEventHandler*     m_broadcast_event_handler;
    StatManager*       m_stat_manager;
    Timer*             m_timer;
    int64_t            m_last_pid_cpu_use;
    int64_t            m_last_total_cpu_use;
    uint32_t           m_stat_timer_ms; // 资源使用采样定时器，供统计用
    AppEventHandler*   m_event_handler;
    SessionMgr*        m_session_mgr;
    BroadcastMgr*      m_broadcast_mgr;
    BroadcastRelayHandler* m_broadcast_relay_handler;
    _PebbleBroadcastClient* m_broadcast_relay_client;
    PebbleControlHandler* m_control_handler;
    cxx::unordered_map<int64_t, IProcessor*> m_processor_map;
    cxx::unordered_map<std::string, Router*> m_router_map;
    std::string m_ini_file_name;
    uint32_t    m_is_overload;
};

///////////////////////////////////////////////////////////////////////////////////////

template<class RPC_CLIENT>
RPC_CLIENT* PebbleServer::NewRpcClientByAddress(const std::string& service_address,
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
RPC_CLIENT* PebbleServer::NewRpcClientByName(const std::string& service_name,
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

template<class RPC_CLIENT>
RPC_CLIENT* PebbleServer::NewRpcClientByChannel(const std::string& channel_name,
    ProcessorType processor_type) {
    if (channel_name.empty()) {
        PLOG_ERROR("invalid channel_name");
        return NULL;
    }
    PebbleRpc* pebble_rpc = GetPebbleRpc(processor_type);
    if (pebble_rpc == NULL) {
        PLOG_ERROR("GetPebbleRpc failed, processor_type: %d", processor_type);
        return NULL;
    }
    RPC_CLIENT* client = new RPC_CLIENT(pebble_rpc);
    client->SetBroadcast(channel_name);
    return client;
}

}  // namespace pebble
#endif   //  _PEBBLE_EXTENSION_PEBBLE_SERVER_H_
