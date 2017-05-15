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

#include "framework/processor.h"


namespace pebble {

class ProcessorErrorStringRegister {
public:
    ProcessorErrorStringRegister() {
        SetErrorString(kPROCESSOR_INVALID_PARAM, "invalid paramater");
        SetErrorString(kPROCESSOR_EMPTY_SEND, "not set send or sendv function");
    }
};
static ProcessorErrorStringRegister s_processor_error_string_register;


IProcessor::IProcessor() {
    m_event_handler = NULL;
    m_last_error[0] = 0;
}

IProcessor::~IProcessor() {
}

int32_t IProcessor::SetSendFunction(const SendFunction& send, const SendVFunction& sendv) {
    if (!send || !sendv) {
        _LOG_LAST_ERROR("param invalid: !send = %d, !sendv = %d", !send, !sendv);
        return kPROCESSOR_INVALID_PARAM;
    }
    m_send = send;
    m_sendv = sendv;
    return 0;
}

int32_t IProcessor::SetBroadcastFunction(const BroadcastFunction& broadcast,
        const BroadcastVFunction& broadcastv) {
    if (!broadcast || !broadcastv) {
        _LOG_LAST_ERROR("param invalid: !broadcast = %d, !broadcastv = %d", !broadcast, !broadcastv);
        return kPROCESSOR_INVALID_PARAM;
    }
    m_broadcast = broadcast;
    m_broadcastv = broadcastv;
    return 0;
}

int32_t IProcessor::SetEventHandler(IEventHandler* event_handler) {
    m_event_handler = event_handler;
    return 0;
}

int32_t IProcessor::Send(int64_t handle, const uint8_t* msg, uint32_t msg_len, int32_t flag) {
    if (m_send) {
        return m_send(handle, msg, msg_len, flag);
    }
    _LOG_LAST_ERROR("send function not set yet");
    return kPROCESSOR_EMPTY_SEND;
}

int32_t IProcessor::SendV(int64_t handle, uint32_t msg_frag_num,
                            const uint8_t* msg_frag[], uint32_t msg_frag_len[], int32_t flag) {
    if (m_sendv) {
        return m_sendv(handle, msg_frag_num, msg_frag, msg_frag_len, flag);
    }
    _LOG_LAST_ERROR("sendv function not set yet");
    return kPROCESSOR_EMPTY_SEND;
}

const char* IProcessor::GetLastError() const {
    return m_last_error;
}


} // namespace pebble

