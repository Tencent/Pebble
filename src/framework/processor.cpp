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
#include "framework/processor.h"


namespace pebble {

IProcessor::IProcessor() {
    m_event_handler = NULL;
}

IProcessor::~IProcessor() {
}

int32_t IProcessor::SetSendFunction(const SendFunction& send, const SendVFunction& sendv) {
    if (!send || !sendv) {
        PLOG_ERROR("param invalid: !send = %d, !sendv = %d", !send, !sendv);
        return kPROCESSOR_INVALID_PARAM;
    }
    m_send = send;
    m_sendv = sendv;
    return 0;
}

int32_t IProcessor::SetBroadcastFunction(const BroadcastFunction& broadcast,
        const BroadcastVFunction& broadcastv) {
    if (!broadcast || !broadcastv) {
        PLOG_ERROR("param invalid: !broadcast = %d, !broadcastv = %d", !broadcast, !broadcastv);
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
    return kPROCESSOR_EMPTY_SEND;
}

int32_t IProcessor::SendV(int64_t handle, uint32_t msg_frag_num,
                            const uint8_t* msg_frag[], uint32_t msg_frag_len[], int32_t flag) {
    if (m_sendv) {
        return m_sendv(handle, msg_frag_num, msg_frag, msg_frag_len, flag);
    }
    return kPROCESSOR_EMPTY_SEND;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// 避免全局变量构造、析构顺序问题
static cxx::unordered_map<int32_t, cxx::shared_ptr<ProcessorFactory> > * g_processor_factory_map = NULL;
struct ProcessorFactoryMapHolder {
    ProcessorFactoryMapHolder() {
        g_processor_factory_map = &_processor_factory_map;
    }
    ~ProcessorFactoryMapHolder() {
        g_processor_factory_map = NULL;
    }
    cxx::unordered_map<int32_t, cxx::shared_ptr<ProcessorFactory> > _processor_factory_map;
};

cxx::shared_ptr<ProcessorFactory> GetProcessorFactory(int32_t type) {
    cxx::shared_ptr<ProcessorFactory> null_factory;
    if (!g_processor_factory_map) {
        PLOG_ERROR("processor factory map not inited.");
        return null_factory;
    }

    cxx::unordered_map<int32_t, cxx::shared_ptr<ProcessorFactory> >::iterator it =
        g_processor_factory_map->find(type);
    if (it != g_processor_factory_map->end()) {
        return it->second;
    }

    PLOG_ERROR("processor type %d not registered.", type);
    return null_factory;
}

int32_t SetProcessorFactory(int32_t type, const cxx::shared_ptr<ProcessorFactory>& factory) {
    static ProcessorFactoryMapHolder processor_factory_map_holder;
    if (!factory) {
        PLOG_ERROR("factory is null");
        return kPROCESSOR_INVALID_PARAM;
    }
    if (g_processor_factory_map == NULL) {
        PLOG_ERROR("processor factory map not inited.");
        return kPROCESSOR_FACTORY_MAP_NULL;
    }
    if (false == g_processor_factory_map->insert({type, factory}).second) {
        PLOG_ERROR("processor type %d is already registered", type);
        return kPROCESSOR_FACTORY_EXISTED;
    }
    return 0;
}

} // namespace pebble

