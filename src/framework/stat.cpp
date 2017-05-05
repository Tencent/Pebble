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

#include "framework/stat.h"


namespace pebble {

void Stat::Clear() {
    m_message_counts = 0;
    m_failure_message_counts = 0;
    m_resource_stat_temp.clear();
    m_message_stat_temp.clear();
    m_resource_stat_result.clear();
    m_message_stat_result.clear();
}

int32_t Stat::AddResourceItem(const std::string& name, float value) {
    if (name.empty()) {
        return -1;
    }

    ResourceStatItem& item = m_resource_stat_result[name];
    ResourceStatTempData& temp = m_resource_stat_temp[name];
    temp._result = &item;
    temp._count++; 
    temp._total_value += value;
    value > item._max_value ? item._max_value = value : value;
    value < item._min_value ? item._min_value = value : value;

    return 0;
}

int32_t Stat::AddMessageItem(const std::string& name, int32_t result, int32_t time_cost_ms) {
    if (name.empty()) {
        return -1;
    }

    m_message_counts++;

    MessageStatItem& item = m_message_stat_result[name];
    MessageStatTempData& temp = m_message_stat_temp[name];
    temp._result = &item;
    temp._total_count++;
    if (result != 0) {
        temp._failure_count++;
        m_failure_message_counts++;
    }
    temp._total_cost_ms += time_cost_ms;

    uint32_t time_cost = static_cast<uint32_t>(time_cost_ms);
    time_cost > item._max_cost_ms ? item._max_cost_ms = time_cost : time_cost;
    time_cost < item._min_cost_ms ? item._min_cost_ms = time_cost : time_cost;
    item._result[result]++;

    return 0;
}

const ResourceStatItem* Stat::GetResourceResultByName(const std::string& name) {
    cxx::unordered_map<std::string, ResourceStatTempData>::iterator it;
    it = m_resource_stat_temp.find(name);
    if (m_resource_stat_temp.end() == it) {
        return NULL;
    }

    CalculateResourceStatResult(&(it->second));

    return it->second._result;
}

const MessageStatItem* Stat::GetMessageResultByName(const std::string& name) {
    cxx::unordered_map<std::string, MessageStatTempData>::iterator it;
    it = m_message_stat_temp.find(name);
    if (m_message_stat_temp.end() == it) {
        return NULL;
    }

    CalculateMessageStatResult(&(it->second));

    return it->second._result;
}

const ResourceStatResult* Stat::GetAllResourceResults() {
    cxx::unordered_map<std::string, ResourceStatTempData>::iterator it;
    for (it = m_resource_stat_temp.begin(); it != m_resource_stat_temp.end(); ++it) {
        CalculateResourceStatResult(&(it->second));
    }

    return &m_resource_stat_result;
}

const MessageStatResult* Stat::GetAllMessageResults() {
    cxx::unordered_map<std::string, MessageStatTempData>::iterator it;
    for (it = m_message_stat_temp.begin(); it != m_message_stat_temp.end(); ++it) {
        CalculateMessageStatResult(&(it->second));
    }

    return &m_message_stat_result;
}

void Stat::CalculateResourceStatResult(ResourceStatTempData* resource_stat_temp) {
    if (0 == resource_stat_temp->_count) {
        return;
    }
    ResourceStatItem* result = resource_stat_temp->_result;
    result->_average_value = resource_stat_temp->_total_value / resource_stat_temp->_count;
}

void Stat::CalculateMessageStatResult(MessageStatTempData* message_stat_temp) {
    if (0 == message_stat_temp->_total_count) {
        return;
    }
    MessageStatItem* result = message_stat_temp->_result;
    result->_failure_rate    = message_stat_temp->_failure_count / message_stat_temp->_total_count;
    result->_average_cost_ms = message_stat_temp->_total_cost_ms / message_stat_temp->_total_count;
}


} // namespace pebble

