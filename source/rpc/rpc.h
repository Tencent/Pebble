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


#ifndef PEBBLE_RPC_RPC_H
#define PEBBLE_RPC_RPC_H

#include <map>
#include <string>
#include <vector>
#include <set>
#include "source/common/pebble_common.h"
#include "source/rpc/common/rpc_define.h"
#include "source/log/log.h"



namespace pebble {

namespace net {
    class HttpDriver;
    class TcpDriver;
    struct NetEventCallbacks;
    class ConnectionMgr;
}

namespace service {
    class ServiceManager;
    struct ServiceAddressInfo;
}

namespace rpc {

namespace protocol {
    class TProtocol;
    class GroupProtocol;
}

namespace processor {
    class TAsyncProcessor;
}

/// @brief 客户端连接事件回调函数原型
/// @param handle 连接句柄
/// @param peer_addr 对端地址
typedef cxx::function<void (int handle, uint64_t peer_addr)> EventConnected;

/// @brief 客户端连接关闭事件回调函数原型
/// @param handle 连接句柄
/// @param peer_addr 对端地址
typedef cxx::function<void (int handle, uint64_t peer_addr)> EventClosed;

/// @brief 连接进入IDLE状态
/// @param handle 连接句柄
/// @param peer_addr 对端地址
/// @param session_id 连接承载的会话ID
typedef cxx::function<void ( // NOLINT
    int handle, uint64_t peer_addr, int64_t session_id)> EventIdle; // NOLINT


/// @brief 网络事件回调
typedef struct tag_EventCallbacks {
    EventConnected event_connected;
    EventClosed event_closed;
    EventIdle event_idle;
} EventCallbacks;


/// @brief 连接对象，用于保存连接地址信息
class ConnectionObj {
public:
    ConnectionObj();

    virtual ~ConnectionObj();

    /// @brief 获取远端原始地址
    /// @return string 远端原始地址，如"tcp://127.0.0.1:888"
    /// @note 暂未实现
    std::string GetOriginalAddr();

    /// @brief 把连接句柄、对端地址赋给连接对象
    /// @param handle 连接句柄
    /// @param peer_addr 对端地址
    /// @param protocol_type 连接对应的编码协议类型
    /// @param session_id 会话id，只针对pipe连接有效，-1表示无会话，默认为-1
    /// @note 此接口由RPC框架调用，用户不用关注
    void Set(int handle, uint64_t peer_addr, int32_t protocol_type, int64_t session_id = -1);

    /// @brief 恢复连接对象
    /// @param restore_url 通过 @ref GetRestoreUrl 获取
    /// @param peer_addr 对端地址
    /// @param session_id 会话id，只针对pipe连接有效，-1表示无会话，默认为-1
    /// @return 成功返回true，失败返回flase
    /// @note 仅供pipe协议使用，其它协议会报错。调用了Rpc::AddServiceManner添加监听地址
    /// 后该Restore方法才有用，否则报错。
    bool Restore(const std::string &restore_url, uint64_t peer_addr, int64_t session_id);

    /// @brief 获取恢复用地址
    /// @param 接收返回的指针
    /// @return 成功返回true，失败返回flase
    bool GetRestoreUrl(std::string * restore_url);

    /// @brief 获取连接句柄
    /// @return int 连接句柄
    /// @note 此接口由RPC框架调用，用户不用关注
    int GetHandle() const;

    /// @brief 获取远端地址
    /// @return uint64_t 远端地址
    /// @note 此接口由RPC框架调用，用户不用关注
    uint64_t GetPeerAddr() const;

    /// @brief 获取连接指定的协议编码格式
    /// @return int32_t 编码格式
    /// @note 此接口由RPC框架调用，用户不用关注
    int32_t GetProtocolType() const;

    /// @brief 获取会话id
    /// @return uint64_t 会话id
    /// @note 此接口由RPC框架调用，用户不用关注
    int64_t GetSessionId() const;

    /// @brief 重载 =
    virtual ConnectionObj& operator = (const ConnectionObj& conn_obj);

private:
    int32_t m_handle;
    int32_t m_protocol_type;
    uint64_t m_peer_addr;
    int64_t m_session_id;
};

/// @brief Rpc接口定义，封装连接管理、服务管理、消息处理等功能
class Rpc {
protected:
    static Rpc* m_rpc;

    Rpc();

public:
    /// @brief 获取RPC实例
    /// @return Rpc* Rpc实例指针
    static Rpc* Instance() {
        if (NULL == m_rpc) {
            m_rpc = new Rpc();
        }
        return m_rpc;
    }

    virtual ~Rpc();

    /// @brief Rpc初始化接口
    /// @param app_id 程序唯一id，三段表示，如"10.10.12"\n
    ///         第一段表示游戏分区id，占用10比特，取值范围0~1023\n
    ///         第二段表示游戏server id，占用10比特，取值范围0~1023\n
    ///         第三段表示server实例id，占用12比特，取值范围0~4095\n
    /// @param game_id 游戏在运营系统注册后返回的唯一id
    /// @param game_key 游戏在运营系统注册后返回的key，请妥善保存，避免泄露
    /// @return 0 成功
    /// @return 非0 失败，初始化失败Rpc不能正常工作
    int Init(const std::string& app_id,
            int64_t game_id,
            const std::string& game_key);

    /// @brief 设置服务管理(zoo keeper)地址
    /// @param zk_url 服务管理地址，如"127.0.0.1:1888"
    /// @param time_out_ms 连接超时时间，单位ms，默认为20s
    /// @note time_out_ms < 10ms时设置无效，使用默认值
    void SetZooKeeperAddress(const std::string& zk_url, int time_out_ms = 20000);

    /// @brief 注册网络事件处理回调
    /// @param cbs 网络事件回调函数接口
    void RegisterEventhandler(const EventCallbacks& cbs);

    /// @brief 注册服务，将用户实现的服务注册到Rpc中
    /// @param service 实现了IDL生成服务接口的对象指针，对象指针内存由用户管理
    /// @param route_type 服务的路由策略，支持3种路由方式(默认取值为1):\n
    ///                1:    轮询路由
    ///                2:    哈希路由
    ///                3:    取模路由
    /// @return 0 成功
    /// @return 非0 失败
    template<typename Class>
    int RegisterService(Class* service, int route_type = 1);

    /// @brief 注册服务，将用户实现的服务注册到Rpc中
    /// @param custom_service_name 自定义服务名
    /// @param service 实现了IDL生成服务接口的对象指针，对象指针内存由用户管理
    /// @param route_type 服务的路由策略，支持3种路由方式(默认取值为1):\n
    ///                1:    轮询路由
    ///                2:    哈希路由
    ///                3:    取模路由
    /// @return 0 成功
    /// @return 非0 失败
    template<typename Class>
    int RegisterService(const std::string &custom_service_name, Class* service, int route_type = 1);

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
                                PROTOCOL_TYPE protocol_type,
                                bool register_2_zk = true);

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
                                PROTOCOL_TYPE protocol_type);

    /// @brief Rpc消息处理，一般在客户端侧应用，需要用户循环调用
    /// @param max_event 每次处理最大消息数，默认100
    /// @param cur_time_ms 从1970年至今的毫秒数(ms)，默认-1(由Rpc自己获取)
    /// @return 本次调用实际处理的消息数
    int Update(int max_event = 100, int64_t cur_time_ms = -1);

    /// @brief Rpc服务，一般在服务端侧应用，Rpc自己的消息处理循环，当没有消息处理时会sleep 10ms
    /// @note 此函数为阻塞，当注册服务失败时，函数立即结束
    void Serve();

    /// @brief 获取当前连接对象，只在用户的具体服务实现函数中调用有效
    /// @param connection_obj 输出参数，将当前的连接信息写到connection_obj中
    /// @return 0 获取成功
    /// @return 非0 获取失败
    virtual int GetCurConnectionObj(ConnectionObj* connection_obj);

    /// @brief 获取当前请求的序号
    /// @return 当前请求的序号
    uint64_t GetSequence();


    //////////////////////////////////////////////////////////////////////////////////
    //////////// 以下接口为内部接口，供上层game server框架以及桩代码调用  ////////////
    //////////////////////////////////////////////////////////////////////////////////

    int SetConnectionIdleTime(int idle_timeout_s);

    // @brief 向服务管理系统注册服务实例
    // @note 内部使用
    int RegisterServiceInstance();

    bool IsValidserviceName(const char *service_name);

    // @brief 向Rpc注册服务实现
    // @note 内部使用
    int RegisterService(const char *custom_service_name,
        cxx::shared_ptr<processor::TAsyncProcessor> service, int route_type);

    // @brief 根据service url和protocol type获取协议对象
    // @note 内部使用
    cxx::shared_ptr<protocol::TProtocol> GetProtocol(const std::string& service_url,
        const std::string& service_name, PROTOCOL_TYPE protocol_type);

    cxx::shared_ptr<protocol::TProtocol> GetProtocol(const std::string& zk_url,
        const std::string& service_name, int unit_id);

    // @brief 根据connection obj获取协议对象
    // @note 内部使用
    cxx::shared_ptr<protocol::TProtocol> GetProtocol(const ConnectionObj& conn_obj);

    // @brief stub析构时需要通知RpcClient释放自己获取的协议对象
    // @note 内部使用
    void FreeProtocol(cxx::shared_ptr<protocol::TProtocol> protocol);

    // @brief 阻塞操作
    // @note 内部使用
    virtual int Block(protocol::TProtocol** prot, int timeout_ms);

    // @brief 异步调用会话管理
    // @note 内部使用
    void AddSession(
        cxx::function<void(int ret, protocol::TProtocol* prot)> recv_cob,
        int timeout_ms);

    // @brief 生成请求序号
    // @note 内部使用
    uint64_t GenSequence();

    // @brief 获取zk服务地址，桩代码使用，返回地址携带"zk://"
    // @note 内部使用
    std::string GetZooKeeperAddress();

    // @brief 通过监听获取handle
    // @note 内部使用
    int GetHandleByListenUrl(const std::string &listen_url, int *handle);

    // @brief 通过handle获取监听地址
    // @note 内部使用
    int GetListenUrlByHandle(int handle, std::string *listen_url);

    // @brief 恢复协议
    // @note 内部使用
    int RestoreProtocol(int handle, uint64_t peer_addr, int32_t *protocol_type);

protected:
    int ProcessMessage(int handle, cxx::shared_ptr<protocol::TProtocol> prot);

    // @brief PebbleServer框架在协程中处理请求
    virtual int ProcessRequest(const std::string& name, int64_t seqid,
                            cxx::shared_ptr<protocol::TProtocol> protocol);

    // @brief PebbleServer框架在协程中处理请求
    int ProcessRequestImp(const std::string& name, int64_t seqid,
                            cxx::shared_ptr<protocol::TProtocol> protocol);

private:
    int Fini();

    int InitServiceManager(const std::string& service_url);

    int InitHttpDriver();

    int InitTcpDriver();

    int OnMessage(int handle, const char* msg, size_t msg_len, uint64_t addr);

    int OnUnknowMessage(int handle, const char* msg, size_t msg_len, uint64_t addr);

    int OnConnect(int handle, uint64_t addr);

    int OnClose(int handle, uint64_t addr, bool is_idle);

    int OnIdle(int handle, uint64_t addr, int64_t session_id);

    void OnServiceAddressChange(std::map<std::string,
            std::vector<cxx::shared_ptr<protocol::TProtocol> > > *service_name_to_protocols,
            const std::string& service_name,
            int service_route,
            const std::vector<service::ServiceAddressInfo>& address);

    void UpdateServiceAddr(cxx::shared_ptr<protocol::TProtocol> prot,
                                std::vector<service::ServiceAddressInfo>& new_addrs);

    int ProcessResponse(protocol::TMessageType mtype, int64_t seqid,
                            protocol::TProtocol* protocol, bool is_sync = false);

    void ProcessTimeout();

    void RequestComplete(bool ok);

    void FreeProtocol();

    int GetServiceAddress(const std::string& zk_url,
        const std::string& service_name, std::vector<std::string>* addresses,
        std::vector<int>* types, int* route_type);

    void AddService2WatchList(std::map<std::string,
            std::vector<cxx::shared_ptr<protocol::TProtocol> > > *service_name_to_protocols,
            const std::string& service_name,
                                    cxx::shared_ptr<protocol::TProtocol> prot);

    bool RmvProtocol4WatchList(std::map<std::string,
            std::vector<cxx::shared_ptr<protocol::TProtocol> > > *service_name_to_protocols,
            cxx::shared_ptr<protocol::TProtocol> prot);

    bool AccessCheck(int handle, const std::string& service_name);

    int CreateClientConnection(std::string url,
        PROTOCOL_TYPE protocol_type,
        cxx::shared_ptr<protocol::GroupProtocol>& group_prot);

    int DeleteClientConnection(int handle);

    int AddServiceManner(const std::string& listen_url,
                                PROTOCOL_TYPE protocol_type,
                                const std::vector<std::string>* white_list,
                                const std::vector<std::string>* black_list,
                                bool register_2_zk = true);

    int AddServiceManner(const std::string& addr_in_docker,
                                const std::string& addr_in_host,
                                PROTOCOL_TYPE protocol_type,
                                const std::vector<std::string>* white_list,
                                const std::vector<std::string>* black_list);

    cxx::shared_ptr<protocol::TProtocol> GetBroadCastProtocol(
        const std::string& service_url,
        const std::string& service_name,
        PROTOCOL_TYPE protocol_type);

private:
    // 异步请求session定义
    class CobSession {
    public:
        CobSession() {
            m_req_id = 0;
            m_time = 0;
        }
        ~CobSession() {}

        uint64_t m_req_id;
        // 响应消息的接收处理函数
        cxx::function<void(int ret, protocol::TProtocol* prot)> recv_cob;
        int64_t m_time; // 超时时间
    };

    // 连接数据，和一个handle一一对应
    // client端，一个handle对应一个连接，server端，一个handle对应n个连接
    // 每个连接对应一个protocol对象
    class ConnectionData {
    public:
        ConnectionData() {
            m_server = false;
            m_prot_type = PROTOCOL_BUTT;
        }
        ~ConnectionData() {}

        bool m_server;
        PROTOCOL_TYPE m_prot_type;
        std::map< uint64_t, cxx::shared_ptr<protocol::TProtocol> > m_protocols;
    };

    class ListenData {
    public:
        ListenData() {
            m_register_2_zk = true;
            m_prot_type = PROTOCOL_BINARY;
        }
        ~ListenData() {}

        // 性能优化
        bool AllowAccess(const std::string& service) {
            for (std::vector<std::string>::iterator it = m_black_list.begin();
                it != m_black_list.end(); ++it) {
                if (service == *it) {
                    return false;
                }
            }

            if (m_white_list.empty()) {
                return true;
            }

            for (std::vector<std::string>::iterator it = m_white_list.begin();
                it != m_white_list.end(); ++it) {
                if (service == *it) {
                    return true;
                }
            }

            return false;
        }

        std::string m_listen_url;
        PROTOCOL_TYPE m_prot_type;
        std::vector<std::string> m_white_list;
        std::vector<std::string> m_black_list;
        bool m_register_2_zk;
    };

    class ServiceInfo {
    public:
        ServiceInfo() { m_route_type = 1; }
        ~ServiceInfo() {}
        cxx::shared_ptr<processor::TAsyncProcessor> m_processer;
        int m_route_type;
        std::set<std::string> m_service_name;
    };

    class ServiceManagerInfo {
    public:
        explicit ServiceManagerInfo(pebble::service::ServiceManager *sm) : service_manager(sm) {
        }
        pebble::service::ServiceManager *service_manager;
        std::map<std::string,
            std::vector<cxx::shared_ptr<protocol::TProtocol> > > service_name_to_protocols;
    };

    class InstanceAutoFree {
    public:
        ~InstanceAutoFree() {
            if (Rpc::m_rpc) {
                delete Rpc::m_rpc;
                Rpc::m_rpc = NULL;
            }
        }
    };
    static InstanceAutoFree m_instance_auto_free;

private:
    // 基础参数
    std::string m_app_id;
    int64_t m_game_id;
    std::string m_game_key;

    // 服务管理相关
    std::string m_service_url;
    int m_connect_timeout_ms;
    pebble::service::ServiceManager* m_service_manager;

    // 网络层相关
    net::NetEventCallbacks* m_event_cbs;

    net::HttpDriver* m_http_driver;

    net::TcpDriver* m_tcp_driver;

    net::ConnectionMgr *m_conn_mgr;

    // 连接管理相关
    std::map<int, ListenData*> m_listen_data;

    std::map<int, ConnectionData*> m_connections;

    std::map<std::string, std::vector< cxx::shared_ptr<protocol::TProtocol> > > m_wathed_service;

    std::map<std::string, ServiceInfo*> m_services;

    ConnectionObj m_cur_conn_obj;

    // 资源管理相关
    uint64_t m_seq;

    std::map<uint64_t, CobSession> m_sessions;

    std::vector< cxx::shared_ptr<protocol::TProtocol> > m_free_protocols;

    // 同步请求处理
    bool m_syn_proc;

    int m_syn_result;

    uint64_t m_syn_seq;

    protocol::TProtocol* m_syn_prot;

    // 上层回调
    EventCallbacks m_conn_event_cbs;

    std::map<std::string, ServiceManagerInfo*> m_unit_to_service_manager;

    std::map<std::string, int> m_url_to_handle;
    std::map<int, std::string> m_handle_to_url;

private:
    ServiceManagerInfo * GetServiceManagerInfo(const std::string& zk_url, int unit_id);
};

template<typename Class> class GetProcessor;

template<typename Class>
int Rpc::RegisterService(Class* service, int route_type) {
    if (NULL == service) {
        return -1;
    }
    return RegisterService(NULL,
        GetProcessor<typename Class::__InterfaceType>()(service), route_type);
}

template<typename Class>
int Rpc::RegisterService(const std::string &custom_service_name, Class* service, int route_type) {
    if (NULL == service) {
        return -1;
    }
    return RegisterService(custom_service_name.c_str(),
        GetProcessor<typename Class::__InterfaceType>()(service), route_type);
}

}  // namespace rpc
}  // namespace pebble

#endif  // PEBBLE_RPC_RPC_H

