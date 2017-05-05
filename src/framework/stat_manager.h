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

#ifndef _PEBBLE_EXTENSION_STAT_MANAGER_H_
#define _PEBBLE_EXTENSION_STAT_MANAGER_H_

#include "common/platform.h"

namespace pebble {

namespace oss {
class SMonitorData;
}
class Stat;
class Timer;

/// @brief Gdata上报类型定义
typedef enum {
    kNO_REPORT = 0,         // 不上报
    kREPORT_BY_MESSAGE = 1, // 按消息(逐条)上报
    kREPORT_BY_CYCLE = 2    // 按周期上报
} GdataReportType;

class StatManager {
public:
    StatManager();
    ~StatManager();

    /// @brief 设置统计结果输出周期，包括输出到log和Gdata，修改后下周期生效
    /// @param report_cycle_s 上报周期，单位为s
    /// @return 0 成功
    /// @return <0 失败
    int32_t SetReportCycle(uint32_t report_cycle_s);

    /// @brief 设置Gdata上报参数，修改后在下一次上报生效
    /// @param report_type @see GdataReportType
    /// @param gdata_id gdata分配的id
    /// @param gdata_log_id gdata分配的logid
    /// @return 0 成功
    /// @return <0 失败
    int32_t SetGdataParameter(int32_t report_type,
        int32_t gdata_id,
        int32_t gdata_log_id);

    /// @brief 初始化StatManager
    /// @param app_id 此进程承载的业务ID 由运营系统分配
    /// @param unit_id 分区ID 由运营系统分配
    /// @param program_id 程序ID 由运营系统分配
    /// @param instance_id 实例ID 由运营系统分配
    /// @param gdata_log_path gdata log路径
    /// @return 0 成功
    /// @return <0 失败
    int32_t Init(int64_t app_id,
        int32_t unit_id,
        int32_t program_id,
        int32_t instance_id,
        const std::string& gdata_log_path);

    /// @brief 事件驱动
    /// @return 处理事件数，未处理返回0
    int32_t Update();

    /// @brief 返回Stat实例
    /// @return 非空
    Stat* GetStat() {
        return m_stat;
    }

    void Report2Gdata(const std::string& name, int32_t result, int64_t time_cost);

    /// @brief 获取程序的运行时间，单位为s
    uint64_t GetRuntimeInSecond();

private:
    int32_t InitGdataApi(int64_t app_id,
        int32_t unit_id,
        int32_t program_id,
        int32_t instance_id,
        const std::string& gdata_log_path);

    int32_t OnTimeout();

    void WriteLog();

    void ReportGdataByCycle();

    void Report2Gdata(const std::string& name,
        int32_t num, int32_t result, int64_t time_cost);

private:
    Stat* m_stat;
    Timer* m_report_timer;
    oss::SMonitorData* m_gdata_monitor;
    uint32_t m_report_cycle_s;
    int32_t m_report_gdata_type;
    int32_t m_gdata_id;
    int32_t m_gdata_log_id;
    uint64_t m_start_time_s;
};

} // namespace pebble

#endif // _PEBBLE_EXTENSION_STAT_MANAGER_H_