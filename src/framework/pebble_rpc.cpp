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

#include "common/log.h"
#include "framework/pebble_rpc.h"
#include "framework/rpc_plugin.inh"
#include "framework/rpc_util.inh"
#include "framework/dr/common/dr_define.h"
#include "framework/dr/protocol/binary_protocol.h"
#include "framework/dr/protocol/json_protocol.h"
#include "framework/dr/transport/buffer_transport.h"
#include "src/framework/exception.h"


namespace pebble {

PebbleRpc::PebbleRpc(CodeType code_type, CoroutineSchedule* coroutine_schedule) {
    m_rpc_util = new RpcUtil(this, coroutine_schedule);
    m_rpc_plugin = NULL;
    m_code_type = code_type;
    switch (m_code_type) {
        case kCODE_BINARY:
        case kCODE_JSON:
            m_rpc_plugin = new ThriftRpcPlugin(this);
            break;
        case kCODE_PB:
            m_rpc_plugin = new ProtoBufRpcPlugin(this);
            break;
        default:
            break;
    }

    for (int32_t i = 0; i < kPOLICY_BUTT; i++) {
        m_codec_array[i] = NULL;
    }
    m_buff = NULL;
    m_buff_size = 0;
}

PebbleRpc::~PebbleRpc() {
    delete m_rpc_plugin;
    delete m_rpc_util;

    for (int32_t i = 0; i < kPOLICY_BUTT; i++) {
        delete m_codec_array[i];
    }
    free(m_buff);
    m_buff_size = 0;
}

dr::protocol::TProtocol* PebbleRpc::GetCodec(MemoryPolicy mem_policy) {
    if (mem_policy < kBORROW || mem_policy >= kPOLICY_BUTT) {
        PLOG_ERROR("mem policy(%d) invalid", mem_policy);
        return NULL;
    }

    dr::protocol::TProtocol* codec = m_codec_array[mem_policy];
    if (codec != NULL) {
        (static_cast<dr::transport::TMemoryBuffer*>(codec->getTransport().get()))->resetBuffer();
        if (kCODE_JSON == m_code_type) {
            (static_cast<dr::protocol::TJSONProtocol*>(codec))->clearContext();
        }
        return codec;
    }

    // 首次创建
    cxx::shared_ptr<dr::transport::TTransport> trans;
    try {
        if (kBORROW == mem_policy) {
            trans.reset(new dr::transport::TMemoryBuffer(NULL, 0));
        } else {
            trans.reset(new dr::transport::TMemoryBuffer());
        }
    } catch (TException e) {
        PLOG_ERROR("catch transport exception : %s", e.what());
        return NULL;
    }

    switch (m_code_type) {
        case kCODE_JSON:
            codec = new dr::protocol::TJSONProtocol(trans);
            break;

        case kCODE_BINARY:
        case kCODE_PB: // Protobuf rpc head使用dr binary编码
            codec = new dr::protocol::TBinaryProtocol(trans);
            break;

        default:
            PLOG_ERROR("unsupport code type : %d", m_code_type);
            return NULL;
    }

    m_codec_array[mem_policy] = codec;
    return codec;
}

uint8_t* PebbleRpc::GetBuffer(int32_t size) {
    static const int32_t max_buff_size = 1024 * 1024 * 8;
    if (m_buff != NULL && size <= m_buff_size) {
        return m_buff;
    }
    if (size > max_buff_size) {
        PLOG_ERROR("the size %d > max buff size %d", size, max_buff_size);
        return NULL;
    }
    // 2 * size or max_buff_size
    int32_t need_realloc = std::min(size + size - m_buff_size, max_buff_size - m_buff_size);
    uint8_t* new_buff = (uint8_t*)realloc(m_buff, need_realloc);
    if (new_buff == NULL) {
        PLOG_ERROR("realloc failed, buff size %d, realloc size %d", m_buff_size, need_realloc);
        return NULL;
    }
    m_buff = new_buff;
    m_buff_size += need_realloc;
    return m_buff;
}

int32_t PebbleRpc::SendRequestSync(int64_t handle,
                    const RpcHead& rpc_head,
                    const uint8_t* buff,
                    uint32_t buff_len,
                    const OnRpcResponse& on_rsp,
                    int32_t timeout_ms) {
    return m_rpc_util->SendRequestSync(handle, rpc_head, buff, buff_len, on_rsp, timeout_ms);
}

void PebbleRpc::SendRequestParallel(int64_t handle,
                                    const RpcHead& rpc_head,
                                    const uint8_t* buff,
                                    uint32_t buff_len,
                                    const OnRpcResponse& on_rsp,
                                    int32_t timeout_ms,
                                    int32_t* ret_code,
                                    uint32_t* num_called,
                                    uint32_t* num_parallel) {
    m_rpc_util->SendRequestParallel(handle,
                                    rpc_head,
                                    buff,
                                    buff_len,
                                    on_rsp,
                                    timeout_ms,
                                    ret_code,
                                    num_called,
                                    num_parallel);
}

int32_t PebbleRpc::HeadEncode(const RpcHead& rpc_head, uint8_t* buff, uint32_t buff_len) {
    if (m_rpc_plugin) {
        return m_rpc_plugin->HeadEncode(rpc_head, buff, buff_len);
    }
    return kPEBBLE_RPC_UNKNOWN_CODEC_TYPE;
}

int32_t PebbleRpc::HeadDecode(const uint8_t* buff, uint32_t buff_len, RpcHead* rpc_head) {
    if (m_rpc_plugin) {
        return m_rpc_plugin->HeadDecode(buff, buff_len, rpc_head);
    }
    return kPEBBLE_RPC_UNKNOWN_CODEC_TYPE;
}

int32_t PebbleRpc::ExceptionEncode(const RpcException& rpc_exception,
    uint8_t* buff, uint32_t buff_len) {

    dr::protocol::TProtocol* encoder = GetCodec(kBORROW);
    if (NULL == encoder) {
        return kPEBBLE_RPC_UNKNOWN_CODEC_TYPE;
    }
    (static_cast<dr::transport::TMemoryBuffer*>(encoder->getTransport().get()))->
        resetBuffer(buff, buff_len, dr::transport::TMemoryBuffer::OBSERVE);

    int32_t len = 0;
    try {
        Exception ex;
        ex.type    = rpc_exception.m_error_code;
        ex.message = rpc_exception.m_message;

        len += ex.write(encoder);
        len += encoder->writeMessageEnd();
        encoder->getTransport()->writeEnd();
    } catch (TException e) {
        PLOG_ERROR("catch exception : %s", e.what());
        return kPEBBLE_RPC_ENCODE_BODY_FAILED;
    }

    return len;
}

int32_t PebbleRpc::ExceptionDecode(const uint8_t* buff, uint32_t buff_len,
    RpcException* rpc_exception) {

    dr::protocol::TProtocol* decoder = GetCodec(kBORROW);
    if (NULL == decoder) {
        return kPEBBLE_RPC_UNKNOWN_CODEC_TYPE;
    }
    (static_cast<dr::transport::TMemoryBuffer*>(decoder->getTransport().get()))->
        resetBuffer(const_cast<uint8_t*>(buff), buff_len, dr::transport::TMemoryBuffer::OBSERVE);

    int32_t len = 0;
    try {
        Exception ex;

        len += ex.read(decoder);
        len += decoder->readMessageEnd();
        decoder->getTransport()->readEnd();

        rpc_exception->m_error_code   = ex.type;
        rpc_exception->m_message      = ex.message;
    } catch (TException e) {
        PLOG_ERROR("catch exception : %s", e.what());
        return kPEBBLE_RPC_DECODE_BODY_FAILED;
    }

    return len;
}

int32_t PebbleRpc::ProcessRequest(int64_t handle, const RpcHead& rpc_head,
    const uint8_t* buff, uint32_t buff_len) {
    return m_rpc_util->ProcessRequest(handle, rpc_head, buff, buff_len);
}

int32_t PebbleRpc::AddService(cxx::shared_ptr<IPebbleRpcService> service) {
    std::string service_name(service->Name());
    if (service_name.empty()) {
        PLOG_ERROR("service name is empty");
        return kRPC_INVALID_PARAM;
    }

    if (m_services.insert({service_name, service}).second == false) {
        PLOG_ERROR("service %s is already existed", service_name.c_str());
        return kPEBBLE_RPC_SERVICE_ALREADY_EXISTED;
    }

    int32_t result = service->RegisterServiceFunction();
    if (result != kRPC_SUCCESS) {
        PLOG_ERROR("service %s RegisterServiceFunction failed(%d)", service_name.c_str(), result);
        return kPEBBLE_RPC_SERVICE_ADD_FAILED;
    }

    return kRPC_SUCCESS;
}

} // namespace pebble

