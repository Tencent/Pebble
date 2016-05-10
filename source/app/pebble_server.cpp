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


#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include "source/app/broadcast_mgr.h"
#include "source/app/control_handler.h"
#include "source/app/coroutine_rpc.h"
#include "source/app/pebble_server.h"
#include "source/app/servant_task.h"
#include "source/app/timer_task.h"
#include "source/broadcast/app_config.h"
#include "source/common/string_utility.h"
#include "source/log/log.h"
#include "source/message/message.h"
#include "source/version.inc"
#include "thirdparty/inih-master/cpp/INIReader.h"
#include <signal.h>
#include "version.h"

#define STOP_NORNAML 1
#define STOP_EXCEPTION 2

#define PEBBLE_DEFAULT_IDLE_COUNT     10
#define PEBBLE_DEFAULT_IDLE_SLEEP     10

static struct {
    int Stop;
    int Reload;
} g_manage_events = {0, 0};


void pebble_on_stop(int signal) {
    g_manage_events.Stop = SIGUSR1 == signal ? STOP_NORNAML : STOP_EXCEPTION;
}

void pebble_on_reload(int signal) {
    g_manage_events.Reload = 1;
}

namespace pebble {

bool PebbleServer::is_used_ = false;
const char* PebbleServer::kDefaultAppId = "1.0.1";
const char* PebbleServer::kTappSection = "tapp";
const char* PebbleServer::kRpcSection = "rpc";
const char* PebbleServer::kAppIdField = "id";
const char* PebbleServer::kGameIdField = "game_id";
const char* PebbleServer::kGameKeyField = "game_key";
const char* PebbleServer::kZookeeperAddressField = "zk_address";
// zk连接超时时间可通过配置文件配置，和zk_address共同配置在rpc组内，
// 字段名为zk_connect_timeout，单位为ms，不配置时默认为20000ms(20s)
const int PebbleServer::kTickTime = 1;
const char* PebbleServer::kTimerSection = "timer";
const char* PebbleServer::kMaxTaskNum = "max_num";
const char* PebbleServer::kControlPort = "control_port";
// 跨server广播，server间用于广播事件、消息通信的地址需要指定
// 地址配置在broadcast组下，字段名为relay_address，可以不配置
const char* PebbleServer::kBroadcastSection = "broadcast";
const char* PebbleServer::kRelayAddressField = "relay_address";

PebbleServer::PebbleServer()
        : status_(NOT_INIT),
          config_path_(""),
          config_(NULL),
          updaters_(),
          schedule_(NULL),
          rpc_(NULL),
          current_time_(0),
          task_timer_(NULL),
          is_during_update_(false),
          is_idle_(true),
          control_handler_(NULL),
          cmd_set_(),
          idle_count_(PEBBLE_DEFAULT_IDLE_COUNT),
          idle_sleep_(PEBBLE_DEFAULT_IDLE_SLEEP) {
    memset(app_name_, 0, sizeof(app_name_));
    m_broadcast_mgr = NULL;
}

PebbleServer::~PebbleServer() {
    // 预防使用中没有调用清理接口
    if (status_ != NOT_INIT)
        OnStop();
}

int PebbleServer::Init(int argc, char** argv, const char* config_path) {

    // 检查看是否只是显示下版本
    if (ShowVersion(argc, argv)) {
        return 0;
    }

    // 检查同一进程内是否有其他实例
    if (IsSingleton()) {
        return -1;
    }

    // 检查状态是否正常
    if (status_ == RUNNING) {
        PLOG_FATAL("PebbleServer is running, can not be initialized.");
        fprintf(stderr,
                "Error: PebbleServer is running so that it can not be init!\n");
        return -1;
    }

    if (0 != InitSignal()) {
        PLOG_INFO("Init Signal fail.");
        return -1;
    }

    // 加载配置文件
    config_path_ = config_path;
    LoadConfig();

    // 处理配置项目
    GetAppPath();
    char* tmp_argv[] = { app_name_ };
    if (argc == 0) {
        argc = 1;
        argv = tmp_argv;
    }
    ProcessCmdlind(argc, argv, &cmd_set_);

    InitLog();

    schedule_ = new CoroutineSchedule();
    if (!schedule_) {
        return -1;
    }

    // 启动协程功能
    if (schedule_->Init()) {
        PLOG_FATAL("PebbleServer initialize error: coroutine open got NULL.");
        fprintf(stderr, "Error: coroutine open return NULL!\n");
        return -1;
    }

    // 初始化定时器
    if (task_timer_ == NULL)
        task_timer_ = new TaskTimer();
    if (task_timer_ == NULL) {
        std::cout << "Can not create task timer! " << std::endl;
        return -1;
    }

    // rpc 需要提前创建
    rpc_ = rpc::CoroutineRpc::Instance();

    // 先创建BroadcastMgr实例，初始化放在OnInit中(有依赖)，避免用户在start前get一个空指针
    if (!m_broadcast_mgr) {
        m_broadcast_mgr = new BroadcastMgr(this);
    }
    if (!m_broadcast_mgr) {
        std::cout << "new BroadcastMgr failed" << std::endl;
        return -4;
    }
    // 提供对外服务，供客户端加入频道或退出频道
    m_broadcast_mgr->RegisterPebbleChannelMgrService();

    // 设定一下当前时间
    SetCurrentTime();

    // 启动定时器功能
    int max_task_num = config_->GetInteger(kTimerSection, kMaxTaskNum,
                                           TaskTimer::kDefaultTimerTaskNumber);
    if (task_timer_ == NULL)
        return -1;
    if (task_timer_->Init(schedule_, &current_time_, max_task_num))
        return -1;
    if (AddUpdater(task_timer_))
        return -1;

    int rpc_init = InitRpc();
    if (rpc_init) {
        PLOG_FATAL("Init RPC fail. (err_code: %d)", rpc_init);
        return -1;
    }

    // 广播模块加载失败可以启动
    if (InitBroadcastService() != 0) {
        PLOG_ERROR("init braodcast fail.");
    }

    // 内置command服务不暴露出去，在rpc初始化完(注册完服务)后再添加command服务
    if (RegisterCommonService()) {
        PLOG_INFO("Registering common service fail.");
    }

    status_ = INITED;
    return 0;
}

int PebbleServer::IsSingleton() {
    if (PebbleServer::is_used_) {
        PLOG_FATAL("PebbleServer initialize error: another tapp::IApp is using.");
        fprintf(stderr, "Error: another game server is using current process!\n");
        return -1;
    }
    PebbleServer::is_used_ = true;
    return 0;
}

int PebbleServer::InitRpc() {
    std::string app_id = config_->Get(kTappSection, kAppIdField, kDefaultAppId);
    std::string game_id_str = config_->Get(kRpcSection, kGameIdField, "");
    int64_t game_id = atol(game_id_str.c_str());
    std::string game_key = config_->Get(kRpcSection, kGameKeyField, "");
    std::string zk_address = config_->Get(kRpcSection, kZookeeperAddressField,
                                          "");
    int zk_connect_timeout = atoi(
            config_->Get(kRpcSection, "zk_connect_timeout", "20000").c_str());

    int conn_idle_timeout = atoi(
        config_->Get(kRpcSection, "connection_idle_timeout", "100").c_str());

    rpc_ = rpc::CoroutineRpc::Instance();

    if (rpc_ == NULL)
        return -1;

    if (!zk_address.empty()) {
        rpc_->SetZooKeeperAddress(zk_address, zk_connect_timeout);
    }

    if ((static_cast<rpc::CoroutineRpc*>(rpc_))->Init(this, app_id, game_id, game_key) != 0)
        return -2;

    rpc_->SetConnectionIdleTime(conn_idle_timeout);

    InitRpcCommandHandler();

    return 0;
}

int PebbleServer::InitSignal() {
    struct sigaction sa;

    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGPIPE, &sa, NULL);

    sigfillset(&sa.sa_mask);

    sa.sa_handler = pebble_on_stop;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);

    sa.sa_handler = pebble_on_reload;
    sigaction(SIGUSR2, &sa, NULL);

    return 0;
}

// TAPP命令行配置形式: --key=value
const char* tapp_param_prefix = "--";

// Pebble命令行配置形式: -Dsection_name=value
const char* pebble_param_prefix = "-D";
const char pebble_param_delimiter = '_';
const char value_delimiter = '=';
const char* pebble_default_config_section = "pebble";

void PebbleServer::ProcessCmdlind(int argc, char** argv,
                                  std::vector<std::string>* cmd_set) {
    // 先插入原先的命令行参数
    std::set<std::string> key_set;
    for (int i = 0; i < argc; ++i) {
        std::string arg(argv[i]);

        // 处理Pebble配置模块的参数，让命令行参数覆盖配置文件
        // 同时过滤掉这些参数不进入tapp的命令行
        if (arg.find(pebble_param_prefix) != std::string::npos) {
            std::string::size_type key_pos = arg.find(value_delimiter);
            if (key_pos == std::string::npos)
                continue;
            std::string value = arg.substr(key_pos + 1);
            std::string::size_type section_key_pos = arg.find(
                    pebble_param_prefix);
            size_t pebble_param_prefix_len = strlen(pebble_param_prefix);
            if (section_key_pos != std::string::npos) {
                std::string section_key = arg.substr(
                        pebble_param_prefix_len,
                        key_pos - section_key_pos - pebble_param_prefix_len);
                std::string section = pebble_default_config_section;
                std::string name = section_key;
                std::string::size_type name_pos = section_key.find(
                        pebble_param_delimiter);
                if (name_pos != std::string::npos) {
                    section = section_key.substr(0, name_pos);
                    name = section_key.substr(name_pos + 1);
                }
                config_->Set(section, name, value);
                std::cout << "Pebble[" << section << "][" << name << "]: "
                          << config_->Get(section, name, "N/A") << std::endl;
            }
            continue;
        }

        cmd_set->push_back(arg);

        // 处理TAPP命令行参数，覆盖配置文件同名项目
        if (arg.find(tapp_param_prefix) != std::string::npos) {
            std::string::size_type key_pos = arg.find(value_delimiter);
            if (key_pos == std::string::npos)
                continue;
            size_t key_delimiter_len = strlen(tapp_param_prefix);
            std::string tapp_key = arg.substr(key_delimiter_len,
                                              key_pos - key_delimiter_len);
            std::string value = arg.substr(key_pos + 1);
            std::cout << "TAPP[" << tapp_key << "]: " << value << std::endl;
            key_set.insert(tapp_key);
            config_->Set(kTappSection, tapp_key, value);
        }
    }

    // 从配置文件中插入其他TAPP命令行参数
    std::set<std::string> fields = config_->GetFields(kTappSection);
    for (std::set<std::string>::iterator fieldsIt = fields.begin();
            fieldsIt != fields.end(); fieldsIt++) {
        // 如果配置文件有，而命令行参数没有，则加入命令行
        if (key_set.find(*fieldsIt) == key_set.end()) {
            cmd_set->push_back(
                    tapp_param_prefix + *fieldsIt + value_delimiter
                            + config_->Get(kTappSection, *fieldsIt, ""));
        }
    }
}

void PebbleServer::GetAppPath() {

    // 获得当前可执行文件目录
    char link_target[1024];
    int rval = readlink("/proc/self/exe", link_target, sizeof(link_target));
    if (rval == -1) {
        std::cout << "Get current executable file path error!" << std::endl;
        return;
    }
    if (rval >= static_cast<int>(sizeof(link_target))) {
        std::cout << "Get current executable file path len(" << rval
                  << ") >= 1024!" << std::endl;
        return;
    }
    link_target[rval] = '\0';

    char* last_slash = strrchr(link_target, '/');
    if (last_slash == NULL || last_slash == link_target) {
        std::cout << "Parse current executable file path error!" << std::endl;
        return;
    }

    // 拷贝目录部分
    size_t result_length = last_slash - link_target + 1;
    char* result = new char[result_length + 1];
    strncpy(result, link_target, result_length);
    result[result_length] = '\0';

    // 拷贝命令部分
    strncpy(app_name_, link_target + result_length, rval - result_length);

    // 绝对路径直接使用
    if (StringUtility::StartsWith(config_path_, "/")) {
        delete[] result;
        return;
    }

    // 修改config_path_的内容
    config_path_ = result + config_path_;
    delete[] result;
}

CoroutineSchedule* PebbleServer::GetCoroutineSchedule() {
    return schedule_;
}

std::string PebbleServer::GetConfig(const std::string &section,
                                    const std::string &name,
                                    std::string default_value) {
    if (config_ == NULL)
        return default_value;
    return config_->Get(section, name, default_value);
}

int PebbleServer::RegisterControlCommand() {
    // 控制命令以http://127.0.0.1:port进行监听
    std::string control_port = config_->Get(kRpcSection, kControlPort, "");
    // 未配置端口，则不开启控制命令功能
    if (control_port == "" || control_port == "0") {
        return 0;
    }
    std::string listen_url("http://127.0.0.1:");
    listen_url += control_port;
    // 内置控制命令服务不注册到zk上
    if (rpc_->AddServiceManner(listen_url, pebble::rpc::PROTOCOL_BINARY, false)
            != 0) {
        PLOG_ERROR("PebbleServer register control service failed, "
                   "listen_url conflict: %s.",
                   listen_url.c_str());
        return -1;
    }
    // 注册运营控制命令服务
    if (control_handler_) {
        delete control_handler_;
        control_handler_ = NULL;
    }
    control_handler_ = new pebble::ControlCommandHandler(this);
    if (rpc_->RegisterService(control_handler_) != 0) {
        PLOG_ERROR("PebbleServer register control service failed, listen_url: %s.",
                listen_url.c_str());
        return -1;
    }
    return 0;
}

// 注册内置服务
int PebbleServer::RegisterCommonService() {
    // 注册运营控制命令服务
    if (RegisterControlCommand() != 0) {
        return -1;
    }
    return 0;
}

void PebbleServer::Start() {
    Serve();
}

int PebbleServer::SetIdleCount(int idle_count) {
    if (idle_count > 1) {
        idle_count_ = idle_count;
        return 0;
    } else {
        return -1;
    }
}

int PebbleServer::SetIdleSleep(int idle_sleep) {
    if (idle_sleep > 1) {
        idle_sleep_ = idle_sleep;
        return 0;
    } else {
        return -1;
    }
}

int PebbleServer::SetOnStop(cxx::function<int ()> callback) {
    if (on_stop_callback_) {
        return -1;
    } else {
        on_stop_callback_ = callback;
        return 0;
    }
}

int PebbleServer::SetOnReload(cxx::function<void ()> callback) {
    if (on_reload_callback_) {
        return -1;
    } else {
        on_reload_callback_ = callback;
        return 0;
    }
}

int PebbleServer::SetOnIdle(cxx::function<void ()> callback) {
    if (on_idle_callback_) {
        return -1;
    } else {
        on_idle_callback_ = callback;
        return 0;
    }
}

void PebbleServer::SetOnVersion(cxx::function<void ()> callback) {
    on_version_callback_ = callback;
}

void PebbleServer::Serve() {
    static int idle_loop = 0;

    if (status_ != INITED) {
        PLOG_FATAL("PebbleServer had not been initialized, please call the Init() first!");
        fprintf(stderr, "Error: Try to Start() the PebbleServer Before Init()! \n");
        return;
    }

    int ret = rpc_->RegisterServiceInstance();
    if (ret != 0) {
        PLOG_ERROR("register service failed(%d)", ret);
        return;
    }

    status_ = RUNNING;

    while (RUNNING == status_) {
        SetCurrentTime(); // 覆盖原来的OnTick

        if (g_manage_events.Stop) {
            OnStop();
        }

        if (g_manage_events.Reload) {
            g_manage_events.Reload = 0;
            OnReload();
        }

        if (OnUpdate() < 0) {
            ++idle_loop;
        } else {
            idle_loop = 0;
        }

        if (idle_loop >= idle_count_) {
            idle_loop = 0;
            OnIdle();
        }
    }
}

void PebbleServer::OnReload() {
    LoadConfig();
    InitLog();

    if (on_reload_callback_) {
        on_reload_callback_();
    }
}

void PebbleServer::OnIdle() {
    log::Log::Flush();

    if (on_idle_callback_) {
        on_idle_callback_();
    } else {
        usleep(idle_sleep_ * 1000);
    }
}

void PebbleServer::OnStop() {
    if (on_stop_callback_) {
        if (on_stop_callback_() >= 0) {
            return;
        }
        on_stop_callback_ = NULL;
    }

    // 清理定时器
    if (task_timer_ != NULL) {
        updaters_.erase(task_timer_);
        delete task_timer_;
        task_timer_ = NULL;
    }

    // 删除的协程数为0才继续往下走
    if (schedule_ != NULL && schedule_->Close()) {
        delete schedule_;
        return;
    }

    // 清理配置文件对象
    if (config_ != NULL) {
        delete config_;
        config_ = NULL;
    }

    delete control_handler_;
    control_handler_ = NULL;

    if (m_broadcast_mgr) {
        delete m_broadcast_mgr;
        m_broadcast_mgr = NULL;
    }

    status_ = STOPPED;

    PLOG_INFO("stoped by event: %d.", g_manage_events.Stop);

    log::Log::Close();
}

int PebbleServer::AddUpdater(UpdaterInterface* updater) {
    updaters_.insert(updater);
    return 0;
}

int PebbleServer::RemoveUpdater(UpdaterInterface* updater) {
    wait_to_remove_updaters_.push_back(updater);
    return 0;
}

int PebbleServer::OnUpdate() {
    assert(schedule_ != NULL);
    int ret = 0;
    bool is_now_idle = true;

    // 处理过程中取消Update
    if (status_ != RUNNING)
        return -1;

    // 处理RPC请求包
    if (rpc_ != NULL) {
        int rpu_ret = rpc_->Update();
        int rpc_ret = static_cast<pebble::rpc::CoroutineRpc*>(rpc_)
                ->ProcessPendingMsg();
        if (rpu_ret > 0 || rpc_ret > 0)
            is_now_idle = false;
    }

    // 处理广播服务
    if (m_broadcast_mgr != NULL) {
        int bro_ret = m_broadcast_mgr->Update();
        if (bro_ret > 0)
            is_now_idle = false;
    }

    // 挨个执行更新器的功能
    is_during_update_ = true;
    std::set<UpdaterInterface*>::iterator it = updaters_.begin();
    while (it != updaters_.end()) {
        UpdaterInterface* updater = *it;
        it++;
        if (updater->Update(this) >= 0)
            is_now_idle = false;
    }
    is_during_update_ = false;

    // 处理要删除的Updater
    if (wait_to_remove_updaters_.empty() == false) {
        is_now_idle = false;
        std::vector<UpdaterInterface*>::iterator remove_it =
                wait_to_remove_updaters_.begin();
        UpdaterInterface* will_be_removed = NULL;
        while (remove_it != wait_to_remove_updaters_.end()) {
            will_be_removed = *remove_it;
            updaters_.erase(will_be_removed);
            ++remove_it;
        }
        wait_to_remove_updaters_.clear();
    }

    // 判断此次循环周期是否空闲
    if (is_now_idle)
        ret = -1;
    is_idle_ = is_now_idle;
    return ret;
}

UpdaterInterface::~UpdaterInterface() {
    // DO NOTHING
}

int PebbleServer::RegisterService(
        const std::string &custem_service_name,
        cxx::shared_ptr<rpc::processor::TAsyncProcessor> service,
        int route_type) {
    int ret = rpc_->RegisterService(custem_service_name.empty() ?
            NULL : custem_service_name.c_str(), service, route_type);
    std::string service_name = service->getServiceName();
    if (ret) {
        PLOG_ERROR("Register service %s error, return code: %d.",
                service_name.c_str(), ret);
    }
    return ret;
}

int PebbleServer::AddServiceManner(const std::string& listen_url,
                                   rpc::PROTOCOL_TYPE protocol_type,
                                   bool register_2_zk) {
    int ret = rpc_->AddServiceManner(listen_url, protocol_type,
                register_2_zk);
    if (ret) {
        PLOG_ERROR("add transport %s failed",
                listen_url.c_str());
    }

    return ret;
}

int PebbleServer::AddServiceManner(const std::string& addr_in_docker,
                                   const std::string& addr_in_host,
                                   rpc::PROTOCOL_TYPE protocol_type) {
    int ret = rpc_->AddServiceManner(addr_in_docker, addr_in_host,
            protocol_type);
    if (ret) {
        PLOG_ERROR("add docker transport %s failed",
                addr_in_docker.c_str());
    }
    return ret;
}

int PebbleServer::LoadConfig() {
    if (config_ != NULL)
        delete config_;
    std::cout << "Loading configure from file: " << config_path_ << " ..."
              << std::endl;
    PLOG_INFO("Loading configure file %s ...",
              config_path_.c_str());
    config_ = new INIReader(config_path_);
    int parse_ret = config_->ParseError();
    if (parse_ret < 0) {
        PLOG_ERROR("Can not load configure file: %s, using default configure.",
            config_path_.c_str());
        std::cout << "Can not load configure file: " << config_path_
                  << ", using default configure." << std::endl;
        return -1;
    }
    if (parse_ret > 0) {
        PLOG_ERROR("Configure file parse error at line %d.", parse_ret);
        std::cout << "Configure file " << config_path_
                  << " parse error at line " << parse_ret
                  << ", the server will try to use other line." << std::endl;
    }

    OverrideConfigWithEnv();
    return 0;
}

int PebbleServer::StartTimerTask(TimerTaskInterface* task, int interval) {
    if (task_timer_ == NULL)
        return -1;
    return task_timer_->AddTask(task, interval);
}

int PebbleServer::StopTimerTask(int task_id) {
    if (task_timer_ == NULL)
        return -1;  // 整个服务未初始化或者已经停止
    return task_timer_->RemoveTask(task_id);
}

TimerTaskInterface::~TimerTaskInterface() {
    // DO NOTHING
}

bool PebbleServer::is_idle() {
    return is_idle_;
}

bool PebbleServer::ShowVersion(int argc, char** argv) {
    for (int i = 0; i < argc; ++i) {
        char* arg = argv[i];
        if (strcasecmp(arg, "-v") == 0 || strcasecmp(arg, "--version") == 0) {
            if (on_version_callback_) {
                on_version_callback_();
            }
            std::cout << "Pebble : " << VERSION << "." << PEBBLE_BUILD_VERSION << std::endl;
            exit(0);
        }
    }
    return false;
}

void PebbleServer::SetCurrentTime() {
    struct timeval now;
    gettimeofday(&now, NULL);
    current_time_ = 1000000 * now.tv_sec;
    current_time_ += now.tv_usec;
}

void PebbleServer::OverrideConfigWithEnv() {
    std::string cfg_output;
    std::map<std::string, std::map<std::string, std::string> > cover_env;

    // 遍历config_
    std::set<std::string> sections = config_->GetSections();
    for (std::set<std::string>::iterator sectionsIt = sections.begin();
            sectionsIt != sections.end(); sectionsIt++) {
        cfg_output += "[" + *sectionsIt + "]: ";
        std::set<std::string> fields = config_->GetFields(*sectionsIt);
        for (std::set<std::string>::iterator fieldsIt = fields.begin();
                fieldsIt != fields.end(); fieldsIt++) {
            cfg_output += *fieldsIt + " ";
            std::string env_key = *sectionsIt + pebble_param_delimiter
                    + *fieldsIt;
            char* env = getenv(env_key.c_str());
            if (env != NULL) {
                std::string value(env);
                std::map<std::string, std::string> conver_fields =
                        cover_env[*sectionsIt];
                conver_fields[*fieldsIt] = value;
                cover_env[*sectionsIt] = conver_fields;
            }
        }
    }
    PLOG_INFO("Configure file content: %s", cfg_output.c_str());

    // 尝试getenv()覆盖
    std::map<std::string, std::map<std::string, std::string> >::iterator cit =
            cover_env.begin();
    while (cit != cover_env.end()) {
        std::string section_name = cit->first;
        std::map<std::string, std::string>::iterator fields_it = cit->second
                .begin();
        while (fields_it != cit->second.end()) {
            PLOG_INFO("Setting env cover: %s.%s=%s",
                      section_name.c_str(), fields_it->first.c_str(),
                      fields_it->second.c_str());
            config_->Set(section_name, fields_it->first, fields_it->second);
            ++fields_it;
        }
        ++cit;
    }
}

void PebbleServer::InitRpcCommandHandler() {
    pebble::rpc::EventCallbacks call_back;
    call_back.event_idle = cxx::bind(&pebble::PebbleServer::OnConnectionIdle, this,
            cxx::placeholders::_1, cxx::placeholders::_2, cxx::placeholders::_3);
    rpc_->RegisterEventhandler(call_back);
}

int PebbleServer::OnConnectionIdle(int handle, uint64_t addr, int64_t session_id) {
    return 0;
}

int PebbleServer::GetCurConnectionObj(rpc::ConnectionObj* connection_obj) {
    if (!rpc_) {
        return -1;
    }

    return rpc_->GetCurConnectionObj(connection_obj);
}

int PebbleServer::InitBroadcastService() {
    if (!m_broadcast_mgr) {
        PLOG_ERROR("broadcast manager is NULL.");
        return -1;
    }

    broadcast::AppConfig cfg;
    cfg.game_id = atoi(config_->Get(kRpcSection, kGameIdField, "").c_str());
    cfg.game_key = config_->Get(kRpcSection, kGameKeyField, "");

    cfg.app_id = config_->Get(kTappSection, kAppIdField, kDefaultAppId);
    cfg.zk_host = config_->Get(kRpcSection, kZookeeperAddressField, "");
    cfg.timeout_ms = 5000;

    // 从配置读取监听端口，组装广播监听地址
    // TODO(tatezhang): 运维功能补充: 未配置时自动获取空闲地址
    std::string relay_address = config_->Get(kBroadcastSection,
                                             kRelayAddressField, "");
    if (relay_address.empty()) {
        PLOG_INFO("broadcast_relay_address not configed.");
    }

    int ret = -1;
    do {
        ret = m_broadcast_mgr->SetAppConfig(cfg);
        if (ret != 0) {
            break;
        }

        if (!relay_address.empty()) {
            ret = m_broadcast_mgr->SetListenAddress(relay_address);
            if (ret != 0) {
                break;
            }
        }

        ret = m_broadcast_mgr->Init();
    } while (0);

    if (ret != 0) {
        PLOG_ERROR("BroadcastMgr init failed(%d).", ret);
        return -2;
    }

    m_broadcast_mgr->OpenChannels();

    return 0;
}

BroadcastMgr* PebbleServer::GetBroadcastMgr() {
    return m_broadcast_mgr;
}

void PebbleServer::InitLog()
{
    const char* log_section  = "log";
    std::string device        = config_->Get(log_section, "device",        "");
    std::string priority      = config_->Get(log_section, "priority",      "");
    std::string max_file_size = config_->Get(log_section, "max_file_size", "");
    std::string max_roll_num  = config_->Get(log_section, "max_roll_num",  "");
    std::string file_path     = config_->Get(log_section, "file_path",     "");

    log::Log::SetOutputDevice(device);
    log::Log::SetLogPriority(priority);
    log::Log::SetMaxFileSize(atoi(max_file_size.c_str()));
    log::Log::SetMaxRollNum(atoi(max_roll_num.c_str()));
    log::Log::SetFilePath(file_path);

    log::Log::EnableCrashRecord();
}

}  // namespace pebble

