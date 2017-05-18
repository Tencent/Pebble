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

#include <algorithm>
#include <iostream>
#include <sstream>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "common/coroutine.h"
#include "common/cpu.h"
#include "common/ini_reader.h"
#include "common/log.h"
#include "common/memory.h"
#include "common/string_utility.h"
#include "common/time_utility.h"
#include "common/timer.h"
#include "framework/broadcast_mgr.h"
#include "framework/broadcast_mgr.inh"
#include "framework/event_handler.inh"
#include "framework/message.h"
#include "framework/monitor.h"
#include "framework/pebble_rpc.h"
#include "framework/register_error.h"
#include "framework/session.h"
#include "framework/stat.h"
#include "framework/stat_manager.h"
#include "pebble_version.inh"
#include "server/pebble_server.h"
#include "src/server/control__PebbleControl.h"
#include "version.inh"


namespace pebble {

static const char* prefix_tbuspp = "tbuspp://";

const char* GetVersion() {
    static char version[256] = {0};

    snprintf(version, sizeof(version), "Pebble : %s %s %s",
        PebbleVersion::GetVersion().c_str(), __TIME__, __DATE__);

    return version;
}


static struct {
    int32_t _stop;
    int32_t _reload;
} g_app_events = { 0, 0 };

void pebble_on_stop(int32_t signal) {
    g_app_events._stop = signal;
}

void pebble_on_reload(int32_t signal) {
    g_app_events._reload = 1;
}

// 独立的控制命令RPC服务处理类，只是纯通道，不关心具体的命令，所有的命令处理都有外部注册
class PebbleControlHandler : public _PebbleControlCobSvIf {
public:
    PebbleControlHandler() : m_index(0), m_history_size(256), m_next_item(0) {}
    ~PebbleControlHandler() {}
    int32_t Init();
    int32_t RegisterCommand(const OnControlCommand& on_cmd, const std::string& cmd,
        const std::string& desc, bool internal = false);
    virtual void RunCommand(const ControlRequest& req,
        cxx::function<void(int32_t ret_code, const ControlResponse& response)>& rsp);

private:
    void OnControlHelp(const std::vector<std::string>& options, int32_t* ret_code, std::string* data);
    void OnControlHistory(const std::vector<std::string>& options, int32_t* ret_code, std::string* data);

private:
    struct CmdInfo {
        bool _internal;         // 是否内置命令
        std::string _cmd;       // 命令
        std::string _desc;      // 命令帮助
        OnControlCommand _proc; // 回调函数
        CmdInfo() : _internal(false) {}
    };
    cxx::unordered_map<std::string, CmdInfo> m_cmds;

    struct HistoryItem {
        uint64_t    _index;
        int32_t     _result;
        std::string _time;
        std::string _cmd;
        HistoryItem() : _index(0), _result(0) {}
    };
    int64_t m_index;            // 执行命令的序号
    int32_t m_history_size;     // 存储历史记录的大小
    uint32_t m_next_item;       // 下一个item的位置
    std::vector<HistoryItem> m_historys;
};

int32_t PebbleControlHandler::RegisterCommand(const OnControlCommand& on_cmd,
    const std::string& cmd, const std::string& desc, bool internal) {
    // 注册和查找时统一大小写
    std::string tmpcmd(cmd);
    StringUtility::ToLower(&tmpcmd);
    cxx::unordered_map<std::string, CmdInfo>::iterator it = m_cmds.find(tmpcmd);
    if (it != m_cmds.end()) {
        PLOG_ERROR("cmd %s is already registered.", cmd.c_str());
        return -1;
    }

    CmdInfo& cmd_info   = m_cmds[tmpcmd];
    cmd_info._internal  = internal;
    cmd_info._cmd       = cmd;
    cmd_info._desc      = desc;
    cmd_info._proc      = on_cmd;
    return 0;
}

int32_t PebbleControlHandler::Init() {
    using namespace cxx::placeholders;
    int32_t ret = RegisterCommand(
        cxx::bind(&PebbleControlHandler::OnControlHelp, this, _1, _2, _3), "help",
        "help               # online help for control command service, no option",
        true);
    RETURN_IF_ERROR(ret != 0, ret, "register help failed.");

    ret = RegisterCommand(
        cxx::bind(&PebbleControlHandler::OnControlHistory, this, _1, _2, _3), "history",
        "history            # show control command executed history, max 256 records\n"
        "                   # format  : history n\n"
        "                   # example : history 10",
        true);
    RETURN_IF_ERROR(ret != 0, ret, "register help failed.");

    m_historys.resize(m_history_size);

    return 0;
}

void PebbleControlHandler::RunCommand(const ControlRequest& req,
    cxx::function<void(int32_t ret_code, const ControlResponse& response)>& rsp) {

    ControlResponse response;
    // 注册和查找时统一大小写
    std::string tmpcmd(req.command);
    StringUtility::ToLower(&tmpcmd);
    cxx::unordered_map<std::string, CmdInfo>::iterator it = m_cmds.find(tmpcmd);
    if (m_cmds.end() == it) {
        response.ret_code = -1;
        response.data = "unsupport command";
    } else {
        std::string tmpdata;
        it->second._proc(req.options, &response.ret_code, &tmpdata);
        response.data.assign("\n");
        response.data.append(tmpdata);
    }

    rsp(0, response);

    PLOG_INFO("Control Command : %s ret=%d, data=%s",
        req.command.c_str(), response.ret_code, response.data.c_str());

    // 记录执行历史
    HistoryItem& history_item = m_historys[m_next_item];
    m_next_item = (m_next_item + 1) % m_history_size;
    history_item._index = m_index++;
    history_item._time  = TimeUtility::GetStringTime();
    history_item._cmd   = req.command;
    if (!req.options.empty()) {
        history_item._cmd.append(" ");
        for (std::vector<std::string>::const_iterator it = req.options.begin();
            it != req.options.end(); ++it) {
            if (it != req.options.begin()) {
                history_item._cmd.append(", ");
            }
            history_item._cmd.append(*it);
        }
    }
    history_item._result = response.ret_code;
}

void PebbleControlHandler::OnControlHelp(const std::vector<std::string>& options,
    int32_t* ret_code, std::string* data) {
    std::ostringstream oss_internal;
    std::ostringstream oss_user;
    oss_internal << "Pebble Control Command:\n\n";
    oss_user << "User Control Command:\n\n";
    for (cxx::unordered_map<std::string, CmdInfo>::iterator it = m_cmds.begin();
        it != m_cmds.end(); ++it) {
        CmdInfo& cmdinfo = it->second;
        if (cmdinfo._internal) {
            oss_internal << cmdinfo._desc << "\n\n";
        } else {
            oss_user << cmdinfo._desc << "\n\n";
        }
    }
    oss_internal << "\n";

    *ret_code = 0;
    data->assign(oss_internal.str());
    data->append(oss_user.str());
}

void PebbleControlHandler::OnControlHistory(const std::vector<std::string>& options,
    int32_t* ret_code, std::string* data) {
    int32_t cnt = std::min(static_cast<int64_t>(m_history_size), m_index);
    if (!options.empty()) {
        int32_t option = atoi(options.front().c_str());
        if (option > 0) {
            cnt = std::min(cnt, option);
        }
    }

    int32_t start = (m_history_size + m_next_item - cnt) % m_history_size;
    std::ostringstream oss;
    oss.fill(' ');
    oss.setf(std::ios_base::left);
    std::string fail("Fail");
    std::string success("Success");
    for (int i = 0; i < cnt; i++) {
        HistoryItem& item = m_historys[start];
        oss.width(10);
        oss << item._index;
        oss.width(23);
        oss << item._time;
        oss.width(11);
        oss << (item._result != 0 ? fail : success);
        oss << item._cmd;
        oss << "\n";
        start = (start + 1) % m_history_size;
    }

    std::ostringstream head;
    head.fill(' ');
    head.setf(std::ios_base::left);
    head.width(10);
    head << "Index";
    head.width(23);
    head << "Time";
    head.width(11);
    head << "Result";
    head << "Command\n";

    *ret_code = 0;
    data->assign(head.str());
    data->append(oss.str());
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////

PebbleServer::PebbleServer() {
    m_coroutine_schedule = NULL;
    m_ini_reader         = NULL;
    m_monitor_centor     = NULL;
    m_task_monitor       = NULL;
    m_message_expire_monitor = NULL;
    m_stat_manager       = NULL;
    m_timer              = NULL;
    m_stat_timer_ms      = 1000;
    m_rpc_event_handler  = NULL;
    m_last_pid_cpu_use   = 0;
    m_last_total_cpu_use = 0;
    m_event_handler      = NULL;
    m_session_mgr        = NULL;
    m_broadcast_mgr      = NULL;
    m_broadcast_relay_handler = NULL;
    m_broadcast_relay_client  = NULL;
    m_is_overload             = kNO_OVERLOAD;
    m_broadcast_event_handler = NULL;
    m_control_handler         = NULL;

    for (int32_t i = 0; i < kNAMING_BUTT; ++i) {
        m_naming_array[i] = NULL;
    }

    for (int32_t i = 0; i < kPROCESSOR_TYPE_BUTT; ++i) {
        m_processor_array[i] = NULL;
    }
}

PebbleServer::~PebbleServer() {
    // delete的原则: 如果出现重复delete说明逻辑实现有问题，通过crash暴露出来，赋NULL可能掩盖问题
    for (cxx::unordered_map<std::string, Router*>::iterator it = m_router_map.begin();
        it != m_router_map.end(); ++it) {
        delete it->second;
    }

    for (int32_t i = 0; i < kNAMING_BUTT; ++i) {
        delete m_naming_array[i];
    }

    for (int32_t i = 0; i < kPROCESSOR_TYPE_BUTT; ++i) {
        delete m_processor_array[i];
    }

    delete m_rpc_event_handler;
    delete m_monitor_centor;
    delete m_task_monitor;
    delete m_message_expire_monitor;
    delete m_stat_manager;
    delete m_ini_reader;
    delete m_coroutine_schedule;
    delete m_timer;
    delete m_session_mgr;
    delete m_broadcast_mgr;
    delete m_broadcast_relay_handler;
    delete m_broadcast_relay_client;
    delete m_broadcast_event_handler;
    delete m_control_handler;
}

int32_t PebbleServer::Init(AppEventHandler* event_handler) {
    // 具体模块初始化失败的详细信息应该在具体的init中输出，这里统一返回
    #define CHECK_RETURN(ret) \
        if (ret) { \
            return -1; \
        }

    m_event_handler = event_handler;

    RegisterErrorString();

    InitLog();

    PLOG_INFO("%s", m_options.ToString().c_str());

    int32_t ret = InitTimer();
    CHECK_RETURN(ret);

    ret = InitCoSchedule();
    CHECK_RETURN(ret);

    ret = InitStat();
    CHECK_RETURN(ret);

    ret = Message::Init();
    CHECK_RETURN(ret);

    InitMonitor();

    m_last_pid_cpu_use   = GetCurCpuTime();
    m_last_total_cpu_use = GetTotalCpuTime();

    if (m_event_handler) {
        ret = m_event_handler->OnInit(this);
        CHECK_RETURN(ret);
    }

    ret = InitControlService();
    CHECK_RETURN(ret);

    signal(SIGPIPE, SIG_IGN);

    return 0;
}

int64_t PebbleServer::Bind(const std::string &url, bool register_name) {
    int64_t handle = Message::Bind(url);
    if (handle < 0) {
        PLOG_ERROR("bind %s failed(%ld:%s)", url.c_str(), handle, Message::GetLastError());
        return handle;
    }
    // bind成功后注册名字只对tbuspp有效，目前tbuspp名字和传输耦合较紧，解耦后可适用其他传输协议
    if (register_name && StringUtility::StartsWith(url, prefix_tbuspp)) {
        Naming* naming = GetNaming();
        if (naming == NULL) {
            Message::Close(handle);
            PLOG_ERROR("bind %s success, but get naming failed", url.c_str());
            return -1;
        }
        // url 格式为 tbuspp://appid.path.name/instanceid
        std::string service_name(url);
        StringUtility::StripPrefix(&service_name, prefix_tbuspp);   // 去掉 "tbuspp://"
        service_name.resize(service_name.find_last_of('/'));        // 去掉 "/instanceid"
        StringUtility::string_replace(".", "/", &service_name);     // tbuspp名字用'/'分隔，名字前要有'/'
        int32_t ret = naming->Register("/" + service_name, url, m_options._app_instance_id);
        if (ret != 0) {
            Message::Close(handle);
            PLOG_ERROR("register name %s url %s failed(%d:%s)", service_name.c_str(), url.c_str(),
                ret, naming->GetLastError());
            return -1;
        }
    }
    return handle;
}

int64_t PebbleServer::Connect(const std::string &url) {
    int64_t handle = Message::Connect(url);
    PLOG_IF_ERROR(handle < 0, "connect %s failed(%ld:%s)", url.c_str(), handle, Message::GetLastError());
    return handle;
}

int32_t PebbleServer::Close(int64_t handle) {
    int32_t ret = Message::Close(handle);
    PLOG_IF_ERROR(ret != 0, "close %ld failed(%s)", handle, Message::GetLastError());
    return ret;
}

void PebbleServer::Serve() {
    // 运行在服务模式时，接管USR1/USR2信号
    signal(SIGUSR1, pebble_on_stop);
    signal(SIGUSR2, pebble_on_reload);

    do {
        if (g_app_events._stop) {
            if (Stop() == 0) {
                g_app_events._stop = 0;
                break;
            }
        }

        if (g_app_events._reload) {
            g_app_events._reload = 0;
            Reload();
        }

        if (Update() <= 0) {
            Idle();
        }
    } while (true);
}

int32_t PebbleServer::Attach(int64_t handle, IProcessor* processor) {
    if (NULL == processor) {
        PLOG_ERROR("param processor is NULL");
        return -1;
    }

    if (m_processor_map.insert({handle, processor}).second == false) {
        PLOG_ERROR("handle %ld is already attach to Processor %p", handle, processor);
        return -1;
    }
    return 0;
}

int32_t PebbleServer::Attach(Router* router, IProcessor* processor) {
    if (NULL == router || NULL == processor) {
        PLOG_ERROR("param error. &router = %p &processor = %p", router, processor);
        return -1;
    }

    router->SetOnAddressChanged(cxx::bind(&PebbleServer::OnRouterAddressChanged, this,
        cxx::placeholders::_1, processor));
    return 0;
}

IProcessor* PebbleServer::GetProcessor(ProcessorType processor_type) {
    switch (processor_type) {
        case kPEBBLE_RPC_BINARY:
        case kPEBBLE_RPC_JSON:
        case kPEBBLE_RPC_PROTOBUF:
            return GetPebbleRpc(processor_type);

        case kPEBBLE_PIPE:
            PLOG_ERROR("pipe processor must spesify nest processor, please call "
                "GetProcessor(ProcessorType processor_type, ProcessorType nest_processor_type)");
            break;

        default:
            PLOG_ERROR("unsupport processor type %d", processor_type);
            break;
    }
    return NULL;
}

IProcessor* PebbleServer::GetProcessor(ProcessorType processor_type,
    ProcessorType nest_processor_type) {
    cxx::shared_ptr<ProcessorFactory> factory;
    switch (processor_type) {
        case kPEBBLE_RPC_BINARY:
        case kPEBBLE_RPC_JSON:
        case kPEBBLE_RPC_PROTOBUF:
            PLOG_ERROR("processor type %d unsupport nest, please call "
                "GetProcessor(ProcessorType processor_type)", processor_type);
            break;

        case kPEBBLE_PIPE:
            if (m_processor_array[processor_type] != NULL) {
                return m_processor_array[processor_type];
            }
            factory = GetProcessorFactory(kPEBBLE_PIPE);
            if (!factory) {
                PLOG_ERROR("pipe processor factory not installed.");
                return NULL;
            }
            m_processor_array[processor_type] =
                factory->GetProcessor(GetTimer(), GetProcessor(nest_processor_type));
            return m_processor_array[processor_type];

        default:
            PLOG_ERROR("unsupport processor type %d", processor_type);
            break;
    }
    return NULL;
}

PebbleRpc* PebbleServer::GetPebbleRpc(ProcessorType processor_type) {
    if (processor_type < kPEBBLE_RPC_BINARY || processor_type > kPEBBLE_RPC_PROTOBUF) {
        PLOG_ERROR("param processor_type invalid(%d)", processor_type);
        return NULL;
    }

    if (m_processor_array[processor_type] != NULL) {
        return dynamic_cast<PebbleRpc*>(m_processor_array[processor_type]);
    }

    if (!m_rpc_event_handler) {
        m_rpc_event_handler = new RpcEventHandler();
        static_cast<RpcEventHandler*>(m_rpc_event_handler)->Init(m_stat_manager);
    }

    CodeType rpc_code_type = kCODE_BUTT;
    switch (processor_type) {
        case kPEBBLE_RPC_BINARY:
            rpc_code_type = kCODE_BINARY;
            break;

        case kPEBBLE_RPC_JSON:
            rpc_code_type = kCODE_JSON;
            break;

        case kPEBBLE_RPC_PROTOBUF:
            rpc_code_type = kCODE_PB;
            break;

        default:
            PLOG_FATAL("unsupport processor type %d", processor_type);
            return NULL;
    }

    PebbleRpc* rpc_instance = new PebbleRpc(rpc_code_type, m_coroutine_schedule);
    rpc_instance->SetSendFunction(Message::Send, Message::SendV);
    rpc_instance->SetEventHandler(m_rpc_event_handler);
    m_processor_array[processor_type] = rpc_instance;

    return rpc_instance;
}

Naming* PebbleServer::GetNaming(NamingType naming_type) {
    if (m_naming_array[naming_type] != NULL) {
        return m_naming_array[naming_type];
    }

    cxx::shared_ptr<NamingFactory> factory = GetNamingFactory(naming_type);
    if (!factory) {
        PLOG_ERROR("unsupport naming_type %d", naming_type);
        return NULL;
    }

    m_naming_array[naming_type] = factory->GetNaming(
        m_options._bc_zk_host, m_options._bc_zk_timeout_ms);

    return m_naming_array[naming_type];
}

Router* PebbleServer::GetRouter(const std::string& name, RouterType router_type) {
    if (name.empty()) {
        PLOG_ERROR("name is empty");
        return NULL;
    }

    cxx::unordered_map<std::string, Router*>::iterator it = m_router_map.find(name);
    if (it != m_router_map.end()) {
        return it->second;
    }

    cxx::shared_ptr<RouterFactory> factory = GetRouterFactory(router_type);
    if (!factory) {
        PLOG_ERROR("unsupport router_type %d", router_type);
        return NULL;
    }

    Router* router = factory->GetRouter(name);
    // 这里默认使用tbuspp名字服务，用户可以重新Init设置其他名字服务
    int32_t ret = router->Init(GetNaming());
    if (ret != 0) {
        PLOG_ERROR("router %s init failed(%d)", name.c_str(), ret);
    }

    m_router_map[name] = router;
    return router;
}

int32_t PebbleServer::Stop() {
    if (m_event_handler) {
        return m_event_handler->OnStop();
    }

    return 0;
}

int32_t PebbleServer::Update() {
    int32_t num = 0;

    int64_t old = TimeUtility::GetCurrentMS();

    for (uint32_t i = 0; i < m_options._max_msg_num_per_loop; ++i) {
        if (ProcessMessage() <= 0) {
            break;
        }
        num++;
    }

    for (int32_t i = 0; i < kNAMING_BUTT; ++i) {
        if (m_naming_array[i]) {
            num += m_naming_array[i]->Update();
        }
    }

    cxx::unordered_map<int64_t, IProcessor*>::iterator it = m_processor_map.begin();
    for (; it != m_processor_map.end(); ++it) {
        num += it->second->Update();
    }

    if (m_timer) {
        num += m_timer->Update();
    }

    if (m_session_mgr) {
        num += m_session_mgr->CheckTimeout();
    }

    if (m_event_handler) {
        num += m_event_handler->OnUpdate();
    }

    if (m_broadcast_mgr) {
        num += m_broadcast_mgr->Update(m_is_overload);
    }

    if (m_stat_manager) {
        num += m_stat_manager->Update();
        m_stat_manager->GetStat()->AddResourceItem("_loop", TimeUtility::GetCurrentMS() - old);
    }

    return num;
}

int32_t PebbleServer::Reload() {
    if (!m_ini_file_name.empty()) {
        if (LoadOptionsFromIni(m_ini_file_name) != 0) {
            return -1;
        }
    }

    if (m_event_handler) {
        int32_t ret = m_event_handler->OnReload();
        if (ret != 0) {
            PLOG_ERROR("user reload failed(%d)", ret);
            return -1;
        }
    }

    PLOG_INFO("%s", m_options.ToString().c_str());

    // log
    InitLog();

    // stat
    m_stat_manager->SetReportCycle(m_options._stat_report_cycle_s);
    m_stat_manager->SetGdataParameter(m_options._stat_report_to_gdata,
        m_options._gdata_id, m_options._gdata_log_id);

    // flow control
    m_task_monitor->SetTaskThreshold(m_options._task_threshold);
    m_message_expire_monitor->SetExpireThreshold(m_options._message_expire_ms);

    return 0;
}

void PebbleServer::Idle() {
    int32_t ret = 0;
    if (m_event_handler) {
        ret = m_event_handler->OnIdle();
    }
    if (ret > 0) {
        return;
    }

    Log::Instance().Flush();

    usleep(m_options._idle_us);
}

int32_t PebbleServer::ProcessMessage() {
    int64_t handle = -1;
    int32_t event  = 0;
    int32_t ret = Message::Poll(&handle, &event, 0);
    if (ret != 0) {
        return 0;
    }

    const uint8_t* msg = NULL;
    uint32_t msg_len   = 0;
    MsgExternInfo msg_info;
    ret = Message::Peek(handle, &msg, &msg_len, &msg_info);
    do {
        if (kMESSAGE_RECV_EMPTY == ret) {
            break;
        }
        if (ret != 0) {
            PLOG_ERROR("peek failed(%d:%s)", ret, Message::GetLastError());
            break;
        }

        cxx::unordered_map<int64_t, IProcessor*>::iterator it = m_processor_map.find(msg_info._self_handle);
        if (m_processor_map.end() == it) {
            Message::Pop(handle);
            PLOG_ERROR("handle(%ld) not attach a processor remote(%ld)", msg_info._self_handle, msg_info._remote_handle);
            break;
        }

        m_is_overload = kNO_OVERLOAD;
        if (m_options._enable_flow_control) {
            m_message_expire_monitor->OnMessage(msg_info._msg_arrived_ms);
            m_task_monitor->SetTaskNum(m_coroutine_schedule->Size()); // 内部实现暂使用协程数
            m_is_overload = m_monitor_centor->IsOverLoad();
        }
        it->second->OnMessage(msg_info._remote_handle, msg, msg_len, &msg_info, m_is_overload);
        Message::Pop(handle);
    } while (0);

    return 1;
}

void PebbleServer::InitLog() {
    Log::Instance().SetOutputDevice(m_options._log_device);
    Log::Instance().SetLogPriority(m_options._log_priority);
    Log::Instance().SetMaxFileSize(m_options._log_file_size_MB);
    Log::Instance().SetMaxRollNum(m_options._log_roll_num);
    Log::Instance().SetFilePath(m_options._log_path);

    // Log::EnableCrashRecord();
}

int32_t PebbleServer::InitCoSchedule() {
    if (m_coroutine_schedule) {
        return 0;
    }

    m_coroutine_schedule = new CoroutineSchedule();
    int32_t ret = m_coroutine_schedule->Init(GetTimer(), m_options._co_stack_size_bytes);
    if (ret != 0) {
        delete m_coroutine_schedule;
        m_coroutine_schedule = NULL;
        PLOG_ERROR("co schedule init failed(%d)", ret);
        return -1;
    }

    return 0;
}

int32_t PebbleServer::InitStat() {
    if (!m_stat_manager) {
        m_stat_manager = new StatManager();
    }

    m_stat_manager->SetReportCycle(m_options._stat_report_cycle_s);
    m_stat_manager->SetGdataParameter(m_options._stat_report_to_gdata,
        m_options._gdata_id, m_options._gdata_log_id);

    return m_stat_manager->Init(m_options._app_id, m_options._app_unit_id,
        m_options._app_program_id, m_options._app_instance_id, m_options._gdata_log_path);
}

void PebbleServer::InitMonitor() {
    if (!m_task_monitor) {
        m_task_monitor = new TaskMonitor();
    }
    if (!m_message_expire_monitor) {
        m_message_expire_monitor = new MessageExpireMonitor();
    }
    if (!m_monitor_centor) {
        m_monitor_centor = new MonitorCenter();
    }

    m_task_monitor->SetTaskThreshold(m_options._task_threshold);
    m_message_expire_monitor->SetExpireThreshold(m_options._message_expire_ms);

    m_monitor_centor->Clear();
    m_monitor_centor->AddMonitor(m_task_monitor);
    m_monitor_centor->AddMonitor(m_message_expire_monitor);
}

int32_t PebbleServer::InitTimer() {
    if (!m_timer) {
        m_timer = new SequenceTimer();
    }

    TimeoutCallback on_stat_timeout = cxx::bind(&PebbleServer::OnStatTimeout, this);
    int64_t ret = m_timer->StartTimer(m_stat_timer_ms, on_stat_timeout);
    if (ret < 0) {
        PLOG_ERROR("start stat timer failed(%d:%s)", ret, m_timer->GetLastError());
        return -1;
    }

    return 0;
}

INIReader* PebbleServer::GetINIReader() {
    if (!m_ini_reader) {
        m_ini_reader = new INIReader();
    }

    return m_ini_reader;
}

int32_t PebbleServer::LoadOptionsFromIni(const std::string& file_name) {
    INIReader* ini_reader = GetINIReader();
    ini_reader->Clear();
    int ret = ini_reader->Parse(file_name);
    if (ret != 0) {
        // 只要解析失败，不管是否部分成功都认为配置不可靠，不再自动给配置赋值
        PLOG_ERROR("parse ini file %s failed(%d)", file_name.c_str(), ret);
        return -1;
    }

    m_ini_file_name = file_name;

    // app
    m_options._app_id = ini_reader->GetInt64(kSectionApp, kAppId, m_options._app_id);
    m_options._app_key = ini_reader->Get(kSectionApp, kAppKey, m_options._app_key);
    m_options._app_instance_id = ini_reader->GetInt32(kSectionApp, kAppInstanceId, m_options._app_instance_id);
    m_options._app_unit_id = ini_reader->GetInt32(kSectionApp, kAppUnitId, m_options._app_unit_id);
    m_options._app_program_id = ini_reader->GetInt32(kSectionApp, kAppProgramId, m_options._app_program_id);
    m_options._app_ctrl_cmd_addr = ini_reader->Get(kSectionApp, kAppCtrlCmdAddr, m_options._app_ctrl_cmd_addr);

    // coroutine
    m_options._co_stack_size_bytes = ini_reader->GetUInt32(kSectionCoroutine, kCoStackSize, m_options._co_stack_size_bytes);

    // log
    m_options._log_device = ini_reader->Get(kSectionLog, kLogDevice, m_options._log_device);
    m_options._log_priority = ini_reader->Get(kSectionLog, kLogPriority, m_options._log_priority);
    m_options._log_file_size_MB = ini_reader->GetUInt32(kSectionLog, kLogFileSize, m_options._log_file_size_MB);
    m_options._log_roll_num = ini_reader->GetUInt32(kSectionLog, kLogRollNum, m_options._log_roll_num);
    m_options._log_path = ini_reader->Get(kSectionLog, kLogPath, m_options._log_path);

    // stat
    m_options._stat_report_cycle_s = ini_reader->GetUInt32(kSectionStat, kStatReportCycleS, m_options._stat_report_cycle_s);
    m_options._stat_report_to_gdata = ini_reader->GetUInt32(kSectionStat, kStatReportToGdata, m_options._stat_report_to_gdata);
    m_options._gdata_id = ini_reader->GetInt32(kSectionStat, kGdataId, m_options._gdata_id);
    m_options._gdata_log_id = ini_reader->GetInt32(kSectionStat, kGdataLogId, m_options._gdata_log_id);
    m_options._gdata_log_path = ini_reader->Get(kSectionStat, kGdataLogPath, m_options._gdata_log_path);

    // flow control
    m_options._enable_flow_control = ini_reader->GetBoolean(kSectionFlowControl, kEnableFlowControl, m_options._enable_flow_control);
    m_options._max_msg_num_per_loop = ini_reader->GetUInt32(kSectionFlowControl, kMaxMsgNumPerLoop, m_options._max_msg_num_per_loop);
    m_options._task_threshold = ini_reader->GetUInt32(kSectionFlowControl, kTaskThreshold, m_options._task_threshold);
    m_options._message_expire_ms = ini_reader->GetUInt32(kSectionFlowControl, kMessageExpireMs, m_options._message_expire_ms);
    m_options._idle_us = ini_reader->GetUInt32(kSectionFlowControl, kIdleUs, m_options._idle_us);

    // broadcast
    m_options._bc_relay_address = ini_reader->Get(kSectionBroadcast, kBcRelayAddress, m_options._bc_relay_address);
    m_options._bc_zk_host = ini_reader->Get(kSectionBroadcast, kBcZkHost, m_options._bc_zk_host);
    m_options._bc_zk_timeout_ms = ini_reader->GetUInt32(kSectionBroadcast, kBcZkTimeoutMs, m_options._bc_zk_timeout_ms);

    return 0;
}

int32_t PebbleServer::OnStatTimeout() {
    Stat* stat = m_stat_manager->GetStat();
    StatCpu(stat);
    StatMemory(stat);
    StatCoroutine(stat);
    StatProcessorResource(stat);

    return m_stat_timer_ms;
}

void PebbleServer::OnRouterAddressChanged(const std::vector<int64_t>& handles,
    IProcessor* processor) {

    for (std::vector<int64_t>::const_iterator it = handles.begin(); it != handles.end(); ++it) {
        Attach(*it, processor);
    }
}

void PebbleServer::StatCpu(Stat* stat) {
    int64_t now_pid_cpu_use   = GetCurCpuTime();
    int64_t now_total_cpu_use = GetTotalCpuTime();
    if (now_pid_cpu_use < 0 || now_total_cpu_use < 0) {
        return;
    }
    float cpu_use = CalculateCurCpuUseage(m_last_pid_cpu_use, now_pid_cpu_use,
        m_last_total_cpu_use, now_total_cpu_use);

    m_last_pid_cpu_use   = now_pid_cpu_use;
    m_last_total_cpu_use = now_total_cpu_use;

    stat->AddResourceItem("_cpu", cpu_use);
}

void PebbleServer::StatMemory(Stat* stat) {
    int32_t vm  = 0;
    int32_t rss = 0;
    if (GetCurMemoryUsage(&vm, &rss) != 0) {
        return;
    }
    stat->AddResourceItem("_vm(M)", vm / 1024);
    stat->AddResourceItem("_rss(M)", rss / 1024);
}

void PebbleServer::StatCoroutine(Stat* stat) {
    stat->AddResourceItem("_coroutine", m_coroutine_schedule->Size());
}

void PebbleServer::StatProcessorResource(Stat* stat) {
    uint32_t task_num = 0;
    cxx::unordered_map<std::string, int64_t> resource;
    cxx::unordered_map<std::string, int64_t>::iterator it;

    for (int32_t i = 0; i < kPROCESSOR_TYPE_BUTT; ++i) {
        if (NULL == m_processor_array[i]) {
            continue;
        }

        // 统计任务数，目前内置Processor任务数实际为协程数
        task_num += m_processor_array[i]->GetUnFinishedTaskNum();

        // 统计Processor使用动态资源情况
        resource.clear();
        m_processor_array[i]->GetResourceUsed(&resource);
        for (it = resource.begin(); it != resource.end(); ++it) {
            stat->AddResourceItem(it->first, it->second);
        }
    }

    stat->AddResourceItem("_task", task_num);
}

SessionMgr* PebbleServer::GetSessionMgr() {
    if (!m_session_mgr) {
        m_session_mgr = new SessionMgr();
    }
    return m_session_mgr;
}

Stat* PebbleServer::GetStat() {
    if (m_stat_manager) {
        return m_stat_manager->GetStat();
    }
    return NULL;
}

BroadcastMgr* PebbleServer::GetBroadcastMgr() {
    if (m_broadcast_mgr) {
        return m_broadcast_mgr;
    }

    int32_t ret    = -1;
    int64_t handle = -1;
    Naming* naming = NULL;
    PebbleRpc* rpc = NULL;
    m_broadcast_mgr = new BroadcastMgr();

    handle = m_broadcast_mgr->BindRelayAddress(m_options._bc_relay_address);
    if (handle < 0) {
        goto error;
    }

    naming = GetNaming(kNAMING_ZOOKEEPER);
    if (naming == NULL) {
        goto error;
    }

    ret = m_broadcast_mgr->Init(m_options._app_id, m_options._app_key, naming);
    if (ret != 0) {
        goto error;
    }

    // server间转发的广播消息使用binary编码
    rpc = GetPebbleRpc(kPEBBLE_RPC_BINARY);
    ret = Attach(handle, rpc);
    if (ret != 0) {
        goto error;
    }

    m_broadcast_relay_handler = new BroadcastRelayHandler(m_broadcast_mgr);
    ret = rpc->AddService(m_broadcast_relay_handler);
    if (ret != 0) {
        goto error;
    }

    m_broadcast_relay_client = new _PebbleBroadcastClient(rpc);
    m_broadcast_mgr->SetRelayClient(m_broadcast_relay_client);
    m_broadcast_event_handler = new BroadcastEventHandler();
    static_cast<BroadcastEventHandler*>(m_broadcast_event_handler)->Init(m_stat_manager);
    m_broadcast_mgr->SetEventHandler(m_broadcast_event_handler);

    // 广播暂只支持一种协议，默认为rpc binary，用户可修改
    m_broadcast_mgr->Attach(rpc);
    rpc->SetBroadcastFunction(
        cxx::bind(&BroadcastMgr::Send, m_broadcast_mgr, cxx::placeholders::_1,
            cxx::placeholders::_2, cxx::placeholders::_3, true),
        cxx::bind(&BroadcastMgr::SendV, m_broadcast_mgr, cxx::placeholders::_1,
            cxx::placeholders::_2, cxx::placeholders::_3, cxx::placeholders::_4, true)
        );

    return m_broadcast_mgr;

error:
    delete m_broadcast_relay_handler;
    delete m_broadcast_mgr;
    m_broadcast_relay_handler = NULL;
    m_broadcast_mgr = NULL;
    return NULL;
}

int32_t PebbleServer::InitControlService() {
    if (m_control_handler) {
        PLOG_INFO("control service is already created");
        return 0;
    }

    // 未配置控制命令监听地址，不启动控制命令服务
    if (m_options._app_ctrl_cmd_addr.empty()) {
        return 0;
    }

    // 所以依赖地址全格式统一放到配置中，避免对代码有影响
    int64_t handle = Bind(m_options._app_ctrl_cmd_addr);
    if (handle < 0) {
        PLOG_ERROR("bind %s failed(%ld,%s)", m_options._app_ctrl_cmd_addr.c_str(), handle,
            Message::GetLastError());
        return -1;
    }

    PebbleRpc* rpc = GetPebbleRpc(kPEBBLE_RPC_JSON);
    Attach(handle, rpc);

    m_control_handler = new PebbleControlHandler();

    int32_t ret = m_control_handler->Init();
    if (ret != 0) {
        PLOG_ERROR("Control Service init failed(%d)", ret);
        return -1;
    }

    ret = rpc->AddService(m_control_handler);
    if (ret != 0) {
        PLOG_ERROR("Add Control Service failed(%d)", ret);
        return -1;
    }

    // 注册内置命令
    using namespace cxx::placeholders;
    ret = m_control_handler->RegisterCommand(
        cxx::bind(&PebbleServer::OnControlReload, this, _1, _2, _3), "reload",
        "reload             # reload config file, no option",
        true);
    RETURN_IF_ERROR(ret != 0, ret, "register reload failed.");

    ret = m_control_handler->RegisterCommand(
        cxx::bind(&PebbleServer::OnControlPrint, this, _1, _2, _3), "print",
        "print              # print pebble runtime info\n"
        "                   # format  : print status | config\n"
        "                   # example : print status",
        true);
    RETURN_IF_ERROR(ret != 0, ret, "register print failed.");

    ret = m_control_handler->RegisterCommand(
        cxx::bind(&PebbleServer::OnControlLog, this, _1, _2, _3), "log",
        "log                # change pebble log level\n"
        "                   # format  : log trace | debug | info | error | fatal\n"
        "                   # example : log debug",
        true);
    RETURN_IF_ERROR(ret != 0, ret, "register log failed.");

    return 0;
}

int32_t PebbleServer::MakeCoroutine(const cxx::function<void()>& routine, bool start_immediately) {
    if (!m_coroutine_schedule) {
        PLOG_ERROR("coroutine schedule is null");
        return -1;
    }

    if (!routine) {
        PLOG_ERROR("task is null");
        return -1;
    }

    CommonCoroutineTask* task = m_coroutine_schedule->NewTask<pebble::CommonCoroutineTask>();
    if (NULL == task) {
        PLOG_ERROR("new co task failed");
        return -1;
    }

    task->Init(routine);
    int64_t coid = task->Start(start_immediately);

    return coid < 0 ? -1 : 0;
}

int32_t PebbleServer::RegisterControlCommand(const OnControlCommand& on_cmd,
    const std::string& cmd, const std::string& desc) {
    // desc 可以为空
    RETURN_IF_ERROR(cmd.empty(), -1, "cmd empty");
    RETURN_IF_ERROR(!on_cmd, -1, "on control command proc function is null");
    RETURN_IF_ERROR(!m_control_handler, -1, "control service not inited");

    return m_control_handler->RegisterCommand(on_cmd, cmd, desc);
}

void PebbleServer::OnControlReload(const std::vector<std::string>& options,
    int32_t* ret_code, std::string* data) {

    if (Reload() != 0) {
        *ret_code = -1;
        data->assign("reload failed.");
    } else {
        *ret_code = 0;
        data->assign("reload success.");
    }
}

void PebbleServer::OnControlPrint(const std::vector<std::string>& options,
    int32_t* ret_code, std::string* data) {
    if (!options.empty()) {
        *ret_code = 0;
        if (strcasecmp(options.front().c_str(), "config") == 0) {
            data->assign(m_options.ToString());
            return;
        } else if (strcasecmp(options.front().c_str(), "status") == 0) {
            std::ostringstream oss;
            oss << "version : " << GetVersion() << std::endl
                << "runtime : " << m_stat_manager->GetRuntimeInSecond() << " S" << std::endl;
            data->assign(oss.str());
            return;
        }
    }

    *ret_code = -1;
    data->assign("options is invalid, please see the help.");
}

void PebbleServer::OnControlLog(const std::vector<std::string>& options,
    int32_t* ret_code, std::string* data) {
    if (options.empty()) {
        *ret_code = -1;
        data->assign("options is null, please see the help.");
        return;
    }

    *ret_code = Log::Instance().SetLogPriority(options.front());
    if (*ret_code != 0) {
        data->assign("option is ");
        data->append(options.front());
        data->append(", must be trace/debug/info/error/fatal.");
        return;
    }

    data->assign("change log level to ");
    data->append(options.front());
    data->append(" success.");
    return;
}


}  // namespace pebble

