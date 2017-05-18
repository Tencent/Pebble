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

#include "framework/event_handler.inh"
#include "framework/rpc.h"
#include "framework/stat.h"
#include "framework/stat_manager.h"


namespace pebble {

void RpcEventHandler::ReportTransportQuality(int64_t handle, int32_t ret_code,
    int64_t time_cost_ms) {
    Message::ReportHandleResult(handle,
        (ret_code == kRPC_MESSAGE_EXPIRED ? 0 : ret_code), time_cost_ms);
}

void RpcEventHandler::RequestProcComplete(const std::string& name,
    int32_t result, int32_t time_cost_ms) {
    if (m_stat_manager) {
        std::string message_name("_recv_rpc_");
        message_name.append(name);
        m_stat_manager->GetStat()->AddMessageItem(message_name, result, time_cost_ms);
        m_stat_manager->Report2Gdata(message_name, result, time_cost_ms);
    }
}

void RpcEventHandler::ResponseProcComplete(const std::string& name,
    int32_t result, int32_t time_cost_ms) {
    if (m_stat_manager) {
        std::string message_name("_send_rpc_");
        message_name.append(name);
        m_stat_manager->GetStat()->AddMessageItem(message_name, result, time_cost_ms);
        m_stat_manager->Report2Gdata(message_name, result, time_cost_ms);
    }
}

void BroadcastEventHandler::RequestProcComplete(const std::string& name,
    int32_t result, int32_t time_cost_ms) {
    if (m_stat_manager) {
        std::string message_name("_broadcast_");
        message_name.append(name);
        m_stat_manager->GetStat()->AddMessageItem(message_name, result, time_cost_ms);
        m_stat_manager->Report2Gdata(message_name, result, time_cost_ms);
    }
}


}  // namespace pebble

