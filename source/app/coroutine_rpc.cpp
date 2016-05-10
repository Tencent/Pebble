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


#include <string>
#include "source/app/coroutine_rpc.h"
#include "source/app/pebble_server.h"
#include "source/app/servant_task.h"
#include "source/common/time_utility.h"
#include "source/rpc/common/rpc_error_info.h"
#include "source/rpc/protocol/protocol_factory.h"
#include "source/rpc/transport/msg_buffer.h"

namespace pebble {
namespace rpc {

CoroutineRpc::CoroutineRpc() {
    m_result = ErrorInfo::kRpcTimeoutError;
    m_pebble_server = NULL;
    m_prot = NULL;
}

CoroutineRpc::~CoroutineRpc() {
    std::map<int64_t, ConnectionObj*>::iterator it = m_conn_objs.begin();
    for (; it != m_conn_objs.end(); ++it) {
        delete it->second;
    }
}

void CoroutineRpc::Resume(int64_t coroutine_id) {
    m_pebble_server->GetCoroutineSchedule()->Resume(coroutine_id);
}

int CoroutineRpc::Block(protocol::TProtocol** prot, int timeout_ms) {
    CoroutineSchedule *coroutine_schedule =
            m_pebble_server->GetCoroutineSchedule();
    int64_t co_id = coroutine_schedule->CurrentTaskId();
    cxx::function<void()> cob = cxx::bind(&CoroutineRpc::Resume, this, co_id);

    cxx::function<void(int ret, protocol::TProtocol* prot)> recv_cob =
        cxx::bind(&CoroutineRpc::Cob, this, cxx::placeholders::_1, cxx::placeholders::_2, cob);

    AddSession(recv_cob, timeout_ms);

    m_result = ErrorInfo::kRpcTimeoutError;

    coroutine_schedule->Yield();

    *prot = m_prot;

    return m_result;
}

int CoroutineRpc::ProcessRequest(
        const std::string& name, int64_t seqid,
        cxx::shared_ptr<protocol::TProtocol> protocol) {
    // 只有请求消息才起协程处理
    pebble::rpc::ServantTask* servant_task = (m_pebble_server
            ->GetCoroutineSchedule())->NewTask<pebble::rpc::ServantTask>();
    if (!servant_task) {
        PLOG_ERROR("new ServantTask failed.");
        return 0;
    }

    cxx::function<void(void)> run = cxx::bind(&CoroutineRpc::ProcessRequestImp, this,
                                              name, seqid, protocol);

    servant_task->Init(run);
    servant_task->Start();

    return 1;
}

int CoroutineRpc::ProcessRequestImp(const std::string& name, int64_t seqid,
                            cxx::shared_ptr<protocol::TProtocol> protocol) {
    AddCurConnectionObj();

    int num = Rpc::ProcessRequestImp(name, seqid, protocol);

    FreeCurConnectionObj();

    return num;
}

int CoroutineRpc::GetCurConnectionObj(ConnectionObj* connection_obj) {
    if (NULL == connection_obj) {
        PLOG_ERROR("para is NULL");
        return -1;
    }

    int64_t id = m_pebble_server->GetCoroutineSchedule()->CurrentTaskId();
    if (-1 == id) {
        return Rpc::GetCurConnectionObj(connection_obj);
    }

    std::map<int64_t, ConnectionObj*>::iterator it = m_conn_objs.find(id);
    if (it != m_conn_objs.end()) {
        *connection_obj = *(it->second);
        return 0;
    }

    return -2;
}

void CoroutineRpc::AddCurConnectionObj() {
    ConnectionObj* conn_obj = new ConnectionObj();
    if (0 == Rpc::GetCurConnectionObj(conn_obj)) {
        int64_t id = m_pebble_server->GetCoroutineSchedule()->CurrentTaskId();
        m_conn_objs[id] = conn_obj;
        return;
    }

    delete conn_obj;
}

void CoroutineRpc::FreeCurConnectionObj() {
    int64_t id = m_pebble_server->GetCoroutineSchedule()->CurrentTaskId();
    std::map<int64_t, ConnectionObj*>::iterator it = m_conn_objs.find(id);
    if (it != m_conn_objs.end()) {
        delete it->second;
        m_conn_objs.erase(it);
    }
}

int CoroutineRpc::PendingMsg(int handle, uint64_t peer_addr, int encode_type,
        const std::string& message) {
    cxx::shared_ptr<PendingMessage> msg(
        new PendingMessage(handle, peer_addr, encode_type, message));
    if (!msg) {
        return -1;
    }

    m_pending_msgs.push_back(msg);
    return 0;
}

int CoroutineRpc::ProcessPendingMsg(int max_event) {
    if (m_pending_msgs.empty()) {
        return 0;
    }

    if (max_event < 0) {
        max_event = 1;
    }

    int events = max_event;
    if (events > static_cast<int>(m_pending_msgs.size())) {
        events = m_pending_msgs.size();
    }

    int num = 0;
    for (int i = 0; i < events; i++) {
        // 取出一个消息
        cxx::shared_ptr<PendingMessage> msg = m_pending_msgs.front();
        m_pending_msgs.erase(m_pending_msgs.begin());
        if (!msg) { continue; }

        // 获得处理消息的protocol
        cxx::shared_ptr<protocol::TProtocol> prot;
        std::map<int, cxx::shared_ptr<protocol::TProtocol> >::iterator it;
        it = m_pending_handler.find(msg->m_encode_type);
        if (it != m_pending_handler.end()) {
            prot = it->second;
        } else {
            cxx::shared_ptr<transport::MsgBuffer> msgbuff(new transport::MsgBuffer());
            protocol::ProtocolFactory prot_fact;
            prot = prot_fact.getProtocol(
                static_cast<rpc::PROTOCOL_TYPE>(msg->m_encode_type), msgbuff);
            if (prot) { m_pending_handler[msg->m_encode_type] = prot; }
        }

        if (!prot) { continue; }

        // 绑定实际连接对象
        transport::MsgBuffer* msgbuff = dynamic_cast<transport::MsgBuffer*>( // NOLINT
            prot->getTransport().get());
        if (!msgbuff) { continue; }

        msgbuff->bind(msg->m_handle, std::string(""), msg->m_peer_addr);
        msgbuff->open();
        msgbuff->setMessage((const uint8_t*)(msg->m_message.data()), msg->m_message.length());

        // 处理消息
        ProcessMessage(msg->m_handle, prot);

        num++;
    }

    return num;
}


}  // namespace rpc
}  // namespace pebble
