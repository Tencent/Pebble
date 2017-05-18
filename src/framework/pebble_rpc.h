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

#ifndef _PEBBLE_FRAMEWORK_PEBBLE_RPC_H_
#define _PEBBLE_FRAMEWORK_PEBBLE_RPC_H_

#include "framework/rpc.h"

namespace pebble {

/// @brief Pebble RPC错误码定义
typedef enum {
    kPEBBLE_RPC_ERROR_BASE              = kRPC_PEBBLE_RPC_ERROR_BASE,
    kPEBBLE_RPC_UNKNOWN_CODEC_TYPE      = kPEBBLE_RPC_ERROR_BASE - 1,  // 编码类型错误
    kPEBBLE_RPC_MISS_RESULT             = kPEBBLE_RPC_ERROR_BASE - 2,  // 响应包无结果
    kPEBBLE_RPC_ENCODE_HEAD_FAILED      = kPEBBLE_RPC_ERROR_BASE - 3,  // 编码RPC头失败
    kPEBBLE_RPC_DECODE_HEAD_FAILED      = kPEBBLE_RPC_ERROR_BASE - 4,  // 解码RPC头失败
    kPEBBLE_RPC_ENCODE_BODY_FAILED      = kPEBBLE_RPC_ERROR_BASE - 5,  // 编码RPC消息体失败
    kPEBBLE_RPC_DECODE_BODY_FAILED      = kPEBBLE_RPC_ERROR_BASE - 6,  // 解码RPC消息体失败
    kPEBBLE_RPC_MSG_TYPE_ERROR          = kPEBBLE_RPC_ERROR_BASE - 7,  // 消息类型错误
    kPEBBLE_RPC_MSG_LENGTH_ERROR        = kPEBBLE_RPC_ERROR_BASE - 8,  // 消息长度错误
    kPEBBLE_RPC_SERVICE_ALREADY_EXISTED = kPEBBLE_RPC_ERROR_BASE - 9,  // 服务已经注册
    kPEBBLE_RPC_SERVICE_ADD_FAILED      = kPEBBLE_RPC_ERROR_BASE - 10, // 服务注册失败
    kPEBBLE_RPC_INSUFFICIENT_MEMORY     = kPEBBLE_RPC_ERROR_BASE - 11, // 内存不足
} PebbleRpcErrorCode;

class PebbleRpcErrorStringRegister {
public:
    static void RegisterErrorString() {
        SetErrorString(kPEBBLE_RPC_UNKNOWN_CODEC_TYPE, "unknown codec type");
        SetErrorString(kPEBBLE_RPC_MISS_RESULT, "miss result");
        SetErrorString(kPEBBLE_RPC_ENCODE_HEAD_FAILED, "encode head failed");
        SetErrorString(kPEBBLE_RPC_DECODE_HEAD_FAILED, "decode head failed");
        SetErrorString(kPEBBLE_RPC_ENCODE_BODY_FAILED, "encode body failed");
        SetErrorString(kPEBBLE_RPC_DECODE_BODY_FAILED, "decode body failed");
        SetErrorString(kPEBBLE_RPC_MSG_TYPE_ERROR, "msg type error");
        SetErrorString(kPEBBLE_RPC_MSG_LENGTH_ERROR, "msg length error");
        SetErrorString(kPEBBLE_RPC_SERVICE_ALREADY_EXISTED, "service already exist");
        SetErrorString(kPEBBLE_RPC_SERVICE_ADD_FAILED, "add service failed");
        SetErrorString(kPEBBLE_RPC_INSUFFICIENT_MEMORY, "infufficient memory");
    }
};


/// @brief Pebble RPC消息编码类型定义
typedef enum {
    kCODE_BINARY  = 0,  // thrift binary protocol
    kCODE_JSON,         // thrift json protocol
    kCODE_PB,           // protobuff protocol
    kCODE_BUTT
} CodeType;


class CoroutineSchedule;
class IPebbleRpcService;
class RpcPlugin;
class RpcUtil;
namespace dr {
namespace protocol {
class TProtocol;
}
}

/// @brief PebbleRpc封装同步、并行处理，服务注册等基本能力，和IDL无关
/// 框架内部通用能力如异常处理等编解码使用dr完成
class PebbleRpc : public IRpc {
public:
    PebbleRpc(CodeType code_type, CoroutineSchedule* coroutine_schedule);
    virtual ~PebbleRpc();

    /// @brief 添加服务，将用户实现的服务注册到PebbleRpc中
    /// @param service 实现了IDL生成服务接口的对象指针，对象内存由用户管理
    /// @return 0 成功
    /// @return 非0 失败 @see PebbleRpcErrorCode
    template<typename Class>
    int32_t AddService(Class* service);

public:
    /// @brief 编解码内存策略
    /// @note 内部使用，用户无需关注
    enum MemoryPolicy {
        kBORROW = 0,
        kMALLOC = 1,
        kPOLICY_BUTT
    };

    /// @brief 获取编解码器
    /// @note 内部使用，用户无需关注
    dr::protocol::TProtocol* GetCodec(MemoryPolicy mem_policy);

    /// @brief 获取内存buffer
    /// @note 内部使用，用户无需关注
    uint8_t* GetBuffer(int32_t size);

    /// @brief stub同步发送接口
    /// @note 内部使用，用户无需关注
    int32_t SendRequestSync(int64_t handle,
                    const RpcHead& rpc_head,
                    const uint8_t* buff,
                    uint32_t buff_len,
                    const OnRpcResponse& on_rsp,
                    int32_t timeout_ms);

    /// @brief stub并行发送接口
    /// @note 内部使用，用户无需关注
    void SendRequestParallel(int64_t handle,
                             const RpcHead& rpc_head,
                             const uint8_t* buff,
                             uint32_t buff_len,
                             const OnRpcResponse& on_rsp,
                             int32_t timeout_ms,
                             int32_t* ret_code,
                             uint32_t* num_called,
                             uint32_t* num_parallel);

private:
    virtual int32_t HeadEncode(const RpcHead& rpc_head, uint8_t* buff, uint32_t buff_len);

    virtual int32_t HeadDecode(const uint8_t* buff, uint32_t buff_len, RpcHead* rpc_head);

    virtual int32_t ExceptionEncode(const RpcException& rpc_exception,
        uint8_t* buff, uint32_t buff_len);

    virtual int32_t ExceptionDecode(const uint8_t* buff, uint32_t buff_len,
        RpcException* rpc_exception);

    virtual int32_t ProcessRequest(int64_t handle, const RpcHead& rpc_head,
        const uint8_t* buff, uint32_t buff_len);

    int32_t AddService(cxx::shared_ptr<IPebbleRpcService> service);

protected:
    CodeType m_code_type;
    RpcPlugin* m_rpc_plugin;
    RpcUtil* m_rpc_util;
    dr::protocol::TProtocol* m_codec_array[kPOLICY_BUTT];
    uint8_t* m_buff;
    int32_t m_buff_size;
    cxx::unordered_map<std::string, cxx::shared_ptr<IPebbleRpcService> > m_services;
};

template<typename Class> class GenServiceHandler;

template<typename Class>
int32_t PebbleRpc::AddService(Class* service) {
    if (NULL == service) {
        return kRPC_INVALID_PARAM;
    }
    return AddService(GenServiceHandler<typename Class::__InterfaceType>()(this, service));
}


/// @brief PebbleRpc IDL生成代码服务端骨架代码接口
/// @note 内部使用，用户无需关注
class IPebbleRpcService {
public:
    virtual ~IPebbleRpcService() {}
    virtual int32_t RegisterServiceFunction() = 0;
    virtual std::string Name() = 0;
};

} // namespace pebble

#endif // _PEBBLE_FRAMEWORK_PEBBLE_RPC_H_