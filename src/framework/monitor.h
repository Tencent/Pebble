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

#ifndef _PEBBLE_APP_MONITOR_H_
#define _PEBBLE_APP_MONITOR_H_

#include <vector>

#include "common/platform.h"
#include "common/time_utility.h"


namespace pebble {

enum OverLoadType {
    kNO_OVERLOAD     = 0,   // 未过载
    kMESSAGE_EXPIRED = 0x1, // 消息过期
    kTASK_OVERLOAD   = 0x2, // 并发任务数过载(超出限制)
};

/// @brief 系统过载监控接口定义
class IMonitor {
public:
    virtual ~IMonitor() {}

    /// @brief 是否过载
    /// @return 0 不过载，其他参考 @seeOverLoadType
    virtual uint32_t IsOverLoad() = 0;
};

/// @brief 系统并发任务监控，当系统未处理完毕的任务数超过限制认为过载
class TaskMonitor : public IMonitor {
public:
    TaskMonitor() : m_task_num(0), m_task_threshold(UINT32_MAX) {}
    virtual ~TaskMonitor() {}

    /// @brief 设置系统并发任务门限
    void SetTaskThreshold(uint32_t task_threshold) {
        m_task_threshold = task_threshold;
    }

    void SetTaskNum(uint32_t task_num) {
        m_task_num = task_num;
    }

    virtual uint32_t IsOverLoad() {
        return m_task_num > m_task_threshold ? kTASK_OVERLOAD : kNO_OVERLOAD;
    }

private:
    uint32_t m_task_num;
    uint32_t m_task_threshold;
};

/// @brief 消息过期监控
class MessageExpireMonitor : public IMonitor {
public:
    MessageExpireMonitor() : m_arrived_ms(0), m_expire_threshold_ms(UINT32_MAX) {}
    virtual ~MessageExpireMonitor() {}

    void SetExpireThreshold(uint32_t expire_threshold_ms) {
        m_expire_threshold_ms = expire_threshold_ms;
    }

    /// @brief 消息的时间戳
    void OnMessage(int64_t arrived_ms) {
        m_arrived_ms = arrived_ms;
    }

    virtual uint32_t IsOverLoad() {
        return (m_arrived_ms + m_expire_threshold_ms) < TimeUtility::GetCurrentMS()
            ? kMESSAGE_EXPIRED : kNO_OVERLOAD;
    }

private:
    int64_t m_arrived_ms;
    uint32_t m_expire_threshold_ms;
};

/// @brief 监控中心，根据系统负载情况和流控策略配置提供流控决策支持
class MonitorCenter {
public:
    MonitorCenter() {}
    ~MonitorCenter() {}

    /// @brief 是否过载
    /// @return 0 不过载，其他参考 @seeOverLoadType
    uint32_t IsOverLoad() {
        uint32_t overload = kNO_OVERLOAD;
        for (std::vector<IMonitor*>::iterator it = m_monitors.begin();
            it != m_monitors.end(); ++it) {
            overload |= (*it)->IsOverLoad();
        }
        return overload;
    }

    /// @brief 添加新的监控能力
    void AddMonitor(IMonitor* monitor) {
        m_monitors.push_back(monitor);
    }

    /// @brief 清空监控列表，一般用于清空系统默认的监控器，重新添加用户自定义的监控器
    void Clear() {
        m_monitors.clear();
    }

private:
    std::vector<IMonitor*> m_monitors;
};

} // namespace pebble

#endif // _PEBBLE_APP_MONITOR_H_