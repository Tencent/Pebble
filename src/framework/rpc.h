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


#ifndef _PEBBLE_COMMON_RPC_H_
#define _PEBBLE_COMMON_RPC_H_

#include "framework/processor.h"


namespace pebble {

/// @brief RPC错误码定义
typedef enum {
    kRPC_ERROR_BASE              = RPC_ERROR_CODE_BASE,
    kRPC_INVALID_PARAM           = kRPC_ERROR_BASE - 1,   // 参数错误
    kRPC_ENCODE_FAILED           = kRPC_ERROR_BASE - 2,   // 编码错误
    kRPC_DECODE_FAILED           = kRPC_ERROR_BASE - 3,   // 解码错误
    kRPC_RECV_EXCEPTION_MSG      = kRPC_ERROR_BASE - 4,   // 收到EXCEPTION消息
    kRPC_UNKNOWN_TYPE            = kRPC_ERROR_BASE - 5,   // 消息类型错误
    kRPC_UNSUPPORT_FUNCTION_NAME = kRPC_ERROR_BASE - 6,   // 不支持的函数名
    kRPC_SESSION_NOT_FOUND       = kRPC_ERROR_BASE - 7,   // session已过期
    kRPC_SEND_FAILED             = kRPC_ERROR_BASE - 8,   // 发送失败
    kRPC_REQUEST_TIMEOUT         = kRPC_ERROR_BASE - 9,   // 请求超时
    kRPC_FUNCTION_NAME_EXISTED   = kRPC_ERROR_BASE - 10,  // 服务名已存在
    kRPC_SYSTEM_ERROR            = kRPC_ERROR_BASE - 11,  // 系统错误
    kRPC_PROCESS_TIMEOUT         = kRPC_ERROR_BASE - 13,  // 服务处理超时
    kPRC_BROADCAST_FAILED        = kRPC_ERROR_BASE - 14,  // 广播失败
    kRPC_FUNCTION_NAME_UNEXISTED = kRPC_ERROR_BASE - 15,  // 服务名不存在
    kRPC_PEBBLE_RPC_ERROR_BASE   = kRPC_ERROR_BASE - 100, // PEBBE RPC错误码BASE
    kRPC_RPC_UTIL_ERROR_BASE     = kRPC_ERROR_BASE - 200, // RPC辅助工具错误码BASE
    kRPC_SYSTEM_OVERLOAD_BASE    = kRPC_ERROR_BASE - 300, // 系统过载BASE
    kRPC_MESSAGE_EXPIRED         = kRPC_SYSTEM_OVERLOAD_BASE - 1, // 系统过载-消息过期
    kRPC_TASK_OVERLOAD           = kRPC_SYSTEM_OVERLOAD_BASE - 2, // 系统过载-并发任务过载
    kRPC_SUCCESS                 = 0,
} RpcErrorCode;

class RpcErrorStringRegister {
public:
    static void RegisterErrorString() {
        SetErrorString(kRPC_INVALID_PARAM, "invalid paramater");
        SetErrorString(kRPC_ENCODE_FAILED, "encode failed");
        SetErrorString(kRPC_DECODE_FAILED, "decode failed");
        SetErrorString(kRPC_RECV_EXCEPTION_MSG, "receive a exception message");
        SetErrorString(kRPC_UNKNOWN_TYPE, "unknown message type received");
        SetErrorString(kRPC_UNSUPPORT_FUNCTION_NAME, "unsupport function name");
        SetErrorString(kRPC_SESSION_NOT_FOUND, "session is expired");
        SetErrorString(kRPC_SEND_FAILED, "send failed");
        SetErrorString(kRPC_REQUEST_TIMEOUT, "request timeout");
        SetErrorString(kRPC_FUNCTION_NAME_EXISTED, "service name is already registered");
        SetErrorString(kRPC_SYSTEM_ERROR, "system error");
        SetErrorString(kRPC_PROCESS_TIMEOUT, "process service timeout");
        SetErrorString(kPRC_BROADCAST_FAILED, "broadcast request failed");
        SetErrorString(kRPC_FUNCTION_NAME_UNEXISTED, "service name unexisted");
        SetErrorString(kRPC_MESSAGE_EXPIRED, "system overload: message expired");
        SetErrorString(kRPC_TASK_OVERLOAD, "system overload: task overload");
    }
};


/// @brief RPC消息类型定义
typedef enum {
    kRPC_CALL      = 1,
    kRPC_REPLY     = 2,
    kRPC_EXCEPTION = 3,
    kRPC_ONEWAY    = 4,
} RpcMessageType;


// 前置声明
class SequenceTimer;
struct RpcSession;

/// @brief RPC协议版本号
typedef enum {
    kVERSION_0 = 0,
} RpcVersion;

/// @brief RPC消息头定义
struct RpcHead {
    RpcHead() {
        m_version       = kVERSION_0;
        m_message_type  = kRPC_EXCEPTION;
        m_session_id    = 0;
        m_dst           = NULL;
    }
    RpcHead(const RpcHead& rhs) {
        m_version       = rhs.m_version;
        m_message_type  = rhs.m_message_type;
        m_session_id    = rhs.m_session_id;
        m_function_name = rhs.m_function_name;
        m_dst           = rhs.m_dst;
    }

    int32_t     m_version;
    int32_t     m_message_type;
    uint64_t    m_session_id;
    std::string m_function_name;

    IProcessor* m_dst; // 非消息相关，标示消息来源模块，响应原路返回
};

/// @brief RPC异常结构定义
struct RpcException {
    RpcException() {
        m_error_code = 0;
    }

    int32_t m_error_code;
    std::string m_message;
};

/// @brief RPC请求消息处理函数原型，由用户实现，RPC触发调用
/// @param buff 收到的消息码流
/// @param buff_len 消息长度
/// @param rsp 响应回调，请求处理完毕后调用此回调发送响应消息\n
///         rsp为空时，表示为ONEWAY请求，不需要回响应
/// @return 0 处理成功
/// @return 非0 处理失败
typedef cxx::function<int32_t(const uint8_t* buff, uint32_t buff_len, // NOLINT
            cxx::function<int32_t(int32_t ret, const uint8_t* buff, uint32_t buff_len)>& rsp)> OnRpcRequest; // NOLINT

/// @brief RPC响应消息处理函数原型，由用户实现，RPC负责回调
/// @param ret RPC调用结果，成功为0，其他非0 @see RpcErrorCode
/// @param buff 响应消息码流
/// @param buff_len 响应消息长度
typedef cxx::function<int32_t(int32_t ret, const uint8_t* buff, uint32_t buff_len)> OnRpcResponse;

class IRpc : public IProcessor {
public:
    IRpc();
    virtual ~IRpc();

    /// @brief 实现Processor接口，RPC管理事件驱动
    /// @return 处理的事件数，0表示无事件
    virtual int32_t Update();

    /// @brief 实现Processor接口，消息处理入口
    /// @return 0 成功
    /// @return 非0 失败 @see RpcErrorCode
    virtual int32_t OnMessage(int64_t handle, const uint8_t* msg,
        uint32_t msg_len, const MsgExternInfo* msg_info, uint32_t is_overload);

    /// @brief 实现Processor接口，返回并发的任务数
    virtual uint32_t GetUnFinishedTaskNum() {
        return m_task_num;
    }

    /// @brief 实现Processor接口，返回动态资源使用情况
    virtual void GetResourceUsed(cxx::unordered_map<std::string, int64_t>* resource_info);

    /// @brief 获取最近一次消息来源的handle，一般用于反向RPC
    virtual int64_t GetLatestMsgHandle() {
        return m_latest_handle;
    }

    /// @brief 添加RPC请求处理函数(RPC服务)
    /// @param name RPC请求服务的名字
    /// @param on_request 请求处理函数
    /// @return 0 成功
    /// @return 非0 失败 @see RpcErrorCode
    int32_t AddOnRequestFunction(const std::string& name, const OnRpcRequest& on_request);

    /// @brief 注销RPC请求处理函数(RPC服务)
    /// @param name RPC请求服务的名字
    /// @return 0 成功
    /// @return 非0 失败 @see RpcErrorCode
    int32_t RemoveOnRequestFunction(const std::string& name);

    /// @brief 发送RPC请求
    /// @param handle 网络句柄
    /// @param rpc_head RPC头部信息
    /// @param buff RPC数据部分
    /// @param buff_len RPC数据部分长度
    /// @param on_rsp 响应回调，为空时表示ONEWAY请求
    /// @param timeout_ms 等待响应超时时间，单位为ms，<=0时使用默认值(10s)
    /// @return 0 成功
    /// @return 非0 失败 @see RpcErrorCode
    int32_t SendRequest(int64_t handle,
                    const RpcHead& rpc_head,
                    const uint8_t* buff,
                    uint32_t buff_len,
                    const OnRpcResponse& on_rsp,
                    int32_t timeout_ms);

    /// @brief 广播RPC消息
    /// @param name 广播频道名
    /// @param rpc_head RPC头部信息
    /// @param buff RPC数据部分
    /// @param buff_len RPC数据部分长度
    /// @return 0 成功
    /// @return 非0 失败 @see RpcErrorCode
    int32_t BroadcastRequest(const std::string& name,
                    const RpcHead& rpc_head,
                    const uint8_t* buff,
                    uint32_t buff_len);

public:
    // TODO: 改成可配置
    static const int32_t REQ_PROC_TIMEOUT_MS = 60 * 1000; // 60s

    /// @note 内部使用，用户无需关注
    int32_t ProcessRequestImp(int64_t handle, const RpcHead& rpc_head,
        const uint8_t* buff, uint32_t buff_len);

    /// @note 内部使用，用户无需关注
    uint64_t GenSessionId() {
        return m_session_id++;
    }

protected:
    /// @brief RPC头的编码接口
    /// @param rpc_head RPC头部信息
    /// @param buff 编码存储buff
    /// @param buff_len buff长度
    /// @return >=0 编码后的RPC头长度
    /// @return <0 失败
    virtual int32_t HeadEncode(const RpcHead& rpc_head, uint8_t* buff, uint32_t buff_len) = 0;

    /// @brief RPC头的解码接口
    /// @param buff 待解码的消息码流
    /// @param buff_len 待解码的消息码流长度
    /// @param rpc_head 输出参数，RPC头部信息
    /// @return >=0 解码的RPC头长度
    /// @return <0 失败
    virtual int32_t HeadDecode(const uint8_t* buff, uint32_t buff_len, RpcHead* rpc_head) = 0;

    /// @brief RPC异常的编码接口
    /// @param rpc_exception RPC异常信息
    /// @param buff 编码存储buff
    /// @param buff_len buff长度
    /// @return >=0 编码后的RPC异常长度
    /// @return <0 失败
    virtual int32_t ExceptionEncode(const RpcException& rpc_exception,
        uint8_t* buff, uint32_t buff_len) = 0;

    /// @brief RPC异常的解码接口
    /// @param buff 待解码的消息码流
    /// @param buff_len 待解码的消息码流长度
    /// @param rpc_head 输出参数，RPC异常信息
    /// @return >=0 解码的RPC异常长度
    /// @return <0 失败
    virtual int32_t ExceptionDecode(const uint8_t* buff, uint32_t buff_len,
        RpcException* rpc_exception) = 0;

    virtual int32_t ProcessRequest(int64_t handle, const RpcHead& rpc_head,
                    const uint8_t* buff,
                    uint32_t buff_len);

private:
    // 发送RPC响应消息
    int32_t SendResponse(uint64_t session_id, int32_t ret, const uint8_t* buff, uint32_t buff_len);

    // 发送RPC消息，把RPC头和数据部分组装发送
    int32_t SendMessage(int64_t handle, const RpcHead& rpc_head, const uint8_t* buff, uint32_t buff_len);

    // 超时处理，暂时支持请求的超时，可扩展支持服务处理超时
    int32_t OnTimeout(uint64_t session_id);

private:
    int32_t ProcessResponse(const RpcHead& rpc_head,
                    const uint8_t* buff,
                    uint32_t buff_len);

    int32_t ResponseException(int64_t handle, int32_t ret, const RpcHead& rpc_head,
        const uint8_t* buff = NULL, uint32_t buff_len = 0);

    void ReportTransportQuality(int64_t handle, int32_t ret_code,
        int64_t time_cost_ms);

    void RequestProcComplete(const std::string& name,
        int32_t result, int32_t time_cost_ms);

    void ResponseProcComplete(const std::string& name,
        int32_t result, int32_t time_cost_ms);

private:
    cxx::unordered_map<std::string, OnRpcRequest> m_service_map;

    uint8_t m_rpc_head_buff[1024];
    uint8_t m_rpc_exception_buff[102400];

    SequenceTimer* m_timer;
    uint64_t m_session_id;
    cxx::unordered_map< uint64_t, cxx::shared_ptr<RpcSession> > m_session_map;
    int64_t  m_task_num; // 并发任务数，只包括服务处理
    int64_t  m_latest_handle;
};

} // namespace pebble

#endif // _PEBBLE_COMMON_RPC_H_
