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

#include <sstream>
#include <string.h>

#include "common/log.h"
#include "common/timer.h"
#include "common/time_utility.h"
#include "framework/rpc.h"

namespace pebble {

/// @brief RPC会话数据结构定义
struct RpcSession {
private:
    RpcSession(const RpcSession& rhs) {
        m_session_id  = rhs.m_session_id;
        m_handle      = rhs.m_handle;
        m_timerid     = rhs.m_timerid;
        m_start_time  = rhs.m_start_time;
        m_server_side = rhs.m_server_side;
    }
public:
    RpcSession() {
        m_session_id  = 0;
        m_handle      = 0;
        m_timerid     = -1;
        m_start_time  = 0;
        m_server_side = false;
    }

    uint64_t m_session_id;
    int64_t  m_handle;
    int64_t  m_timerid;
    int64_t  m_start_time;
    RpcHead  m_rpc_head;
    bool     m_server_side;
    OnRpcResponse m_rsp;
};

// TODO: timer改为外部传入
IRpc::IRpc() {
    m_session_id        = 0;
    m_timer             = new SequenceTimer();
    m_task_num          = 0;
    m_latest_handle     = -1;
}

IRpc::~IRpc() {
    if (m_timer) {
        delete m_timer;
        m_timer = NULL;
    }
}

int32_t IRpc::Update() {
    int32_t num = 0;
    if (m_timer) {
        num += m_timer->Update();
    }

    return num;
}

int32_t IRpc::OnMessage(int64_t handle, const uint8_t* msg,
    uint32_t msg_len, const MsgExternInfo* msg_info, uint32_t is_overload) {

    if (NULL == msg || 0 == msg_len) {
        PLOG_ERROR("param invalid: buff = %p, buff_len = %u", msg, msg_len);
        return kRPC_INVALID_PARAM;
    }

    RpcHead head;

    int32_t head_len = HeadDecode(msg, msg_len, &head);
    if (head_len < 0) {
        PLOG_ERROR("HeadDecode failed(%d).", head_len);
        return kRPC_DECODE_FAILED;
    }

    if (head_len > static_cast<int32_t>(msg_len)) {
        PLOG_ERROR("head_len(%d) > buff_len(%u)", head_len, msg_len);
        return kRPC_DECODE_FAILED;
    }

    if (msg_info) {
        head.m_dst = (IProcessor*)(msg_info->_src);
    }
    const uint8_t* data = msg + head_len;
    uint32_t data_len   = msg_len - head_len;

    int32_t ret = kRPC_UNKNOWN_TYPE;
    switch (head.m_message_type) {
        case kRPC_CALL:
            if (is_overload != 0) {
                ret = ResponseException(handle, kRPC_SYSTEM_OVERLOAD_BASE - is_overload, head);
                RequestProcComplete(head.m_function_name, kRPC_SYSTEM_OVERLOAD_BASE - is_overload, 0);
                break;
            }
        case kRPC_ONEWAY:
            m_latest_handle = handle;
            ret = ProcessRequest(handle, head, data, data_len);
            break;

        case kRPC_REPLY:
        case kRPC_EXCEPTION:
            ret = ProcessResponse(head, data, data_len);
            break;

        default:
            PLOG_ERROR("rpc msg type error(%d)", head.m_message_type);
            break;
    }

    return ret;
}

int32_t IRpc::AddOnRequestFunction(const std::string& name, const OnRpcRequest& on_request) {
    if (name.empty() || !on_request) {
        PLOG_ERROR("param invalid: name = %s, !on_request = %u", name.c_str(), !on_request);
        return kRPC_INVALID_PARAM;
    }

    if (false == m_service_map.insert({name, on_request}).second) {
        PLOG_ERROR("the %s is existed", name.c_str());
        return kRPC_FUNCTION_NAME_EXISTED;
    }

    return kRPC_SUCCESS;
}

int32_t IRpc::RemoveOnRequestFunction(const std::string& name) {
    return m_service_map.erase(name) == 1 ? kRPC_SUCCESS : kRPC_FUNCTION_NAME_UNEXISTED;
}

void IRpc::GetResourceUsed(cxx::unordered_map<std::string, int64_t>* resource_info) {
    if (!resource_info) {
        return;
    }
    std::ostringstream timer;
    timer << "Rpc(" << this << "):timer";
    (*resource_info)[timer.str()]   = m_timer->GetTimerNum();

    std::ostringstream session;
    session << "Rpc(" << this << "):session";
    (*resource_info)[session.str()] = m_session_map.size();
    return;
}

int32_t IRpc::SendRequest(int64_t handle,
                    const RpcHead& rpc_head,
                    const uint8_t* buff,
                    uint32_t buff_len,
                    const OnRpcResponse& on_rsp,
                    int32_t timeout_ms) {
    // buff允许为空，长度非0时做非空检查
    if (buff_len != 0 && NULL == buff) {
        PLOG_ERROR("param invalid: buff = %p, buff_len = %u", buff, buff_len);
        return kRPC_INVALID_PARAM;
    }

    // 发送请求
    int32_t ret = SendMessage(handle, rpc_head, buff, buff_len);
    if (ret != kRPC_SUCCESS) {
        ResponseProcComplete(rpc_head.m_function_name, kRPC_SEND_FAILED, 0);
        return ret;
    }

    // ONEWAY请求
    if (!on_rsp) {
        ResponseProcComplete(rpc_head.m_function_name, kRPC_SUCCESS, 0);
        return kRPC_SUCCESS;
    }

    // 保持会话
    cxx::shared_ptr<RpcSession> session(new RpcSession());
    session->m_session_id  = rpc_head.m_session_id;
    session->m_handle      = handle;
    session->m_rsp         = on_rsp;
    session->m_rpc_head    = rpc_head;
    session->m_server_side = false;
    TimeoutCallback cb     = cxx::bind(&IRpc::OnTimeout, this, session->m_session_id);

    if (timeout_ms <= 0) {
        timeout_ms = 10 * 1000;
    }
    session->m_timerid     = m_timer->StartTimer(timeout_ms, cb);
    session->m_start_time  = TimeUtility::GetCurrentMS();

    m_session_map[session->m_session_id] = session;

    return kRPC_SUCCESS;
}

int32_t IRpc::BroadcastRequest(const std::string& name,
                    const RpcHead& rpc_head,
                    const uint8_t* buff,
                    uint32_t buff_len) {
    // buff允许为空，长度非0时做非空检查
    if (buff_len != 0 && NULL == buff) {
        PLOG_ERROR("param invalid: buff = %p, buff_len = %u", buff, buff_len);
        return kRPC_INVALID_PARAM;
    }

    // 发送请求
    int32_t head_len = HeadEncode(rpc_head, m_rpc_head_buff, sizeof(m_rpc_head_buff));
    if (head_len < 0) {
        PLOG_ERROR("encode head failed(%d)", head_len);
        return kRPC_ENCODE_FAILED;
    }

    const uint8_t* msg_frag[] = { m_rpc_head_buff, buff     };
    uint32_t msg_frag_len[]   = { head_len       , buff_len };

    int32_t num = m_broadcastv(name, sizeof(msg_frag) / sizeof(*msg_frag), msg_frag, msg_frag_len);

    return num >= 0 ? kRPC_SUCCESS : kPRC_BROADCAST_FAILED;
}

int32_t IRpc::SendResponse(uint64_t session_id, int32_t ret,
    const uint8_t* buff, uint32_t buff_len) {

    cxx::unordered_map< uint64_t, cxx::shared_ptr<RpcSession> >::iterator it =
        m_session_map.find(session_id);
    if (m_session_map.end() == it) {
        PLOG_ERROR("session %lu not found", session_id);
        return kRPC_SESSION_NOT_FOUND;
    }

    m_timer->StopTimer(it->second->m_timerid);

    int32_t result = kRPC_SUCCESS;
    if (kRPC_SUCCESS == ret) {
        // 业务处理成功，构造响应消息返回
        it->second->m_rpc_head.m_message_type = kRPC_REPLY;
        result = SendMessage(it->second->m_handle, it->second->m_rpc_head, buff, buff_len);
        RequestProcComplete(it->second->m_rpc_head.m_function_name,
            result, TimeUtility::GetCurrentMS() - it->second->m_start_time);
    } else {
        // 业务处理失败，构造异常消息携带错误信息返回
        result = ResponseException(it->second->m_handle, ret, it->second->m_rpc_head, buff, buff_len);
        RequestProcComplete(it->second->m_rpc_head.m_function_name,
            ret, TimeUtility::GetCurrentMS() - it->second->m_start_time);
    }

    m_session_map.erase(it);
    m_task_num--;

    return result;
}

int32_t IRpc::SendMessage(int64_t handle, const RpcHead& rpc_head,
    const uint8_t* buff, uint32_t buff_len) {
    int32_t head_len = HeadEncode(rpc_head, m_rpc_head_buff, sizeof(m_rpc_head_buff));
    if (head_len < 0) {
        PLOG_ERROR("encode head failed(%d)", head_len);
        return kRPC_ENCODE_FAILED;
    }

    const uint8_t* msg_frag[] = { m_rpc_head_buff, buff     };
    uint32_t msg_frag_len[]   = { head_len       , buff_len };

    // 消息原路返回，如果有消息来源，则返回到来源点，如果无消息来源，走默认发送流程
    int32_t send_ret = 0;
    if (rpc_head.m_dst) {
        send_ret = rpc_head.m_dst->SendV(handle, sizeof(msg_frag)/sizeof(*msg_frag), msg_frag, msg_frag_len, 0);
    }

    send_ret = SendV(handle, sizeof(msg_frag)/sizeof(*msg_frag), msg_frag, msg_frag_len, 0);
    if (send_ret != 0) {
        PLOG_ERROR("send failed %d", send_ret);
    }

    return send_ret;
}

int32_t IRpc::OnTimeout(uint64_t session_id) {
    cxx::unordered_map< uint64_t, cxx::shared_ptr<RpcSession> >::iterator it =
        m_session_map.find(session_id);
    if (m_session_map.end() == it) {
        PLOG_ERROR("session %lu not found", session_id);
        return kTIMER_BE_REMOVED;
    }

    // request timeout
    if ((it->second)->m_rsp) {
        (it->second)->m_rsp(kRPC_REQUEST_TIMEOUT, NULL, 0);
        ReportTransportQuality(it->second->m_handle, kRPC_REQUEST_TIMEOUT, 0);
    }

    if (it->second->m_server_side) {
        m_task_num--;
        RequestProcComplete(it->second->m_rpc_head.m_function_name,
            kRPC_PROCESS_TIMEOUT, TimeUtility::GetCurrentMS() - it->second->m_start_time);
    } else {
        ResponseProcComplete(it->second->m_rpc_head.m_function_name,
            kRPC_REQUEST_TIMEOUT, TimeUtility::GetCurrentMS() - it->second->m_start_time);
    }

    m_session_map.erase(it);

    return kTIMER_BE_REMOVED;
}

int32_t IRpc::ProcessRequest(int64_t handle, const RpcHead& rpc_head,
    const uint8_t* buff, uint32_t buff_len) {
    return ProcessRequestImp(handle, rpc_head, buff, buff_len);
}

int32_t IRpc::ProcessRequestImp(int64_t handle, const RpcHead& rpc_head,
    const uint8_t* buff, uint32_t buff_len) {

    cxx::unordered_map<std::string, OnRpcRequest>::iterator it =
        m_service_map.find(rpc_head.m_function_name);
    if (m_service_map.end() == it) {
        PLOG_ERROR("%s's request proc func not found", rpc_head.m_function_name.c_str());
        ResponseException(handle, kRPC_UNSUPPORT_FUNCTION_NAME, rpc_head);
        RequestProcComplete(rpc_head.m_function_name, kRPC_UNSUPPORT_FUNCTION_NAME, 0);
        return kRPC_UNSUPPORT_FUNCTION_NAME;
    }

    if (kRPC_ONEWAY == rpc_head.m_message_type) {
        cxx::function<int32_t(int32_t, const uint8_t*, uint32_t)> rsp; // NOLINT
        int32_t ret = (it->second)(buff, buff_len, rsp);
        RequestProcComplete(rpc_head.m_function_name, ret, 0);
        return ret;
    }

    // 请求处理也保持会话，方便扩展
    cxx::shared_ptr<RpcSession> session(new RpcSession());
    session->m_session_id  = GenSessionId();
    session->m_handle      = handle;
    session->m_rpc_head    = rpc_head;
    session->m_server_side = true;

    TimeoutCallback cb     = cxx::bind(&IRpc::OnTimeout, this, session->m_session_id);
    session->m_timerid     = m_timer->StartTimer(REQ_PROC_TIMEOUT_MS, cb);
    session->m_start_time  = TimeUtility::GetCurrentMS();

    m_session_map[session->m_session_id] = session;
    m_task_num++;

    cxx::function<int32_t(int32_t, const uint8_t*, uint32_t)> rsp = cxx::bind( // NOLINT
        &IRpc::SendResponse, this, session->m_session_id,
        cxx::placeholders::_1, cxx::placeholders::_2, cxx::placeholders::_3);

    return (it->second)(buff, buff_len, rsp);
}

int32_t IRpc::ProcessResponse(const RpcHead& rpc_head,
    const uint8_t* buff, uint32_t buff_len) {

    cxx::unordered_map< uint64_t, cxx::shared_ptr<RpcSession> >::iterator it =
        m_session_map.find(rpc_head.m_session_id);
    if (m_session_map.end() == it) {
        PLOG_ERROR("session(%lu) not found, function_name(%s)",
                        rpc_head.m_session_id, rpc_head.m_function_name.c_str());
        return kRPC_SESSION_NOT_FOUND;
    }

    m_timer->StopTimer(it->second->m_timerid);

    int ret = kRPC_SUCCESS;
    const uint8_t* real_buff = buff;
    uint32_t real_buff_len = buff_len;

    RpcException exception;
    if (kRPC_EXCEPTION == rpc_head.m_message_type) {
        int32_t len = ExceptionDecode(buff, buff_len, &exception);
        if (len < 0) {
            PLOG_ERROR("ExceptionDecode failed(%d)", len);
            ret = kRPC_RECV_EXCEPTION_MSG;
            real_buff = NULL;
            real_buff_len = 0;
        } else {
            ret = exception.m_error_code;
            real_buff = (const uint8_t*)exception.m_message.data();
            real_buff_len = exception.m_message.size();
        }
    }

    if (it->second->m_rsp) {
        ret = it->second->m_rsp(ret, real_buff, real_buff_len);
    }

    int64_t time_cost = TimeUtility::GetCurrentMS() - it->second->m_start_time;
    ReportTransportQuality(it->second->m_handle, ret, time_cost);
    ResponseProcComplete(it->second->m_rpc_head.m_function_name, ret, time_cost);

    m_session_map.erase(it);

    return ret;
}

int32_t IRpc::ResponseException(int64_t handle, int32_t ret, const RpcHead& rpc_head,
    const uint8_t* buff, uint32_t buff_len) {

    (const_cast<RpcHead&>(rpc_head)).m_message_type = kRPC_EXCEPTION;

    RpcException exception;
    exception.m_error_code = ret;
    if (buff_len > 0) {
        exception.m_message.assign((const char*)buff, buff_len);
    }

    int32_t result = ExceptionEncode(exception, m_rpc_exception_buff, sizeof(m_rpc_exception_buff));
    if (result < 0) {
        PLOG_ERROR("ExceptionEncode failed, ret = %d", result);
        return result;
    }

    return SendMessage(handle, rpc_head, m_rpc_exception_buff, result);
}

void IRpc::ReportTransportQuality(int64_t handle, int32_t ret_code,
        int64_t time_cost_ms) {
    if (m_event_handler) {
        m_event_handler->ReportTransportQuality(handle, ret_code, time_cost_ms);
    }
}

void IRpc::RequestProcComplete(const std::string& name,
    int32_t result, int32_t time_cost_ms) {
    if (m_event_handler) {
        m_event_handler->RequestProcComplete(name, result, time_cost_ms);
    }
}

void IRpc::ResponseProcComplete(const std::string& name,
    int32_t result, int32_t time_cost_ms) {
    if (m_event_handler) {
        m_event_handler->ResponseProcComplete(name, result, time_cost_ms);
    }
}

} // namespace pebble

