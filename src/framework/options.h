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


#ifndef  _PEBBLE_EXTENSION_OPTIONS_H_
#define  _PEBBLE_EXTENSION_OPTIONS_H_

#include <string>

#include "common/platform.h"


namespace pebble {

/// @brief PebbleServer的全量参数定义，每个参数均有默认值，用户只需修改自己使用的即可
/// 配置参数赋值后在init和reload后生效
struct Options {
    // app
    int64_t     _app_id;            // 应用ID，默认为0
    std::string _app_key;           // 应用的key，默认为"app_key"
    int32_t     _app_instance_id;   // 应用的实例ID，默认为0

    int32_t     _app_unit_id;       // 兼容OMS，UNIT ID，默认为0
    int32_t     _app_program_id;    // 兼容OMS, PROGRAM(SERVER) ID，默认为0
    std::string _app_ctrl_cmd_addr; // 控制命令监听地址

    // coroutine
    uint32_t _co_stack_size_bytes;  // 协程栈大小（单位字节），默认为256K，非reload生效

    // log
    std::string _log_device;        // 打印输出方式 { FILE、STDOUT }，默认为FILE
    std::string _log_priority;      // 打印级别 { TRACE, DEBUG, INFO, ERROR, FATAL }，默认为INFO
    uint32_t _log_file_size_MB;     // 单个log文件的最大大小，单位为"M bytes"，默认为10M
    uint32_t _log_roll_num;         // 日志文件滚动个数，默认为10个
    std::string _log_path;          // 日志文件存储路径，默认为"./log"

    // stat
    uint32_t _stat_report_cycle_s;  // 统计输出周期，单位为秒，默认为60s
    uint32_t _stat_report_to_gdata; // 是否上报给告警分析系统 { 0:不上报 1:逐条上报 2:按统计输出周期上报 }，默认为2
    int32_t  _gdata_id;             // 由告警分析系统分配的框架的业务id，默认为7
    int32_t  _gdata_log_id;         // 由告警分析系统分配的框架的日志id，默认为10
    std::string _gdata_log_path;    // 告警分析上报写本地文件路径，默认为"./log"，非reload生效

    // flow control
    bool     _enable_flow_control;  // 是否打开流控，0 - 关闭，1 - 打开，默认为1
    uint32_t _max_msg_num_per_loop; // 每个tick最大消息处理数量，默认为100
    uint32_t _task_threshold;       // 系统并发任务门限，默认为1w
    uint32_t _message_expire_ms;    // 消息过期时间（单位ms），默认为10*1000(10s)
    uint32_t _idle_us;              // idle time by us

    // broadcast
    std::string _bc_relay_address;  // 接收其他server转发的广播消息的监听地址，非reload生效
    std::string _bc_zk_host;        // zk地址，非reload生效
    uint32_t _bc_zk_timeout_ms;     // zk连接超时时间，单位ms，默认20000ms，非reload生效

    Options();
    std::string ToString();
};

/// @brief ini配置默认字段名定义
// section
extern const char* kSectionApp;         // [app]
extern const char* kSectionCoroutine;   // [coroutine]
extern const char* kSectionLog;         // [log]
extern const char* kSectionStat;        // [stat]
extern const char* kSectionFlowControl; // [flowcontrol]
extern const char* kSectionBroadcast;   // [broadcast]


// config name
// [app]
extern const char* kAppId;
extern const char* kAppKey;
extern const char* kAppInstanceId;
extern const char* kAppUnitId;
extern const char* kAppProgramId;
extern const char* kAppCtrlCmdAddr;


// [coroutine]
extern const char* kCoStackSize;

// [log]
extern const char* kLogDevice;
extern const char* kLogPriority;
extern const char* kLogFileSize;
extern const char* kLogRollNum;
extern const char* kLogPath;

// [stat]
extern const char* kStatReportCycleS;
extern const char* kStatReportToGdata;
extern const char* kGdataId;
extern const char* kGdataLogId;
extern const char* kGdataLogPath;

// [flow_control]
extern const char* kEnableFlowControl;
extern const char* kMaxMsgNumPerLoop;
extern const char* kTaskThreshold;
extern const char* kMessageExpireMs;
extern const char* kIdleUs;

// [broadcast]
extern const char* kBcRelayAddress;
extern const char* kBcZkHost;
extern const char* kBcZkTimeoutMs;


// default values
// [app]
#define DEFAULT_APP_ID          0
#define DEFAULT_APP_KEY         "app_key"
#define DEFAULT_APP_INSTANCE_ID 0

#define DEFAULT_APP_UNIT_ID     0
#define DEFAULT_APP_PROGRAM_ID  0


// [coroutine]
#define DEFAULT_CO_STACK_SIZE   (256 * 1024)

// [log]
#define DEFAULT_LOG_DEVICE      "FILE"
#define DEFAULT_LOG_PRIORITY    "INFO"
#define DEFAULT_LOG_FILE_SIZE   10
#define DEFAULT_LOG_ROLL_NUM    10
#define DEFAULT_LOG_PATH        "./log"

// [stat]
#define DEFAULT_STAT_REPORT_CYCLE       60
#define DEFAULT_STAT_REPORT_TO_GDATA    2
#define DEFAULT_GDATA_ID        7
#define DEFAULT_GDATA_LOG_ID    10
#define DEFAULT_GDATA_LOG_PATH  "./log/gdata"

// [flow_control]
#define DEFAULT_ENABLE_FLOW_CONTROL true
#define DEFAULT_MAX_MSG_NUM_PER_LOOP    100
#define DEFAULT_TASK_THRESHOLD      (10000)
#define DEFAULT_MESSAGE_EXPIRE_MS   (10 * 1000)
#define DEFAULT_IDLE_US         (1000)

// [broadcast]
#define DEFAULT_BC_ZK_TIMEOUT_MS    20000

}  // namespace pebble
#endif   //  _PEBBLE_EXTENSION_OPTIONS_H_


