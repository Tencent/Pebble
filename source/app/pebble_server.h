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


#ifndef  SOURCE_APP_PEBBLE_SERVER_H
#define  SOURCE_APP_PEBBLE_SERVER_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include "source/app/pebble_server_fd.h" // 前置声明
#include "source/rpc/rpc.h"


/// @brief 主要的 Pebble API名字空间
namespace pebble {

/// 此枚举表示Pebble Server的运行状态，分为刚构建的“未初始化”状态，
/// 还未运行的“已初始化”状态，“正在运行”状态，以及最后“关闭状态”
enum PebbleStatus {
    NOT_INIT = 0,
    INITED = 1,
    RUNNING = 2,
    STOPPED = 3,
};

/// @brief 此类提供一个Game Server的主控制对象。
///
/// 并且重建了两个流程控制接口：OnSigReload() & OnSigQuit()
class PebbleServer {
public:
    PebbleServer();
    virtual ~PebbleServer();

    /// @brief 初始化此对象
    /// 默认配置文件是'../cfg/pebble.ini'，你可以用三种方法自定义配置文件中的项目：
    /// 第一是修改配置文件; 第二是使用类似：-Dsection_name=value的命令行参数;
    /// 第三是使用系统变量，如：export section_name=value。这三种方式的优先级是：
    /// 命令行>系统变量>配置文件。
    /// 此方法会尝试解析命令行参数，和系统变量，以便后续可以用GetConfig()获得所需
    /// 配置内容。
    /// @note 如果你想使用系统变量，你必须在配置文件中有对应的section和name
    /// 的配置。只有这样的配置项目才能被覆盖。
    /// @note 如果你想使用命令行参数来设定此服务的参数，
    /// 应该正确从main()中获得argc,argv参数传入此方法
    ///
    /// @param[in] argc 命令行参数个数，如果此参数不为0, argv不得为NULL。
    /// @param[in] argv 命令行参数内容
    /// @param[in] config_path 此对象使用的配置文件目录
    /// @return 0 表示初始化成功, 其他值表示失败。
    int Init(int argc = 0, char** argv = NULL,
             const char* config_path = "../cfg/pebble.ini");

    /// @brief 读取框架配置值
    ///
    /// @param[in] section 配置项所在的组
    /// @param[in] name 配置项名称
    /// @param[in] default_value 配置项的默认值，如果找不到对应的配置会返回此值
    /// @return  指定配置组section下面对应配置名称name的配置值
    std::string GetConfig(const std::string &section, const std::string &name,
                          std::string default_value = "");

    /// @brief 注册RPC服务
    ///
    /// 此处注册的RPC服务，是由pebble工具，根据IDL描述文件自动生成的源代码所定义的。
    /// 用户应该先用pebble IDL格式定义RPC服务的接口形式，然后使用pebble工具对IDL文件
    /// 进行处理，生成C++以及其他源代码，其中的"服务接口定义类"即为此处 service参数的
    /// 输入对象。service对象中会包含一个或多个可被远程调用的函数，用户可以自己定义这
    /// 些函数的名字、参数列表，并且可以用继承的方式实现具体的逻辑。
    /// @see 《Pebble RPC开发手册.pdf》包含完整的RPC服务定义教程。
    /// @note 可使用类型推断功能，省略掉Class参数输入，编译器会自动根据service参数推断
    /// @param[in] service 想要注册的RPC服务对象，必须是pebble工具生成的类型。
    /// @param route_type 服务的路由策略，支持3种路由方式(默认取值为1):\n
    ///                1:    轮询路由
    ///                2:    哈希路由
    ///                3:    取模路由
    /// @return 0 成功
    /// @return 非0 失败
    template<typename Class>
    int RegisterService(Class* service, int route_type = 1);

    /// @brief 注册RPC服务
    ///
    /// @note 可使用类型推断功能，省略掉Class参数输入，编译器会自动根据service参数推断
    /// @param custom_service_name 自定义服务名
    /// @param[in] service 想要注册的RPC服务对象
    /// @param route_type 服务的路由策略，支持3种路由方式(默认取值为1):\n
    ///                1:    轮询路由
    ///                2:    哈希路由
    ///                3:    取模路由
    /// @return 0 成功
    /// @return 非0 失败
    template<typename Class>
    int RegisterService(const std::string &custom_service_name, Class* service,
                        int route_type = 1);

    /// @brief 添加监听地址，一个地址只能承载一种编码协议\n
    /// @param listen_url server监听地址，支持tcp/http/tbus/pipe传输协议，如:\n
    ///   "tcp://127.0.0.1:6666"\n
    ///   "http://127.0.0.1:7777"\n
    ///   "tbus://1.1.1:8888"\n
    ///   "pipe://1.1.1:9999"
    /// @param protocol_type 此通道使用的编码协议，支持binary/json/bson
    /// @param register_2_zk 此监听地址是否要注册到zk，默认注册，(调试地址可选择不注册)
    /// @return 0 成功
    /// @return 非0 失败
    int AddServiceManner(const std::string& listen_url,
                         rpc::PROTOCOL_TYPE protocol_type, bool register_2_zk = true);

    /// @brief 添加监听地址，server部署在docker环境下使用，一个地址只能承载一种编码协议\n
    /// @param addr_in_docker 在docker中的地址，实际监听此地址
    /// @param addr_in_host 在母机中的地址，对外公布的是此地址 \n
    ///   "tcp://127.0.0.1:6666"\n
    ///   "http://127.0.0.1:7777"
    /// @return 0 成功
    /// @return 非0 失败
    /// @note 当server部署在docker环境中时，对tcp/http传输需要使用此接口，tbus/pipe无需使用
    int AddServiceManner(const std::string& addr_in_docker,
                                const std::string& addr_in_host,
                                rpc::PROTOCOL_TYPE protocol_type);

    /// @brief 兼容API，已经废弃，新API是Serve
    void Start();

    /// @brief 启动这个服务，将会阻塞当前线程。
    /// 注意调用此函数前，必须先调用Init()方法，并且确保返回值是0。
    /// 完整的使用顺序是：声明并构造变量；调用Init()返回0；调用Serve()阻塞
    void Serve();

    /// @brief 获得协程调度器，你可以用它来控制协程的Yield,Resume和获得当前协程ID
    CoroutineSchedule* GetCoroutineSchedule();

    /// @brief 在主循环中添加自己的代码。
    /// @param[in] updater 你想要添加的代码应该实现UpdaterInterface，然后传入此处。
    /// 你必须保证传入的这个对象指针在整个game server运行期间都是可用的。
    /// @return 返回0表示成功，如果返回-1表示有错误，通常是updater太多达到配置上限了。
    int AddUpdater(UpdaterInterface* updater);

    /// @brief 删除安装在这个server上的updater
    /// @param[in] updater 要删除的updater
    /// @return 你删除的updater的数量，一般来说如果updater已经安装，应该是1。
    int RemoveUpdater(UpdaterInterface* updater);

    /// @brief 启动一个新的定时任务
    ///
    /// @param[in] task 实现了TimerTaskInterface的对象。
    /// @param[in] interval 并定时调用的间隔，单位：1/1000 秒（毫秒）
    /// @return 返回启动的定时任务的ID， 这个ID你可以用来停止此任务
    /// 如果错误则返回-1, 比如已经在运行太多的任务。
    int StartTimerTask(TimerTaskInterface* task, int interval);

    /// @brief 使用定时任务的ID来停止任务
    ///
    /// @param[in] task_id 启动定时任务返回的ID，在这里传入可以停掉此ID对应的任务
    /// @return 返回0表示成功；返回-1表示错误，原因可能是这个任务已经停止了。
    int StopTimerTask(int task_id);

    /// @brief 获取当前连接对象，只在用户的具体服务实现函数中调用有效
    /// @param connection_obj 输出参数，将当前的连接信息写到connection_obj中
    /// @return 0 获取成功
    /// @return 非0 获取失败
    int GetCurConnectionObj(rpc::ConnectionObj* connection_obj);

    /// @brief 收到控制命令时此方法会被调用。
    ///
    /// @param[in] command 控制命令名字字符串，
    ///  其格式和功能由用户定义
    /// @param[out] result 命令结果
    /// @param[out] description 命令结果的描述
    virtual void OnControlCommand(const std::string& command, int* result,
                                  std::string* description) {
    }

    /// @brief 获取广播manager对象，用户可使用manager对象来完成广播相关操作
    /// @return 非空 获取成功，manager对象可用
    /// @return 空 获取失败
    BroadcastMgr* GetBroadcastMgr();

    /// @brief 返回上一次主循环是否空闲。
    bool is_idle();

    /// @brief 设置多少个空闲周期才调用OnIdle回调
    /// @param[in] idle_count 空闲周期个数
    int SetIdleCount(int idle_count);

    /// @brief 如果没有设置OnIdle的话，经过IdleCount个空闲周期后的休眠时间
    /// @param[in] idle_sleep 休眠时间，单位为毫秒
    int SetIdleSleep(int idle_sleep);

    /// @brief 设置OnStop回调
    /// @param[in] callback 当有Stop事件时，该回调会被调用，
    /// 当返回值小于0表示确认可以退出，否则将继续下个处理周期
    int SetOnStop(cxx::function<int ()> callback);

    /// @brief 设置OnReload回调
    /// @param[in] callback 当有Reload事件时，该回调会被调用
    int SetOnReload(cxx::function<void ()> callback);

    /// @brief 设置OnIdle回调
    /// @param[in] callback 当有IdleCount个周期空闲，该回调会被调用
    int SetOnIdle(cxx::function<void ()> callback);

    /// @brief 设置输出业务版本信息的回调
    /// @param[in] callback 当命令行参数为 -v 或 --version 时callback会被调用
    /// @note 需要在 Init 前调用
    void SetOnVersion(cxx::function<void ()> callback);

private:
    static const char* kTappSection;
    static const char* kDefaultAppId;
    static const char* kRpcSection;
    static const char* kAppIdField;
    static const char* kGameIdField;
    static const char* kGameKeyField;
    static const char* kZookeeperAddressField;
    static const char* kTimerSection;
    static const char* kMaxTaskNum;
    static const int kTickTime;                 // 默认tick时间间隔，毫秒为单位
    static const char* kBroadcastSection;
    static const char* kRelayAddressField;

    pebble::PebbleStatus status_;                 // 当前状态
    std::string config_path_;                    // 配置文件所在目录
    INIReader* config_;                          // 存放配置的对象

    // 接收控制命令的端口号，IP固定为127.0.0.1，传输方式固定为HTTP
    static const char* kControlPort;    // 来自TAPP框架的回调，在此以其他方法取代了
    int OnUpdate();  // 替换为：AddUpdaterInterface()
    void OnReload();  // 替换为：TappHandler::OnTappReload()
    void OnIdle();
    void OnStop();  // 替换为：TappHandler::OnTappQuit()

    // 因为TAPP规定一个进程只能有一个APP运行，且用了很多static或全局变量，
    // 因此用一个静态变量控制不可在一个进程内启动多个。
    static bool is_used_;

    // 更新程序列表
    std::set<UpdaterInterface*> updaters_;

    // 等待删除的更新程序
    std::vector<UpdaterInterface*> wait_to_remove_updaters_;
    CoroutineSchedule* schedule_;                         // 协程句柄
    rpc::Rpc* rpc_;                                         // RPC服务器对象
    uint64_t current_time_;                                // 当前的时间戳，单位微秒
    TaskTimer* task_timer_;                                // 定时任务管理器
    char app_name_[256];                                   // 存放当前进程名字
    bool is_during_update_;                                // 表示进程是否可Resume协程
    bool is_idle_;                                         // 表示主循环过程中是否空闲

    pebble::ControlCommandHandler* control_handler_;     // 控制命令服务

    std::vector<std::string> cmd_set_;                    // 命令行处理结果

    BroadcastMgr* m_broadcast_mgr;

    int idle_count_;
    int idle_sleep_;

    cxx::function<int ()> on_stop_callback_;
    cxx::function<void ()> on_reload_callback_;
    cxx::function<void ()> on_idle_callback_;
    cxx::function<void ()> on_version_callback_;

    int IsSingleton();
    int InitRpc();
    int InitSignal();
    int RegisterService(
            const std::string &custem_service_name,
            cxx::shared_ptr<rpc::processor::TAsyncProcessor> service,
            int route_type);
    int LoadConfig();
    void ProcessCmdlind(int argc, char** argv,
                        std::vector<std::string>* cmd_set);
    void GetAppPath();
    void OverrideConfigWithEnv();
    int RegisterCommonService();
    int RegisterControlCommand();
    void InitRpcCommandHandler();
    bool ShowVersion(int argc, char** argv);
    int OnConnectionIdle(int handle, uint64_t addr, int64_t session_id);

    /// @brief 初始化广播模块
    /// @return 0 成功
    /// @return 非0 失败
    int InitBroadcastService();
    void SetCurrentTime();
    void InitLog();
};

// service 不用释放，由game server管理
template<typename Class>
int PebbleServer::RegisterService(Class* service, int route_type) {
    if (NULL == service) {
        return -1;
    }
    return RegisterService(
            "",
            pebble::rpc::GetProcessor<typename Class::__InterfaceType>()(
                    service),
            route_type);
}

// service 不用释放，由game server管理
template<typename Class>
int PebbleServer::RegisterService(const std::string &custom_service_name,
                                  Class* service, int route_type) {
    if (custom_service_name.empty()) {
        PLOG_ERROR("service name is invalid!");
        return -1;
    }
    if (NULL == service) {
        return -1;
    }
    return RegisterService(
            custom_service_name,
            pebble::rpc::GetProcessor<typename Class::__InterfaceType>()(
                    service),
            route_type);
}

/// @brief 用户自定义的定时器接口
class TimerTaskInterface {
public:
    virtual ~TimerTaskInterface();

    /// @brief 用户用自定义功能实现此方法。
    /// 此方法将会被定期调用
    virtual void OnTimer() = 0;
};

/// @brief 用户可以使用此接口插入代码到主循环中。
/// 一般用来检查异步状态然后Resume协程
class UpdaterInterface {
public:
    virtual ~UpdaterInterface();

    /// @brief 此方法每个主循环都被调用
    ///
    /// @param[in] pebble_server  主循环所在的PebbleServer对象
    /// @return <0: 如果此次调用执行的任务是空闲的，应当返回负值。
    /// 如果所有的Updater的任务都是空闲（Update()返回负值），框架
    /// 将休眠一小段时间。
    virtual int Update(PebbleServer *pebble_server) = 0;
};

}  // namespace pebble
#endif   //  SOURCE_APP_PEBBLE_SERVER_H
