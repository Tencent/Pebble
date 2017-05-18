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
#include <time.h>

#include "common/log.h"
#include "common/string_utility.h"
#include "common/timer.h"
#include "framework/gdata_api.h"
#include "framework/options.h"
#include "framework/stat.h"
#include "framework/stat_manager.h"


namespace pebble {

StatManager::StatManager() {
    m_stat          = new Stat();
    m_report_timer  = new SequenceTimer(); // 这里使用顺序定时器，避免浪费fd资源
    m_gdata_monitor = NULL;

    m_report_cycle_s    = DEFAULT_STAT_REPORT_CYCLE;
    m_report_gdata_type = DEFAULT_STAT_REPORT_TO_GDATA;
    m_gdata_id          = DEFAULT_GDATA_ID;
    m_gdata_log_id      = DEFAULT_GDATA_LOG_ID;
    m_start_time_s      = 0;
}

StatManager::~StatManager() {
    WriteLog();
    delete m_report_timer;
    delete m_gdata_monitor;
    delete m_stat;
    oss::CLogDataAPI::FiniDataLog();
}

int32_t StatManager::SetReportCycle(uint32_t report_cycle_s) {
    if (0 == report_cycle_s) {
        return -1;
    }
    m_report_cycle_s = report_cycle_s;
    return 0;
}

int32_t StatManager::SetGdataParameter(int32_t report_type,
    int32_t gdata_id,
    int32_t gdata_log_id) {
    if (report_type < kNO_REPORT ||
        report_type > kREPORT_BY_CYCLE) {
        return -1;
    }
    m_report_gdata_type = report_type;
    m_gdata_id          = gdata_id;
    m_gdata_log_id      = gdata_log_id;
    return 0;
}

int32_t StatManager::Init(int64_t app_id, int32_t unit_id, int32_t program_id, int32_t instance_id,
    const std::string& gdata_log_path) {

    m_start_time_s = time(NULL);

    TimeoutCallback on_timeout = cxx::bind(&StatManager::OnTimeout, this);
    int64_t ret = m_report_timer->StartTimer(m_report_cycle_s * 1000, on_timeout);
    if (ret < 0) {
        PLOG_ERROR("start report timer failed(%d:%s)", ret, m_report_timer->GetLastError());
        return -1;
    }

    return InitGdataApi(app_id, unit_id, program_id, instance_id, gdata_log_path);
}

int32_t StatManager::InitGdataApi(int64_t app_id, int32_t unit_id, int32_t program_id,
    int32_t instance_id, const std::string& gdata_log_path) {

    std::ostringstream o_app;
    o_app << app_id;
    std::ostringstream o_unit;
    o_unit << unit_id;
    std::ostringstream o_program;
    o_program << program_id;
    std::ostringstream o_instance;
    o_instance << instance_id;

    pebble::oss::SUniqueSeriesID un_id(o_app.str(), o_unit.str(), o_program.str(), o_instance.str());

    int ret = pebble::oss::CLogDataAPI::InitDataLog(un_id, gdata_log_path);
    if (ret != 0) {
        PLOG_ERROR("init data log failed(%d)", ret);
        return -1;
    }

    if (!m_gdata_monitor) {
        delete m_gdata_monitor;
    }
    m_gdata_monitor = new oss::SMonitorData(m_gdata_id, m_gdata_log_id);
    m_gdata_monitor->type.assign("RPC");

    return 0;
}

int32_t StatManager::Update() {
    return m_report_timer->Update();
}

int32_t StatManager::OnTimeout() {
    WriteLog();
    ReportGdataByCycle();
    m_stat->Clear();
    return m_report_cycle_s * 1000;
}

void StatManager::WriteLog() {
    static const int32_t BUFF_LEN = 4000;
    static const int32_t BUFF_WATER_LINE = 3600;
    char buff[BUFF_LEN] = {0};

    int32_t len = 0;

    len += snprintf(buff + len, BUFF_LEN - len, "Pebble Resource Stat(cycle = 1s)\n");

    const ResourceStatResult* resource_result = m_stat->GetAllResourceResults();
    cxx::unordered_map<std::string, ResourceStatItem>::const_iterator it1 = resource_result->begin();
    for (; it1 != resource_result->end(); ++it1) {
        if (len > BUFF_WATER_LINE) {
            PLOG_STAT(buff);
            len = 0;
        }

        const ResourceStatItem& result = it1->second;
        len += snprintf(buff + len, BUFF_LEN - len,
            "\t%s:{avg:%.2f,max:%.2f,min:%.2f}\n",
            it1->first.c_str(), result._average_value, result._max_value, result._min_value);
    }

    float _message_per_second = 0;
    float _failurate_rate = 0;
    if (m_report_cycle_s != 0) {
        _message_per_second = m_stat->GetAllMessageCounts() / m_report_cycle_s;
    }
    if (m_stat->GetAllMessageCounts() != 0) {
        _failurate_rate = 100.f * m_stat->GetAllFailureMessageCounts() / m_stat->GetAllMessageCounts();
    }

    len += snprintf(buff + len, BUFF_LEN - len,
        "\nPebble Message Stat(cycle = %ds)\n_message:%.2f/s _failure_rate:%.2f\n",
        m_report_cycle_s, _message_per_second, _failurate_rate);

    const MessageStatResult* message_result = m_stat->GetAllMessageResults();
    cxx::unordered_map<std::string, MessageStatItem>::const_iterator it2 = message_result->begin();
    cxx::unordered_map<int32_t, uint32_t>::const_iterator rit;
    for (; it2 != message_result->end(); ++it2) {
        if (len > BUFF_WATER_LINE) {
            PLOG_STAT(buff);
            len = 0;
        }

        const MessageStatItem& result = it2->second;
        len += snprintf(buff + len, BUFF_LEN - len,
            "\t%s:{failure_rate:%.2f,cost:{avg:%u,max:%u,min:%u}}",
            it2->first.c_str(), result._failure_rate,
            result._average_cost_ms, result._max_cost_ms, result._min_cost_ms);

        len += snprintf(buff + len, BUFF_LEN - len, " err:num{");
        for (rit = result._result.begin(); rit != result._result.end(); ++rit) {
            len += snprintf(buff + len, BUFF_LEN - len, "%d:%u ", rit->first, rit->second);
        }
        len += snprintf(buff + len, BUFF_LEN - len, "}\n");
    }

    if (len > 0) {
        PLOG_STAT(buff);
    }
}

void StatManager::ReportGdataByCycle() {
    if (m_report_gdata_type != kREPORT_BY_CYCLE) {
        return;
    }

    const MessageStatResult* message_result = m_stat->GetAllMessageResults();
    cxx::unordered_map<std::string, MessageStatItem>::const_iterator it = message_result->begin();
    cxx::unordered_map<int32_t, uint32_t>::const_iterator rit;
    for (; it != message_result->end(); ++it) {
        for (rit = it->second._result.begin(); rit != it->second._result.end(); ++rit) {
            Report2Gdata(it->first, rit->second, rit->first, it->second._average_cost_ms);
        }
    }
}

void StatManager::Report2Gdata(const std::string& name,
    int32_t result, int64_t time_cost) {

    if (m_report_gdata_type != kREPORT_BY_MESSAGE) {
        return;
    }

    Report2Gdata(name, 1, result, time_cost);
}

void StatManager::Report2Gdata(const std::string& name,
    int32_t num, int32_t result, int64_t time_cost) {

    m_gdata_monitor->interface_name = name;
    m_gdata_monitor->cost_time      = time_cost;
    m_gdata_monitor->counts         = num;
    m_gdata_monitor->return_type    = (result != 0 ? pebble::oss::kSYSERR : pebble::oss::kSUC);
    m_gdata_monitor->return_code    = result;
    m_gdata_monitor->datetime       = time(NULL);
    pebble::oss::CLogDataAPI::LogMonitor(*m_gdata_monitor);
}

uint64_t StatManager::GetRuntimeInSecond() {
    return time(NULL) - m_start_time_s;
}

} // namespace pebble

