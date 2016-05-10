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


#include "source/broadcast/channel_mgr.h"
#include "source/common/mutex.h"
#include "source/common/string_utility.h"
#include "source/common/time_utility.h"
#include "source/message/connection_mgr.h"
#include "source/message/http_driver.h"
#include "source/message/message.h"
#include "source/message/tcp_driver.h"
#include "source/rpc/common/rpc_define.h"
#include "source/rpc/common/rpc_error_info.h"
#include "source/rpc/processor/async_dispatch_processor.h"
#include "source/rpc/protocol/group_protocol.h"
#include "source/rpc/protocol/protocol_factory.h"
#include "source/rpc/rpc.h"
#include "source/service/service_manage.h"
#include "source/rpc/transport/msg_buffer.h"
#include <sstream>
#include <algorithm>


namespace pebble {
namespace rpc {

ConnectionObj::ConnectionObj() {
    m_handle = -1;
    m_peer_addr = 0;
    m_protocol_type = PROTOCOL_BINARY;
    m_session_id = -1;
}

ConnectionObj::~ConnectionObj() {}

std::string ConnectionObj::GetOriginalAddr() {
    // TODO(tatezhang) 需要NET库提供反查接口
    return "";
}

void ConnectionObj::Set(int handle, uint64_t peer_addr,
        int32_t protocol_type, int64_t session_id) {
    m_handle = handle;
    m_peer_addr = peer_addr;
    m_protocol_type = protocol_type;
    m_session_id = session_id;
}

bool ConnectionObj::Restore(const std::string &restore_url,
        uint64_t peer_addr, int64_t session_id) {
    if (Rpc::Instance()->GetHandleByListenUrl(restore_url, &m_handle) != 0) {
        return false;
    }

    pebble::net::MessageDriver *driver = pebble::net::Message::QueryDriver(m_handle);
    if (driver && driver->Name() != "pipe") {
        return false;
    }

    if (Rpc::Instance()->RestoreProtocol(m_handle, peer_addr, &m_protocol_type) != 0) {
        return false;
    }

    m_peer_addr = peer_addr;
    m_session_id = session_id;

    return true;
}

bool ConnectionObj::GetRestoreUrl(std::string * restore_url) {
    return Rpc::Instance()->GetListenUrlByHandle(m_handle, restore_url) == 0;
}

int ConnectionObj::GetHandle() const {
    return m_handle;
}

uint64_t ConnectionObj::GetPeerAddr() const {
    return m_peer_addr;
}

int32_t ConnectionObj::GetProtocolType() const {
    return m_protocol_type;
}

int64_t ConnectionObj::GetSessionId() const {
    return m_session_id;
}

ConnectionObj& ConnectionObj::operator = (const ConnectionObj& conn_obj) {
    this->m_handle = (const_cast<ConnectionObj&>(conn_obj)).GetHandle();
    this->m_peer_addr = (const_cast<ConnectionObj&>(conn_obj)).GetPeerAddr();
    this->m_protocol_type = (const_cast<ConnectionObj&>(conn_obj)).GetProtocolType();
    this->m_session_id = (const_cast<ConnectionObj&>(conn_obj)).GetSessionId();
    return *this;
}


Rpc* Rpc::m_rpc = NULL;
Rpc::InstanceAutoFree Rpc::m_instance_auto_free;


/// 地址解析可以支持多种形式:
/*
  明确服务管理和地址相关的配置要求:
  1、创建server instance时需指定此instance支持的所有传输地址，以','分开
  2、指定的传输地址需要携带前缀，如"tcp://"
  示例:
  tcp://127.0.0.1:8888,tcp://127.0.0.1:9999; // 多个地址带前缀
  tcp://127.0.0.1:8888,tbus://1.2.2.1:9999;  // 不同地址类型
 */

static const char kTBusPrefix[] = "tbus://";        // tbus://1.2.2.1:8888
static const char kTcpPrefix[] = "tcp://";          // tcp://127.0.0.1:8888
static const char kZkPrefix[] = "zk://";            // zk://127.0.0.1:8888
static const char kHttpPrefix[] = "http://";        // http://127.0.0.1:8888
static const char kPipePrefix[] = "pipe://";        // pipe://127.0.0.1:8888
static const char kGcpPrefix[] = "gcp://";        // pipe://127.0.0.1:8888
static const char kBroadcastPrefix[] = "broadcast://"; // broadcast://channel_name


Rpc::Rpc() {
    m_service_manager    = NULL;
    m_event_cbs          = NULL;
    m_http_driver        = NULL;
    m_tcp_driver         = NULL;
    m_seq                = 0;
    m_syn_proc           = false;
    m_syn_seq            = 0;
    m_syn_result         = 0;
    m_syn_prot           = NULL;
    m_game_id            = 0;
    m_connect_timeout_ms = 20000;
    m_conn_mgr           = NULL;
}

Rpc::~Rpc() {
    Fini();
}

int Rpc::Init(const std::string& app_id, int64_t game_id, const std::string& game_key) {
    m_app_id      = app_id;
    m_game_id     = game_id;
    m_game_key    = game_key;

    net::OnMessage recv_func = cxx::bind(&Rpc::OnMessage, this, cxx::placeholders::_1,
        cxx::placeholders::_2, cxx::placeholders::_3, cxx::placeholders::_4);

    net::OnPeerConnected conn_func = cxx::bind(&Rpc::OnConnect, this, cxx::placeholders::_1,
        cxx::placeholders::_2);

    net::OnPeerClosed close_func = cxx::bind(&Rpc::OnClose, this, cxx::placeholders::_1,
        cxx::placeholders::_2, false);

    net::OnMessage unknow_msg = cxx::bind(&Rpc::OnUnknowMessage, this, cxx::placeholders::_1,
        cxx::placeholders::_2, cxx::placeholders::_3, cxx::placeholders::_4);

    if (!m_event_cbs) {
        m_event_cbs = new net::NetEventCallbacks();
    }
    if (!m_event_cbs) {
        PLOG_ERROR("new NetEventCallbacks failed.");
        return -1;
    }
    m_event_cbs->on_message = recv_func;
    m_event_cbs->on_peer_connected = conn_func;
    m_event_cbs->on_peer_closed = close_func;
    m_event_cbs->on_invalid_message = unknow_msg;

    if (!m_conn_mgr) {
        m_conn_mgr = new net::ConnectionMgr();
        net::ConnectionEvent conn_evt;
        conn_evt.onIdle = cxx::bind(&Rpc::OnIdle, this,
            cxx::placeholders::_1, cxx::placeholders::_2, cxx::placeholders::_3);
        m_conn_mgr->RegisterEventCb(conn_evt);
    }

    ErrorInfo::Init();

    int ret = InitServiceManager(m_service_url);
    if (ret != ErrorInfo::kRpcNoRrror) {
        return ret;
    }

    // 通信层dirver的初始化原则是只有监听了地址才注册 ?
    ret = InitHttpDriver();
    if (ret != 0) {
        PLOG_ERROR("http driver init failed(%d)", ret);
        return ret;
    }

    ret = InitTcpDriver();
    if (ret != 0) {
        PLOG_ERROR("tcp driver init failed(%d)", ret);
        return ret;
    }

    return ErrorInfo::kRpcNoRrror;
}

void Rpc::SetZooKeeperAddress(const std::string& zk_url, int time_out_ms) {
    m_service_url = zk_url;
    if (time_out_ms >= 10) {
        m_connect_timeout_ms = time_out_ms;
    }
}

void Rpc::RegisterEventhandler(const EventCallbacks& cbs) {
    m_conn_event_cbs = cbs;
}

int Rpc::AddServiceManner(const std::string& listen_url,
                                PROTOCOL_TYPE protocol_type,
                                bool register_2_zk) {
    return AddServiceManner(listen_url, protocol_type, NULL, NULL, register_2_zk);
}

int Rpc::AddServiceManner(const std::string& addr_in_docker,
                                const std::string& addr_in_host,
                                PROTOCOL_TYPE protocol_type)
{
    return AddServiceManner(addr_in_docker, addr_in_host, protocol_type, NULL, NULL);
}

int Rpc::Update(int max_event, int64_t cur_time_ms) {

    int num = net::Message::Poll(*m_event_cbs, max_event);

    ProcessTimeout();

    if (m_service_manager) {
        m_service_manager->Update();
    }

    m_conn_mgr->Update(cur_time_ms);

    FreeProtocol();

    return num;
}

void Rpc::Serve() {
    int ret = RegisterServiceInstance();
    if (ret != 0) {
        PLOG_FATAL("register service failed(%d)", ret);
        return;
    }

    int num = 0;
    do {
        num = Update();

        if (num < 1) {
            usleep(10 * 1000);
        }
    } while (true);
}

int Rpc::GetCurConnectionObj(ConnectionObj* connection_obj) {
    if (NULL == connection_obj) {
        PLOG_ERROR("para is NULL");
        return -1;
    }

    *connection_obj = m_cur_conn_obj;
    return 0;
}

uint64_t Rpc::GetSequence() {
    return m_seq;
}

int Rpc::RegisterServiceInstance() {
    if (!m_service_manager) {
        if (m_service_url.empty()) {
            return 0;
        }

        InitServiceManager(m_service_url);
        if (!m_service_manager) {
            return ErrorInfo::kCreateServiceInstanceFailed;
        }
    }

    int ret = 0;
    std::vector<service::ServiceAddressInfo> addresses;
    std::map<std::string, ServiceInfo*>::iterator it;
    for (it = m_services.begin(); it != m_services.end(); ++it) {
        // 每个服务根据白名单、黑名单计算对外暴露的地址列表
        addresses.clear();
        for (std::map<int, ListenData*>::iterator lit = m_listen_data.begin();
            lit != m_listen_data.end(); ++lit) {
            if (!((lit->second)->m_register_2_zk)) {
                continue;
            }
            if (!(lit->second)->AllowAccess(it->first)) {
                continue;
            }
            service::ServiceAddressInfo addr_info(
                (lit->second)->m_listen_url, (lit->second)->m_prot_type);
            addresses.push_back(addr_info);
        }

        // 没有地址需要注册就不注册
        if (addresses.empty()) { continue; }

        pebble::service::ServiceRouteType route_type =
            static_cast<pebble::service::ServiceRouteType>((it->second)->m_route_type);
        for (std::set<std::string>::iterator sniter = it->second->m_service_name.begin();
                sniter != it->second->m_service_name.end(); sniter++) {
            ret = m_service_manager->CreateServiceInstance(*sniter,
                    route_type, addresses);
            if (ret != pebble::service::kSM_OK) {
                PLOG_ERROR("create service instance faild. ret = %d.", ret);
                return ret;
            }
        }
    }

    return 0;
}

bool Rpc::IsValidserviceName(const char *service_name) {
    if (!service_name) {
        return false;
    }

    size_t service_name_len = strlen(service_name);
    if (service_name_len < 1 || service_name_len > 128) {
        return false;
    }

    for (size_t i = 0; i < service_name_len; i++) {
        char c = service_name[i];
        if ((c < '0' || (c > '9' && c < 'A') || (c > 'Z' && c < 'a') || c > 'z') && c != '_') {
            return false;
        }
    }

    return true;
}

int Rpc::RegisterService(const char *custom_service_name,
    cxx::shared_ptr<processor::TAsyncProcessor> service, int route_type) {
    std::string service_name(service->getServiceName());
    if (custom_service_name) {
        if (!IsValidserviceName(custom_service_name)) {
            PLOG_ERROR("service name is invalid!");
            return -1;
        }
        service_name = custom_service_name;
    }

    if (service_name.empty()) {
        PLOG_ERROR("service name is NULL!");
        return -1;
    }

    // 有继承关系的service分开
    std::vector<std::string> vec;
    StringUtility::Split(service->getServiceName(), ";", &vec);
    for (std::vector<std::string>::iterator it = vec.begin(); it != vec.end(); ++it) {
        ServiceInfo* service_info = NULL;

        if (m_services.find(*it) != m_services.end()) {
            service_info = m_services[*it];
            PLOG_ERROR("service name repeated(%s)!", (*it).c_str());
        } else {
            service_info = new ServiceInfo();
        }

        // 服务名重复时，新的processor覆盖旧的
        service_info->m_processer = service;
        service_info->m_route_type = route_type;
        if (it != vec.begin()) { // 父服务
            service_info->m_service_name.insert(*it);
        } else {
            service_info->m_service_name.insert(custom_service_name ? service_name : *it);
        }
        m_services[*it] = service_info;
    }

    return 0;
}

cxx::shared_ptr<protocol::TProtocol> Rpc::GetProtocol(
                                    const std::string& service_url,
                                    const std::string& service_name,
                                    PROTOCOL_TYPE protocol_type) {
    if (StringUtility::StartsWith(service_url, kBroadcastPrefix)) {
        return GetBroadCastProtocol(service_url, service_name, protocol_type);
    }

    std::vector<std::string> urls;
    std::vector<int> types;
    int watch = 0;

    // 解析url
    int route_type = pebble::service::kRoundRoute;
    if (StringUtility::StartsWith(service_url, kZkPrefix)) {
        // 首次连接主动获取服务地址，后续通过watch通知刷新
        std::string zk_url = service_url.substr(strlen(kZkPrefix));
        int ret = GetServiceAddress(zk_url, service_name, &urls, &types, &route_type);
        if (ret != 0) {
            PLOG_ERROR("get service addr failed %d", ret);
        }
        watch = 1;
    } else {
        StringUtility::Split(service_url, ",", &urls);
    }

    // 定义group protocol，客户端统一使用group protocol
    cxx::shared_ptr<protocol::GroupProtocol> group_prot;

    int i = 0;
    PROTOCOL_TYPE type;
    for (std::vector<std::string>::iterator it = urls.begin(); it != urls.end(); ++it) {
        if (watch && StringUtility::StartsWith(*it, kPipePrefix)) {
            // 从zk上拉取的服务地址需要剔除pipe，pipe地址必须通过pipe接入
            // 指定地址不剔除，方便测试
            continue;
        }

        if (types.empty()) {
            type = protocol_type;
        } else {
            type = static_cast<PROTOCOL_TYPE>(types[i]);
            i++;
        }

        CreateClientConnection(*it, type, group_prot);
    }

    if (watch) {
        if (!group_prot) {
            group_prot.reset(new protocol::GroupProtocol());
        }
        AddService2WatchList(&m_wathed_service, service_name, group_prot);
    }

    if (group_prot) {
        group_prot->setRouteType(route_type);
    }

    return group_prot;
}

cxx::shared_ptr<protocol::TProtocol> Rpc::GetProtocol(const std::string& zk_url,
        const std::string& service_name, int unit_id) {
    cxx::shared_ptr<protocol::GroupProtocol> group_prot;

    Rpc::ServiceManagerInfo *service_manager_info = GetServiceManagerInfo(zk_url, unit_id);
    if (service_manager_info) {
        group_prot.reset(new protocol::GroupProtocol());
        pebble::service::ServiceManager *service_manager = service_manager_info->service_manager;
        int route_type = 0;
        std::vector<service::ServiceAddressInfo> addr_info;
        int ret = service_manager->GetServiceAddress(service_name, &route_type, &addr_info);
        service_manager->WatchServiceAddress(service_name);
        if (service::kSM_OK == ret) {
            std::vector<service::ServiceAddressInfo>::iterator it;
            for (it = addr_info.begin(); it != addr_info.end(); ++it) {
                CreateClientConnection(it->address_url,
                        static_cast<PROTOCOL_TYPE>(it->protocol_type), group_prot);
            }
        }
        AddService2WatchList(&(service_manager_info->service_name_to_protocols),
                service_name, group_prot);

        if (route_type <= pebble::service::kUnknownRoute ||
            route_type >= pebble::service::kMAXRouteVal) {
            route_type = pebble::service::kRoundRoute;
        }
        group_prot->setRouteType(route_type);
    }
    return group_prot;
}

Rpc::ServiceManagerInfo * Rpc::GetServiceManagerInfo(const std::string& zk_url, int unit_id) {
    std::string zk_addr = zk_url;
    std::transform(zk_addr.begin(), zk_addr.end(), zk_addr.begin(), ::tolower);
    if (StringUtility::StartsWith(zk_addr, kZkPrefix)) {
        zk_addr = zk_addr.substr(strlen(kZkPrefix));
    }
    std::stringstream ss;
    ss << zk_addr << unit_id;
    std::string sm_key = ss.str();

    std::map<std::string, ServiceManagerInfo*>::iterator iter =
        m_unit_to_service_manager.find(sm_key);

    if (iter == m_unit_to_service_manager.end()) {
        pebble::service::ServiceManager *service_manager = new pebble::service::ServiceManager();
        if (!service_manager) {
            return NULL;
        }

        int ret = service_manager->Init(zk_addr, 5000);
        if (ret != pebble::service::kSM_OK) {
            PLOG_ERROR("service manager init failed %s,%d.", zk_url.c_str(), ret);

            ErrorInfo::SetErrorCode(ErrorInfo::kInitZKFailed);
            service_manager->Fini();
            delete service_manager;

            return NULL;
        }

        ret = service_manager->SetAppInfo(m_game_id, m_game_key, unit_id);
        if (ret != pebble::service::kSM_OK) {
            PLOG_ERROR("service manager set app info failed %d.", ret);
            service_manager->Fini();
            delete service_manager;
            return NULL;
        }

        ServiceManagerInfo * service_manager_info = new ServiceManagerInfo(service_manager);
        if (!service_manager_info) {
            PLOG_ERROR("new ServiceManagerInfo failed.");
            service_manager->Fini();
            delete service_manager;
            return NULL;
        }

        service::CbServiceAddrChanged func = cxx::bind(&Rpc::OnServiceAddressChange,
                this, &(service_manager_info->service_name_to_protocols),
                cxx::placeholders::_1, cxx::placeholders::_2, cxx::placeholders::_3);
        service_manager->RegisterWatchFunc(func);

        m_unit_to_service_manager[sm_key] = service_manager_info;
        return service_manager_info;
    } else {
        return iter->second;
    }
}

cxx::shared_ptr<protocol::TProtocol> Rpc::GetProtocol(const ConnectionObj& conn_obj) {
    cxx::shared_ptr<protocol::TProtocol> prot;

    int handle = (const_cast<ConnectionObj&>(conn_obj)).GetHandle();
    uint64_t peer_addr = (const_cast<ConnectionObj&>(conn_obj)).GetPeerAddr();

    // http暂不支持反向rpc
    std::map<int, ListenData*>::iterator lit = m_listen_data.find(handle);
    if (m_listen_data.end() == lit) {
        PLOG_ERROR("unknown handle(%d, %lu)", handle, peer_addr);
        return prot;
    }

    if (StringUtility::StartsWith((lit->second)->m_listen_url, kHttpPrefix)) {
        PLOG_ERROR("http unsupport rerpc(%d, %lu)", handle, peer_addr);
        return prot;
    }

    std::map<int, ConnectionData*>::iterator it = m_connections.find(handle);
    if (m_connections.end() == it) {
        PLOG_ERROR("unknown handle(%d, %lu)", handle, peer_addr);
        return prot;
    }

    if (!((it->second)->m_server)) {
        PLOG_ERROR("conn is not server(%d, %lu)", handle, peer_addr);
        return prot;
    }

    std::map< uint64_t, cxx::shared_ptr<protocol::TProtocol> >::iterator mit;
    mit = ((it->second)->m_protocols).find(peer_addr);
    if (((it->second)->m_protocols).end() == mit) {
        PLOG_ERROR("unknown addr(%d, %lu)", handle, peer_addr);
        return prot;
    }

    return mit->second;
}

void Rpc::FreeProtocol(cxx::shared_ptr<protocol::TProtocol> protocol) {
    // client对象销毁时需要释放框架创建的资源，交由框架处理
    if (!protocol) {
        return;
    }
    m_free_protocols.push_back(protocol);
}

int Rpc::Block(protocol::TProtocol** prot, int timeout_ms) {

    if (timeout_ms <= 0) {
        timeout_ms = kRequestTimeOutMs;
    }

    int64_t begin = TimeUtility::GetCurremtMs();
    int64_t end = begin;
    int num = 0;

    m_syn_proc = true;
    m_syn_seq = GetSequence();
    m_syn_result = ErrorInfo::kRpcNoRrror;
    m_syn_prot = NULL;
    do {
        end = TimeUtility::GetCurremtMs();

        num = Update(1);
        if (!m_syn_proc) {
            *prot = m_syn_prot;
            return m_syn_result;
        }

        if (num < 1) {
            usleep(10 * 1000);
        }
    } while (end - begin < timeout_ms);
    m_syn_proc = false;

    return ErrorInfo::kRpcTimeoutError;
}

void Rpc::AddSession(
    cxx::function<void(int ret, protocol::TProtocol* prot)> recv_cob, int timeout_ms) {
    if (timeout_ms <= 0) {
        timeout_ms = kRequestTimeOutMs;
    }
    CobSession session;
    session.m_req_id = GetSequence();
    session.recv_cob = recv_cob;
    session.m_time   = TimeUtility::GetCurremtMs() + timeout_ms;

    m_sessions[session.m_req_id] = session;
}

uint64_t Rpc::GenSequence() {
    return ++m_seq;
}

int Rpc::ProcessRequest(const std::string& name, int64_t seqid,
                            cxx::shared_ptr<protocol::TProtocol> protocol) {
    return ProcessRequestImp(name, seqid, protocol);
}

int Rpc::ProcessRequestImp(const std::string& name, int64_t seqid,
                            cxx::shared_ptr<protocol::TProtocol> protocol) {
    // ProcessRequestImp 可能在协程中单独调用，异常需要自己闭环
    TApplicationException::TApplicationExceptionType ex_type = TApplicationException::UNKNOWN;
    std::string ex_msg;
    try {
        std::vector<std::string> vec;
        StringUtility::Split(name, ":", &vec);
        if (vec.size() != 2) {
            throw TApplicationException(TApplicationException::PROTOCOL_ERROR,
                "invalid name format(service name:function name) : " + name);
        }

        std::map<std::string, ServiceInfo*>::iterator it;
        it = m_services.find(vec[0]);
        if (it != m_services.end()) {
            cxx::function<void(bool ok)> cob = cxx::bind(
                &Rpc::RequestComplete, this, cxx::placeholders::_1);
            ((it->second)->m_processer)->process(cob, protocol, vec[1], seqid);
            return 1;
        }

        throw TApplicationException(TApplicationException::UNKNOWN_SERVICE,
                "unknown service name : " + vec[0]);
    } catch (transport::TTransportException te) {
        ex_type = TApplicationException::TRANSPORT_EXCEPTION;
        ex_msg = te.what();
    } catch (protocol::TProtocolException pe) {
        ex_type = TApplicationException::PROTOCOL_EXCEPTION;
        ex_msg = pe.what();
    } catch (TApplicationException ae) {
        ex_type = ae.getType();
        ex_msg = ae.what();
    } catch (TException e) {
        ex_msg = e.what();
    }

    PLOG_ERROR("process msg failed: %s", ex_msg.c_str());

    try {
        protocol->getTransport()->readEnd();

        protocol->writeMessageBegin(name, protocol::T_EXCEPTION, seqid);
        TApplicationException ex(ex_type, ex_msg);
        ex.write(protocol.get());
        protocol->writeMessageEnd();
        protocol->getTransport()->writeEnd();
        protocol->getTransport()->flush();
    } catch (TException e) {
        ex_msg = e.what();
        PLOG_ERROR("exception(%s)", ex_msg.c_str());
    }

    return 1;
}

std::string Rpc::GetZooKeeperAddress() {
    return (kZkPrefix + m_service_url);
}

int Rpc::Fini() {
    if (m_event_cbs) {
        delete m_event_cbs;
        m_event_cbs = NULL;
    }

    if (m_http_driver) {
        delete m_http_driver;
        m_http_driver = NULL;
    }

    if (m_tcp_driver) {
        delete m_tcp_driver;
        m_tcp_driver = NULL;
    }

    if (m_service_manager) {
        m_service_manager->Fini();
        delete m_service_manager;
        m_service_manager = NULL;
    }

    for (std::map<int, ConnectionData*>::iterator it = m_connections.begin();
        it != m_connections.end(); ++it) {
        delete it->second;
        it->second = NULL;
    }

    for (std::map<int, ListenData*>::iterator it = m_listen_data.begin();
        it != m_listen_data.end(); ++it) {
        delete it->second;
        it->second = NULL;
    }

    for (std::map<std::string, ServiceInfo*>::iterator it = m_services.begin();
        it != m_services.end(); ++it) {
        delete it->second;
        it->second = NULL;
    }

    for (std::map<std::string, ServiceManagerInfo*>::iterator it =
            m_unit_to_service_manager.begin();
            it != m_unit_to_service_manager.end(); ++it) {
        it->second->service_manager->Fini();
        delete it->second->service_manager;
    }

    if (m_conn_mgr) {
        delete m_conn_mgr;
        m_conn_mgr = NULL;
    }

    return 0;
}

int Rpc::InitServiceManager(const std::string& service_url) {
    if (service_url.empty()) {
        // url为空时不创建
        return ErrorInfo::kRpcNoRrror;
    }
    if (m_service_manager) {
        return ErrorInfo::kRpcNoRrror;
    }

    m_service_manager = new pebble::service::ServiceManager();
    if (!m_service_manager) {
        return ErrorInfo::kInitZKFailed;
    }

    int ret = m_service_manager->Init(service_url, m_connect_timeout_ms);
    if (ret != pebble::service::kSM_OK) {
        PLOG_ERROR("service manager init failed %s,%d.", service_url.c_str(), ret);

        ErrorInfo::SetErrorCode(ErrorInfo::kInitZKFailed);

        delete m_service_manager;
        m_service_manager = NULL;

        return ErrorInfo::kInitZKFailed;
    }

    ret = m_service_manager->SetAppInfo(m_game_id, m_game_key, m_app_id);
    if (ret != pebble::service::kSM_OK) {
        PLOG_ERROR("service manager set app info failed %d.", ret);

        delete m_service_manager;
        m_service_manager = NULL;
        return ErrorInfo::kInitZKFailed;
    }

    service::CbServiceAddrChanged func = cxx::bind(&Rpc::OnServiceAddressChange,
        this, &m_wathed_service,
        cxx::placeholders::_1, cxx::placeholders::_2, cxx::placeholders::_3);
    m_service_manager->RegisterWatchFunc(func);

    return ErrorInfo::kRpcNoRrror;
}

int Rpc::InitHttpDriver() {
    if (m_http_driver) {
        return 0;
    }

    m_http_driver = new net::HttpDriver();
    if (!m_http_driver) {
        return -1;
    }

    int ret = m_http_driver->Init();
    if (ret != 0) {
        delete m_http_driver;
        m_http_driver = NULL;
        PLOG_ERROR("init http driver failed %d", ret);
        return ret;
    }

    ret = net::Message::RegisterDriver(m_http_driver);
    if (ret != 0) {
        PLOG_ERROR("register http driver failed %d", ret);
    }

    return ret;
}

int Rpc::InitTcpDriver() {
    if (m_tcp_driver) {
        return 0;
    }

    m_tcp_driver = new net::TcpDriver();
    if (!m_tcp_driver) {
        return -1;
    }

    int ret = m_tcp_driver->Init();
    if (ret != 0) {
        delete m_tcp_driver;
        m_tcp_driver = NULL;
        PLOG_ERROR("init tcp driver failed %d", ret);
        return ret;
    }

    ret = net::Message::RegisterDriver(m_tcp_driver);
    if (ret != 0) {
        PLOG_ERROR("register tcp driver failed %d", ret);
    }

    return ret;
}

int Rpc::OnMessage(int handle, const char* msg, size_t msg_len, uint64_t addr) {
    if (0 == msg_len) {
        return 0;
    }

    std::map<int, ConnectionData*>::iterator it = m_connections.find(handle);
    if (m_connections.end() == it) {
        PLOG_ERROR("unknown handle(%d)", handle);
        return -1;
    }

    // 设置当前会话连接信息
    int64_t session_id = -1;

    // 查找连接信息
    ConnectionData* conn = it->second;
    cxx::shared_ptr<protocol::TProtocol> tmp_proto;
    if (conn->m_server) {
        m_conn_mgr->OnMessage(handle, addr, session_id);
        std::map< uint64_t, cxx::shared_ptr<protocol::TProtocol> >::iterator itp;
        itp = (conn->m_protocols).find(addr);
        if ((conn->m_protocols).end() == itp) {
            // 新接入连接
            cxx::shared_ptr<transport::MsgBuffer> msgbuff(new transport::MsgBuffer());
            std::string url("");
            msgbuff->bind(handle, url, addr);

            protocol::ProtocolFactory prot_fact;
            cxx::shared_ptr<protocol::TProtocol> proto(
                prot_fact.getProtocol(conn->m_prot_type, msgbuff));
            if (!proto) {
                PLOG_ERROR("get protocol failed(%d)", conn->m_prot_type);
                return -2;
            }

            msgbuff->open();
            (conn->m_protocols)[addr] = proto;
            tmp_proto = proto;
        } else {
            tmp_proto = itp->second;
        }
    } else {
        tmp_proto = (conn->m_protocols).find(0)->second;
    }

    if (!tmp_proto) {
        PLOG_ERROR("can't find protoco(%d)", handle);
        return -3;
    }

    // 取出消息
    transport::MsgBuffer* buf =
        dynamic_cast<transport::MsgBuffer*>(tmp_proto->getTransport().get()); // NOLINT
    if (!buf) {
        PLOG_ERROR("dynamic_cast failed");
        return -4;
    }

    m_cur_conn_obj.Set(handle, addr, conn->m_prot_type, session_id);

    buf->setMessage((const uint8_t*)(msg), msg_len);

    // 消息处理
    int ret = ProcessMessage(handle, tmp_proto);

    return ret;
}

int Rpc::OnUnknowMessage(int handle, const char* msg, size_t msg_len, uint64_t addr) {
    PLOG_DEBUG("recv unknown msg(%d,%lu)", handle, addr);
    return 0;
}

int Rpc::OnConnect(int handle, uint64_t addr) {
    // tbus需要rpc自己处理新连接接入，其他传输由NET上报connect事件
    std::map<int, ConnectionData*>::iterator it = m_connections.find(handle);
    if (m_connections.end() == it) {
        PLOG_ERROR("unknown handle(%d)", handle);
        return -1;
    }

    ConnectionData* conn = it->second;
    if (!conn->m_server) {
        PLOG_ERROR("onconnect at client(%d)", handle);
        return -2;
    }

    std::map< uint64_t, cxx::shared_ptr<protocol::TProtocol> >::iterator itp;
    itp = (conn->m_protocols).find(addr);
    if ((conn->m_protocols).end() != itp) {
        PLOG_ERROR("%d:%lu already connected", handle, addr);
        return -3;
    }

    // 新接入连接
    cxx::shared_ptr<transport::MsgBuffer> msgbuff(new transport::MsgBuffer());
    std::string url("");
    msgbuff->bind(handle, url, addr);

    protocol::ProtocolFactory prot_fact;
    cxx::shared_ptr<protocol::TProtocol> proto(prot_fact.getProtocol(conn->m_prot_type, msgbuff));
    if (!proto) {
        PLOG_ERROR("get protocol failed(%d)", conn->m_prot_type);
        return -4;
    }

    msgbuff->open();
    (conn->m_protocols)[addr] = proto;

    int64_t session_id = -1;
    m_conn_mgr->OnMessage(handle, addr, session_id);

    return 0;
}

int Rpc::OnClose(int handle, uint64_t addr, bool is_idle) {
    std::map<int, ConnectionData*>::iterator it = m_connections.find(handle);
    if (m_connections.end() == it) {
        PLOG_ERROR("unknown handle(%d)", handle);
        return -1;
    }

    ConnectionData* conndata = it->second;
    if (conndata->m_server) {
        if (!is_idle) {
            m_conn_mgr->OnClose(handle, addr);
        }

        std::map< uint64_t, cxx::shared_ptr<protocol::TProtocol> >::iterator mit;
        mit = conndata->m_protocols.find(addr);
        if (conndata->m_protocols.end() == mit) {
            PLOG_ERROR("unknown addr(%lu)", addr);
            return -2;
        }
        // server: 连接close需要释放其上承载的protocol，以及远端地址信息
        conndata->m_protocols.erase(mit);
        return 0;
    }

    // client: 不关心addr，只关心handle
    if (2 == conndata->m_protocols.size()) {
        cxx::shared_ptr<protocol::GroupProtocol> group =
            cxx::dynamic_pointer_cast<protocol::GroupProtocol>(conndata->m_protocols[1]);
        cxx::shared_ptr<protocol::TProtocol> prot = conndata->m_protocols[0];
        group->rmvProtocol(prot);
    }
    delete it->second;
    m_connections.erase(it);

    return 0;
}

int Rpc::OnIdle(int handle, uint64_t addr, int64_t session_id) {
    PLOG_INFO("idle(%d,%lu,%ld)", handle, addr, session_id);
    if (m_conn_event_cbs.event_idle) {
        m_conn_event_cbs.event_idle(handle, addr, session_id);
    }
    OnClose(handle, addr, true);
    return 0;
}

void Rpc::OnServiceAddressChange(std::map<std::string,
        std::vector<cxx::shared_ptr<protocol::TProtocol> > > *service_name_to_protocols,
        const std::string& service_name,
        int service_route,
        const std::vector<service::ServiceAddressInfo>& address) {
    std::map<std::string, std::vector< cxx::shared_ptr<protocol::TProtocol> > >::iterator iter =
        service_name_to_protocols->find(service_name);
    if (iter != service_name_to_protocols->end()) {
        for (uint32_t i = 0; i < iter->second.size(); i++) {
            std::tr1::function<void(int)> cob; // NOLINT
            int ret = m_service_manager->WatchServiceAddressAsync(service_name, cob);
            PLOG_IF_ERROR(ret != 0, "watch %s failed(%d)", service_name.c_str(), ret);
            UpdateServiceAddr(iter->second[i],
                    const_cast<std::vector<service::ServiceAddressInfo>& >(address));
        }
    }
}

void Rpc::UpdateServiceAddr(cxx::shared_ptr<protocol::TProtocol> prot,
                                std::vector<service::ServiceAddressInfo>& new_addrs) {
    // 客户端protocol 统一为group protocol
    protocol::GroupProtocol* group_prot = dynamic_cast<protocol::GroupProtocol*>(prot.get()); // NOLINT
    if (!group_prot) {
        PLOG_ERROR("dynamic_cast protocol error");
        return;
    }

    std::vector< cxx::shared_ptr<protocol::TProtocol> > prots = group_prot->getProtocols();
    std::vector< cxx::shared_ptr<protocol::TProtocol> >::iterator it;
    transport::MsgBuffer* msgbuf = NULL;
    std::vector<service::ServiceAddressInfo> add_addrs = new_addrs;
    for (it = prots.begin(); it != prots.end(); ++it) {
        msgbuf = dynamic_cast<transport::MsgBuffer*>(((*it)->getTransport()).get()); // NOLINT
        if (!msgbuf) {
            PLOG_ERROR("dynamic_cast msgbuff error");
            continue;
        }

        std::string url = msgbuf->getPeerUrl();
        bool find = false;

        std::vector<service::ServiceAddressInfo>::iterator sit;
        for (sit = add_addrs.begin(); sit != add_addrs.end(); ++sit) {
            if (url == (*sit).address_url) {
                find = true;
                add_addrs.erase(sit); // 这个地址没有变化
                break;
            }
        }

        if (!find) {
            // 这个地址被删除了，清除相应资源
            DeleteClientConnection(msgbuf->getHandle());

            group_prot->rmvProtocol(*it);
        }
    }

    for (std::vector<service::ServiceAddressInfo>::iterator ait = add_addrs.begin();
        ait != add_addrs.end(); ++ait) {
        cxx::shared_ptr<protocol::GroupProtocol> tmp_prot =
            cxx::dynamic_pointer_cast<protocol::GroupProtocol>(prot);
        if (CreateClientConnection((*ait).address_url,
            static_cast<PROTOCOL_TYPE>((*ait).protocol_type), tmp_prot) != 0) {
            continue;
        }
    }
}

int Rpc::ProcessMessage(int handle, cxx::shared_ptr<protocol::TProtocol> prot) {
    std::string name;
    protocol::TMessageType type;
    int64_t seqid;

    TApplicationException::TApplicationExceptionType ex_type = TApplicationException::UNKNOWN;
    std::string ex_msg;

    try {
        prot->readMessageBegin(name, type, seqid);

        PLOG_DEBUG("message info(%s,%d,%lld)", name.c_str(), type, (long long int)seqid);

        switch (type) {
        case protocol::T_CALL:
        case protocol::T_ONEWAY:
            if (!AccessCheck(handle, name.substr(0, name.find(":")))) {
                throw TApplicationException(TApplicationException::ACCESS_DENIED,
                    name + " access denied.");
            }

            ProcessRequest(name, seqid, prot);
            return 1;

        case protocol::T_REPLY:
        case protocol::T_EXCEPTION:
            ProcessResponse(type, seqid, prot.get());
            return 1;

        default:
            // 解码失败，名称可能超长，截断处理
            if (name.size() > 128) {
                name.resize(128);
            }
            break;
        }
    } catch (transport::TTransportException te) {
        ex_type = TApplicationException::TRANSPORT_EXCEPTION;
        ex_msg = te.what();
    } catch (protocol::TProtocolException pe) {
        ex_type = TApplicationException::PROTOCOL_EXCEPTION;
        ex_msg = pe.what();
    } catch (TApplicationException ae) {
        ex_type = ae.getType();
        ex_msg = ae.what();
    } catch (TException e) {
        ex_msg = e.what();
    }

    PLOG_ERROR("exception(%d,%s)", ex_type, ex_msg.c_str());

    try {
        prot->getTransport()->readEnd();

        if (protocol::T_CALL == type) {
            prot->writeMessageBegin(name, protocol::T_EXCEPTION, seqid);
            TApplicationException ex(ex_type, ex_msg);
            ex.write(prot.get());
            prot->writeMessageEnd();
            prot->getTransport()->writeEnd();
            prot->getTransport()->flush();
        }
    } catch (TException e) {
        ex_msg = e.what();
        PLOG_ERROR("exception(%s)", ex_msg.c_str());
    }

    return -4;
}

int Rpc::ProcessResponse(protocol::TMessageType mtype, int64_t seqid,
                            protocol::TProtocol* protocol, bool is_sync) {
    // 如果是同步，收到配对的响应后返回，递交给stub处理
    if (m_syn_proc && m_syn_seq == static_cast<uint64_t>(seqid)) {
        m_syn_proc = false;
        m_syn_prot = protocol;
        if (mtype != protocol::T_REPLY) {
            m_syn_result = ErrorInfo::kRecvException;
        }
        return 0;
    }

    // 处理响应消息中产生的异常自己要吃掉
    try {
        if (m_sessions.find(seqid) == m_sessions.end()) {
            protocol->getTransport()->readEnd();

            PLOG_ERROR("session is expired(%lld)", (long long int)seqid);
            return 1;
        }

        if (mtype == protocol::T_EXCEPTION) {
            TApplicationException x;
            x.read(protocol);
            protocol->readMessageEnd();
            protocol->getTransport()->readEnd();

            m_sessions[seqid].recv_cob(ErrorInfo::kRecvException, protocol);
            m_sessions.erase(seqid);

            PLOG_ERROR("recv exception(%s)", x.what());
            return 1;
        }

        // 接收处理响应消息
        m_sessions[seqid].recv_cob(ErrorInfo::kRpcNoRrror, protocol);
        m_sessions.erase(seqid);
    } catch (TException ex) {
        m_sessions.erase(seqid);

        PLOG_ERROR("ProcessResponse failed. %s", ex.what());
    }

    return 1;
}

void Rpc::ProcessTimeout() {
    int64_t now = TimeUtility::GetCurremtMs();

    std::map<uint64_t, CobSession>::iterator it;
    std::map<uint64_t, CobSession>::iterator erase_begin = m_sessions.begin();
    std::map<uint64_t, CobSession>::iterator erase_end = m_sessions.end();

    for (it = m_sessions.begin(); it != m_sessions.end(); ++it) {
        if (it->second.m_time > now) {
            // map按key排序，消息的seqid越小则消息越老，老的消息未超时，后面就不用再看
            break;
        }

        PLOG_ERROR("request timeout, seqid = %lld", (long long int)(it->first));

        m_sessions[it->first].recv_cob(ErrorInfo::kRpcTimeoutError, NULL);
        erase_end = it;
    }

    if (erase_end != m_sessions.end()) {
        m_sessions.erase(erase_begin, ++erase_end);
    }
}

void Rpc::RequestComplete(bool ok) {
    // stat?
}

void Rpc::FreeProtocol() {
    std::vector< cxx::shared_ptr<protocol::TProtocol> >::iterator it;
    for (it = m_free_protocols.begin(); it != m_free_protocols.end(); ++it) {
        protocol::GroupProtocol* group_prot = dynamic_cast<protocol::GroupProtocol*>((*it).get()); // NOLINT
        if (!group_prot) {
            PLOG_ERROR("dynamic_cast protocol error");
            continue;
        }

        std::vector< cxx::shared_ptr<protocol::TProtocol> > prots = group_prot->getProtocols();
        std::vector< cxx::shared_ptr<protocol::TProtocol> >::iterator pit;
        transport::MsgBuffer* msgbuf = NULL;
        for (pit = prots.begin(); pit != prots.end(); ++pit) {
            msgbuf = dynamic_cast<transport::MsgBuffer*>(((*pit)->getTransport()).get()); // NOLINT
            if (!msgbuf) {
                PLOG_ERROR("dynamic_cast msgbuff error");
                continue;
            }

            DeleteClientConnection(msgbuf->getHandle());
            group_prot->rmvProtocol(*pit);
        }

        // 清理监听服务地址列表中记录的protocol
        if (!RmvProtocol4WatchList(&m_wathed_service, *it)) {
            // 不是本区
            for (std::map<std::string, ServiceManagerInfo*>::iterator iter =
                    m_unit_to_service_manager.begin();
                    iter != m_unit_to_service_manager.end(); ++iter) {
                if (RmvProtocol4WatchList(&(iter->second->service_name_to_protocols), *it)) {
                    if (iter->second->service_name_to_protocols.empty()) {
                        iter->second->service_manager->Fini();
                        delete iter->second->service_manager;
                        m_unit_to_service_manager.erase(iter);
                    }
                    break;
                }
            }
        }
    }
    m_free_protocols.clear();
}

int Rpc::GetServiceAddress(const std::string& zk_url,
                                const std::string& service_name,
                                std::vector<std::string>* addresses,
                                std::vector<int>* types,
                                int* route_type) {
    int ret = 0;
    if (!m_service_manager) {
        ret = InitServiceManager(zk_url);
        if (ret != 0) {
            return ret;
        }
    }
    if (!m_service_manager) {
        return -1;
    }

    std::vector<service::ServiceAddressInfo> addr_info;
    ret = m_service_manager->GetServiceAddress(service_name, route_type, &addr_info);
    m_service_manager->WatchServiceAddress(service_name);
    if (service::kSM_OK == ret) {
        std::vector<service::ServiceAddressInfo>::iterator it;
        for (it = addr_info.begin(); it != addr_info.end(); ++it) {
            addresses->push_back((*it).address_url);
            types->push_back((*it).protocol_type);
        }
    }

    if (*route_type <= pebble::service::kUnknownRoute ||
        *route_type >= pebble::service::kMAXRouteVal) {
        *route_type = pebble::service::kRoundRoute;
    }

    return ret;
}

void Rpc::AddService2WatchList(std::map<std::string,
        std::vector<cxx::shared_ptr<protocol::TProtocol> > > *service_name_to_protocols,
        const std::string& service_name,
        cxx::shared_ptr<protocol::TProtocol> prot) {
    std::map<std::string, std::vector< cxx::shared_ptr<protocol::TProtocol> > >::iterator it;
    it = service_name_to_protocols->find(service_name);
    if (it != service_name_to_protocols->end()) {
        (it->second).push_back(prot);
        return;
    }

    std::vector< cxx::shared_ptr<protocol::TProtocol> > vect;
    vect.push_back(prot);
    (*service_name_to_protocols)[service_name] = vect;
}

bool Rpc::RmvProtocol4WatchList(std::map<std::string,
        std::vector<cxx::shared_ptr<protocol::TProtocol> > > *service_name_to_protocols,
        cxx::shared_ptr<protocol::TProtocol> prot) {
    std::map<std::string, std::vector< cxx::shared_ptr<protocol::TProtocol> > >::iterator it;
    std::vector< cxx::shared_ptr<protocol::TProtocol> >::iterator vit;
    for (it = service_name_to_protocols->begin(); it != service_name_to_protocols->end(); ++it) {
        for (vit = (it->second).begin(); vit != (it->second).end(); ++vit) {
            if (*vit == prot) {
                (it->second).erase(vit);
                if ((it->second).empty()) {
                    service_name_to_protocols->erase(it);
                }
                return true;
            }
        }
    }
    return false;
}

bool Rpc::AccessCheck(int handle, const std::string& service_name) {
    std::map<int, ListenData*>::iterator it = m_listen_data.find(handle);
    if (m_listen_data.end() == it) {
        // 客户端从自己通道上收到的反向rpc消息
        return true;
    }

    ListenData* listen = it->second;
    if (listen && !listen->AllowAccess(service_name)) {
        return false;
    }

    return true;
}

int Rpc::CreateClientConnection(std::string url,
                            PROTOCOL_TYPE protocol_type,
                            cxx::shared_ptr<protocol::GroupProtocol>& group_prot) {
    // 创建连接
    int handle = net::Message::NewHandle();
    int ret = net::Message::Connect(handle, url);
    if (ret != 0) {
        net::Message::Close(handle);
        PLOG_ERROR("connect failed(%d(%s),%s)", ret, net::Message::LastErrorMessage(), url.c_str());
        return -1;
    }

    // 创建msgbuff
    cxx::shared_ptr<transport::MsgBuffer> msgbuff(new transport::MsgBuffer());
    msgbuff->bind(handle, url);
    msgbuff->open();

    // 创建protocol
    protocol::ProtocolFactory prot_fact;
    cxx::shared_ptr<protocol::TProtocol> prot = prot_fact.getProtocol(protocol_type, msgbuff);

    // 创建ConnectionData
    ConnectionData* conn_data = new ConnectionData();
    conn_data->m_server = false;
    conn_data->m_prot_type = protocol_type;
    conn_data->m_protocols[0] = prot;

    // 添加handle和ConnectionData的映射关系
    m_connections[handle] = conn_data;

    if (!group_prot) {
        group_prot.reset(new protocol::GroupProtocol());
    }
    group_prot->addProtocol(prot);

    conn_data->m_protocols[1] = group_prot;

    return 0;
}

int Rpc::DeleteClientConnection(int handle) {
    std::map<int, ConnectionData*>::iterator mit = m_connections.find(handle);
    if (mit != m_connections.end()) {
        delete mit->second;
        m_connections.erase(mit);
    } else {
        PLOG_ERROR("can't find %d in conns", handle);
    }

    net::Message::Close(handle);

    return 0;
}

/// @brief 添加监听地址，一个地址只能承载一种编码协议，支持白名单、黑名单\n
/// 白名单、黑名单的作用是过滤或放通从某连接收到的请求，处理流程如下:\n
///   0. 从接入此监听地址的连接收到请求\n
///   1. 检查访问的服务名是否在黑名单中，如果在则拒绝访问，否则进入第2步\n
///   2. 检查白名单，若白名单为空，允许访问，若白名单非空，进入第3步\n
///   3. 检查访问的服务名是否在白名单中，如果在白名单中允许访问，否则拒绝访问\n
///   4. 对于拒绝访问的请求Rpc会返回异常响应\n
///   RPC会按照上述规则注册地址到服务管理系统中
/// @param listen_url server监听地址，支持tcp/http/tbus/pipe传输协议，如:\n
///   "tcp://127.0.0.1:6666"\n
///   "http://127.0.0.1:7777"\n
///   "tbus://1.1.1:8888"\n
///   "pipe://1.1.1:9999"
/// @param protocol_type 此通道使用的编码协议，支持binary/json/bson
/// @param white_list 从此监听地址接入的连接收到的请求能够访问的服务列表，默认NULL(白名单为空)
/// @param black_list 从此监听地址接入的连接收到的请求不能访问的服务列表，默认NULL(黑名单为空)
/// @param register_2_zk 此监听地址是否要注册到zk，默认注册，(调试地址可选择不注册)
/// @return 0 成功
/// @return 非0 失败
int Rpc::AddServiceManner(const std::string& listen_url,
                                PROTOCOL_TYPE protocol_type,
                                const std::vector<std::string>* white_list,
                                const std::vector<std::string>* black_list,
                                bool register_2_zk) {
    // 冲突检查，每个url只允许承载一种协议
    for (std::map<int, ListenData*>::iterator it = m_listen_data.begin();
        it != m_listen_data.end(); ++it) {
        if (listen_url != (it->second)->m_listen_url) {
            continue;
        }

        std::map<int, ConnectionData*>::iterator mit = m_connections.find(it->first);
        if (m_connections.end() == mit) {
            PLOG_FATAL("data unidentical(%d)", it->first);
            continue;
        }

        if (mit->second->m_prot_type != protocol_type) {
            PLOG_ERROR("%s already listened(%d != %d)", listen_url.c_str(),
                mit->second->m_prot_type, protocol_type);
            return -1;
        }

        return 0;
    }

    // bind listen url
    int handle = net::Message::NewHandle();
    int ret = net::Message::Bind(handle, listen_url);
    if (ret != 0) {
        net::Message::Close(handle);
        PLOG_ERROR("Net bind failed(%d, %s) ret=%d", handle, listen_url.c_str(), ret);
        return -2;
    }

    ConnectionData* conn = new ConnectionData();
    conn->m_server = true;
    conn->m_prot_type = protocol_type;

    m_connections[handle] = conn;

    // 成功监听的连接才记录并注册到服务管理上
    ListenData* listen = new ListenData();
    listen->m_listen_url = listen_url;
    listen->m_prot_type = protocol_type;
    if (white_list) { listen->m_white_list = *white_list; }
    if (black_list) { listen->m_black_list = *black_list; }
    listen->m_register_2_zk = register_2_zk;
    m_listen_data[handle] = listen;

    m_url_to_handle[listen_url] = handle;
    m_handle_to_url[handle] = listen_url;

    return 0;
}

int Rpc::AddServiceManner(const std::string& addr_in_docker,
                                const std::string& addr_in_host,
                                PROTOCOL_TYPE protocol_type,
                                const std::vector<std::string>* white_list,
                                const std::vector<std::string>* black_list) {
    // 当server部署在docker环境中时，对tcp/http传输需要使用此接口，tbus/pipe无需使用
    if ((!StringUtility::StartsWith(addr_in_docker, kTcpPrefix)
        && !StringUtility::StartsWith(addr_in_docker, kHttpPrefix))
        || (!StringUtility::StartsWith(addr_in_host, kTcpPrefix)
        && !StringUtility::StartsWith(addr_in_host, kHttpPrefix))) {
        PLOG_ERROR("addr must be tcp/http");
        return -1;
    }

    // 冲突检查，每个url只允许承载一种协议
    for (std::map<int, ListenData*>::iterator it = m_listen_data.begin();
        it != m_listen_data.end(); ++it) {
        if (addr_in_host != (it->second)->m_listen_url) {
            continue;
        }

        std::map<int, ConnectionData*>::iterator mit = m_connections.find(it->first);
        if (m_connections.end() == mit) {
            PLOG_FATAL("data unidentical(%d)", it->first);
            continue;
        }

        if (mit->second->m_prot_type != protocol_type) {
            PLOG_ERROR("%s already listened(%d != %d)", addr_in_host.c_str(),
                mit->second->m_prot_type, protocol_type);
            return -1;
        }

        return 0;
    }

    // bind listen url
    int handle = net::Message::NewHandle();
    int ret = net::Message::Bind(handle, addr_in_docker);
    if (ret != 0) {
        net::Message::Close(handle);
        PLOG_ERROR("Net bind failed(%d, %s) ret=%d", handle, addr_in_docker.c_str(), ret);
        return -2;
    }

    ConnectionData* conn = new ConnectionData();
    conn->m_server = true;
    conn->m_prot_type = protocol_type;

    m_connections[handle] = conn;

    // 成功监听的连接才记录并注册到服务管理上
    ListenData* listen = new ListenData();
    listen->m_listen_url = addr_in_host;
    listen->m_prot_type = protocol_type;
    if (white_list) { listen->m_white_list = *white_list; }
    if (black_list) { listen->m_black_list = *black_list; }
    listen->m_register_2_zk = true;
    m_listen_data[handle] = listen;

    m_url_to_handle[addr_in_host] = handle;
    m_handle_to_url[handle] = addr_in_host;

    return 0;
}

int Rpc::GetHandleByListenUrl(const std::string &listen_url, int *handle) {
    std::map<std::string, int>::iterator iter = m_url_to_handle.find(listen_url);
    if (iter == m_url_to_handle.end()) {
        return -1;
    }

    *handle = iter->second;
    return 0;
}

int Rpc::GetListenUrlByHandle(int handle, std::string *listen_url) {
    std::map<int, std::string>::iterator iter = m_handle_to_url.find(handle);
    if (iter == m_handle_to_url.end()) {
        return -1;
    }

    *listen_url = iter->second;
    return 0;
}

int Rpc::RestoreProtocol(int handle, uint64_t peer_addr, int32_t *protocol_type) {
    std::map<int, ListenData*>::iterator iter = m_listen_data.find(handle);
    if (iter == m_listen_data.end()) {
        PLOG_ERROR("no listen data for(%d)", handle);
        return -1;
    }

    *protocol_type = iter->second->m_prot_type;

    std::map<int, ConnectionData*>::iterator it = m_connections.find(handle);
    if (m_connections.end() == it) {
        PLOG_ERROR("unknown handle(%d)", handle);
        return -2;
    }

    ConnectionData* conn = it->second;

    std::map< uint64_t, cxx::shared_ptr<protocol::TProtocol> >::iterator itp;
    itp = (conn->m_protocols).find(peer_addr);
    if ((conn->m_protocols).end() == itp) {
        cxx::shared_ptr<transport::MsgBuffer> msgbuff(new transport::MsgBuffer());
        std::string url("");
        msgbuff->bind(handle, url, peer_addr);

        protocol::ProtocolFactory prot_fact;
        cxx::shared_ptr<protocol::TProtocol> proto(
                prot_fact.getProtocol(conn->m_prot_type, msgbuff));
        if (!proto) {
            PLOG_ERROR("get protocol failed(%d)", conn->m_prot_type);
            return -3;
        }

        msgbuff->open();
        (conn->m_protocols)[peer_addr] = proto;
    }

    return 0;
}

cxx::shared_ptr<protocol::TProtocol> Rpc::GetBroadCastProtocol(
        const std::string& service_url,
        const std::string& service_name,
        PROTOCOL_TYPE protocol_type) {
    cxx::shared_ptr<protocol::GroupProtocol> group_prot;
    if (!pebble::broadcast::ChannelMgr::Instance()) {
        PLOG_ERROR("broadcast_mgr is null");
        return group_prot;
    }

    // 获取channel name
    std::string channel_name = service_url.substr(strlen(kBroadcastPrefix));
    if (!pebble::broadcast::ChannelMgr::Instance()->ChannelExist(channel_name)) {
        PLOG_ERROR("channel(%s) not exist", channel_name.c_str());
        return group_prot;
    }

    // 创建msgbuff
    cxx::shared_ptr<transport::MsgBuffer> msgbuff(new transport::MsgBuffer());
    msgbuff->bind(pebble::broadcast::ChannelMgr::Instance(), channel_name, protocol_type);
    msgbuff->open();

    // 创建protocol
    protocol::ProtocolFactory prot_fact;
    cxx::shared_ptr<protocol::TProtocol> prot = prot_fact.getProtocol(protocol_type, msgbuff);
    if (prot) {
        group_prot.reset(new protocol::GroupProtocol());
        group_prot->addProtocol(prot);
    }

    return group_prot;
}

int Rpc::SetConnectionIdleTime(int idle_timeout_s)
{
    if (m_conn_mgr) {
        m_conn_mgr->SetIdleTimeout(idle_timeout_s);
    }
    return -1;
}


} // namespace rpc
}  // namespace pebble

