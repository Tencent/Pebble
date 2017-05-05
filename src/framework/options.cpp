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

#include "common/log.h"
#include "framework/options.h"


namespace pebble {

// 配置参数的有效性交给具体使用的模块，Options不关心正确性
Options::Options() {
    // app
    _app_id                 = DEFAULT_APP_ID;
    _app_key                = DEFAULT_APP_KEY;
    _app_instance_id        = DEFAULT_APP_INSTANCE_ID;
    _app_unit_id            = DEFAULT_APP_UNIT_ID;
    _app_program_id         = DEFAULT_APP_PROGRAM_ID;

    // coroutine
    _co_stack_size_bytes    = DEFAULT_CO_STACK_SIZE;

    // log
    _log_device             = DEFAULT_LOG_DEVICE;
    _log_priority           = DEFAULT_LOG_PRIORITY;
    _log_file_size_MB       = DEFAULT_LOG_FILE_SIZE;
    _log_roll_num           = DEFAULT_LOG_ROLL_NUM;
    _log_path               = DEFAULT_LOG_PATH;

    // stat
    _stat_report_cycle_s    = DEFAULT_STAT_REPORT_CYCLE;
    _stat_report_to_gdata   = DEFAULT_STAT_REPORT_TO_GDATA;
    _gdata_id               = DEFAULT_GDATA_ID;
    _gdata_log_id           = DEFAULT_GDATA_LOG_ID;
    _gdata_log_path         = DEFAULT_GDATA_LOG_PATH;

    // flow control
    _enable_flow_control    = DEFAULT_ENABLE_FLOW_CONTROL;
    _max_msg_num_per_loop   = DEFAULT_MAX_MSG_NUM_PER_LOOP;
    _task_threshold         = DEFAULT_TASK_THRESHOLD;
    _message_expire_ms      = DEFAULT_MESSAGE_EXPIRE_MS;
    _idle_us                = DEFAULT_IDLE_US;

    // broadcast
    _bc_zk_timeout_ms       = DEFAULT_BC_ZK_TIMEOUT_MS;
}

std::string Options::ToString() {
    std::ostringstream oss;
    oss << "Pebble Options:\n"
        << "[" << kSectionApp << "]\n"
            << kAppId               << " = " << _app_id               << "\n"
            << kAppKey              << " = " << _app_key              << "\n"
            << kAppInstanceId       << " = " << _app_instance_id      << "\n"
            << kAppUnitId           << " = " << _app_unit_id          << "\n"
            << kAppProgramId        << " = " << _app_program_id       << "\n"
            << kAppCtrlCmdAddr      << " = " << _app_ctrl_cmd_addr    << "\n"
        << "[" << kSectionCoroutine << "]\n"
            << kCoStackSize         << " = " << _co_stack_size_bytes  << "\n"
        << "[" << kSectionLog << "]\n"
            << kLogDevice           << " = " << _log_device           << "\n"
            << kLogPriority         << " = " << _log_priority         << "\n"
            << kLogFileSize         << " = " << _log_file_size_MB     << "\n"
            << kLogRollNum          << " = " << _log_roll_num         << "\n"
            << kLogPath             << " = " << _log_path             << "\n"
        << "[" << kSectionStat << "]\n"
            << kStatReportCycleS    << " = " << _stat_report_cycle_s  << "\n"
            << kStatReportToGdata   << " = " << _stat_report_to_gdata << "\n"
            << kGdataId             << " = " << _gdata_id             << "\n"
            << kGdataLogId          << " = " << _gdata_log_id         << "\n"
            << kGdataLogPath        << " = " << _gdata_log_path       << "\n"
        << "[" << kSectionFlowControl << "]\n"
            << kEnableFlowControl   << " = " << _enable_flow_control  << "\n"
            << kMaxMsgNumPerLoop    << " = " << _max_msg_num_per_loop << "\n"
            << kTaskThreshold       << " = " << _task_threshold       << "\n"
            << kMessageExpireMs     << " = " << _message_expire_ms    << "\n"
            << kIdleUs              << " = " << _idle_us              << "\n"
        << "[" << kSectionBroadcast << "]\n"
            << kBcRelayAddress      << " = " << _bc_relay_address     << "\n"
            << kBcZkHost            << " = " << _bc_zk_host           << "\n"
            << kBcZkTimeoutMs       << " = " << _bc_zk_timeout_ms     << "\n"
        ;

    return oss.str();
}

// section
const char* kSectionApp         = "app";
const char* kSectionCoroutine   = "coroutine";
const char* kSectionLog         = "log";
const char* kSectionStat        = "stat";
const char* kSectionFlowControl = "flow_control";
const char* kSectionBroadcast   = "broadcast";


// config name
// [app]
const char* kAppId              = "app_id";
const char* kAppKey             = "app_key";
const char* kAppInstanceId      = "instance_id";
const char* kAppUnitId          = "unit_id";
const char* kAppProgramId       = "program_id";
const char* kAppCtrlCmdAddr     = "ctrl_cmd_address";

// [coroutine]
const char* kCoStackSize        = "stack_size";

// [log]
const char* kLogDevice          = "device";
const char* kLogPriority        = "priority";
const char* kLogFileSize        = "file_size";
const char* kLogRollNum         = "roll_num";
const char* kLogPath            = "log_path";

// [stat]
const char* kStatReportCycleS   = "report_cycle_s";
const char* kStatReportToGdata  = "report_to_gdata";
const char* kGdataId            = "gdata_id";
const char* kGdataLogId         = "gdata_log_id";
const char* kGdataLogPath       = "gdata_log_path";

// [flow_control]
const char* kEnableFlowControl  = "enable";
const char* kMaxMsgNumPerLoop   = "msg_num_per_loop";
const char* kTaskThreshold      = "task_threshold";
const char* kMessageExpireMs    = "message_expire_ms";
const char* kIdleUs             = "idle_us";

// [broadcast]
const char* kBcRelayAddress     = "relay_address";
const char* kBcZkHost           = "zk_host";
const char* kBcZkTimeoutMs      = "zk_connect_timeout_ms";

}  // namespace pebble


