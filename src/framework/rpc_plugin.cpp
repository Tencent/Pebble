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
#include "framework/dr/common/dr_define.h"
#include "framework/dr/protocol/protocol.h"
#include "framework/dr/transport/buffer_transport.h"
#include "framework/pebble_rpc.h"
#include "framework/rpc_plugin.inh"
#include "src/framework/protobuf_rpc_head.h"


namespace pebble {


int32_t ProtoBufRpcPlugin::HeadEncode(const RpcHead& rpc_head, uint8_t* buff, uint32_t buff_len) {
    if (NULL == buff || 0 == buff_len) {
        PLOG_ERROR("invalid param buff = %p, buff_len = %u", buff, buff_len);
        return kRPC_INVALID_PARAM;
    }

    dr::protocol::TProtocol* encoder = m_pebble_rpc->GetCodec(PebbleRpc::kBORROW);
    if (NULL == encoder) {
        return kPEBBLE_RPC_UNKNOWN_CODEC_TYPE;
    }

    (static_cast<dr::transport::TMemoryBuffer*>(encoder->getTransport().get()))->
        resetBuffer(buff, buff_len, dr::transport::TMemoryBuffer::OBSERVE);

    int32_t len = -1;
    try {
        // 1. ProtoBufRpcHead赋值
        ProtoBufRpcHead pb_head;
        // pb_head.version         = rpc_head.m_version; // 暂时不需要版本号
        pb_head.msg_type        = rpc_head.m_message_type;
        pb_head.session_id      = rpc_head.m_session_id;
        pb_head.function_name   = rpc_head.m_function_name;

        // 2. 序列化ProtoBufRpcHead，考虑到性能不使用write(buff, bufflen)接口
        len = pb_head.write(encoder);
    } catch (TException e) {
        PLOG_ERROR("catch exception : %s", e.what());
        return kPEBBLE_RPC_ENCODE_HEAD_FAILED;
    }

    return len;
}

int32_t ProtoBufRpcPlugin::HeadDecode(const uint8_t* buff, uint32_t buff_len, RpcHead* rpc_head) {
    if (NULL == buff || 0 == buff_len || NULL == rpc_head) {
        PLOG_ERROR("invalid param buff = %p, buff_len = %u, rpchead = %p", buff, buff_len, rpc_head);
        return kRPC_INVALID_PARAM;
    }

    dr::protocol::TProtocol* decoder = m_pebble_rpc->GetCodec(PebbleRpc::kBORROW);
    if (NULL == decoder) {
        return kPEBBLE_RPC_UNKNOWN_CODEC_TYPE;
    }

    (static_cast<dr::transport::TMemoryBuffer*>(decoder->getTransport().get()))->
        resetBuffer(const_cast<uint8_t*>(buff), buff_len, dr::transport::TMemoryBuffer::OBSERVE);

    // 解消息头
    int32_t head_len = -1;
    try {
        // 1. 反序列化ProtoBufRpcHead
        ProtoBufRpcHead pb_head;
        head_len = pb_head.read(decoder);

        // 2. 给rpc_head赋值
        if (pb_head.__isset.version) {
            rpc_head->m_version       = pb_head.version;
        }
        rpc_head->m_message_type  = pb_head.msg_type;
        rpc_head->m_session_id    = pb_head.session_id;
        rpc_head->m_function_name = pb_head.function_name;
    } catch (TException e) {
        PLOG_ERROR("catch exception : %s", e.what());
        return kPEBBLE_RPC_DECODE_HEAD_FAILED;
    }

    if (rpc_head->m_message_type < kRPC_CALL
        || rpc_head->m_message_type > kRPC_ONEWAY) {
        PLOG_ERROR("message type error %d", rpc_head->m_message_type);
        return kRPC_UNKNOWN_TYPE;
    }

    return head_len;
}

int32_t ThriftRpcPlugin::HeadEncode(const RpcHead& rpc_head, uint8_t* buff, uint32_t buff_len) {
    if (NULL == buff || 0 == buff_len) {
        PLOG_ERROR("invalid param buff = %p, buff_len = %u", buff, buff_len);
        return kRPC_INVALID_PARAM;
    }

    dr::protocol::TProtocol* encoder = m_pebble_rpc->GetCodec(PebbleRpc::kBORROW);
    if (NULL == encoder) {
        return kPEBBLE_RPC_UNKNOWN_CODEC_TYPE;
    }

    (static_cast<dr::transport::TMemoryBuffer*>(encoder->getTransport().get()))->
        resetBuffer(buff, buff_len, dr::transport::TMemoryBuffer::OBSERVE);

    int32_t len = -1;
    try {
        len = encoder->writeMessageBegin(rpc_head.m_function_name,
            static_cast<pebble::dr::protocol::TMessageType>(rpc_head.m_message_type),
            rpc_head.m_session_id);
    } catch (TException e) {
        PLOG_ERROR("catch exception : %s", e.what());
        return kPEBBLE_RPC_ENCODE_HEAD_FAILED;
    }

    return len;
}

int32_t ThriftRpcPlugin::HeadDecode(const uint8_t* buff, uint32_t buff_len, RpcHead* rpc_head) {
    if (NULL == buff || 0 == buff_len || NULL == rpc_head) {
        PLOG_ERROR("invalid param buff = %p, buff_len = %u, rpchead = %p", buff, buff_len, rpc_head);
        return kRPC_INVALID_PARAM;
    }

    dr::protocol::TProtocol* decoder = m_pebble_rpc->GetCodec(PebbleRpc::kBORROW);
    if (NULL == decoder) {
        return kPEBBLE_RPC_UNKNOWN_CODEC_TYPE;
    }

    (static_cast<dr::transport::TMemoryBuffer*>(decoder->getTransport().get()))->
        resetBuffer(const_cast<uint8_t*>(buff), buff_len, dr::transport::TMemoryBuffer::OBSERVE);

    // 解消息头
    int32_t head_len = -1;
    try {
        int64_t seqid = 0;
        pebble::dr::protocol::TMessageType msg_type;
        head_len = decoder->readMessageBegin(rpc_head->m_function_name, msg_type, seqid);
        rpc_head->m_message_type = static_cast<int32_t>(msg_type);
        rpc_head->m_session_id   = static_cast<uint64_t>(seqid);
    } catch (TException e) {
        PLOG_ERROR("catch exception : %s", e.what());
        return kPEBBLE_RPC_DECODE_HEAD_FAILED;
    }

    if (rpc_head->m_message_type < dr::protocol::T_CALL
        || rpc_head->m_message_type > dr::protocol::T_ONEWAY) {
        PLOG_ERROR("message type error %d", rpc_head->m_message_type);
        return kPEBBLE_RPC_MSG_TYPE_ERROR;
    }

    return head_len;
}


} // namespace pebble

