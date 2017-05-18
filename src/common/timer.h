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

#ifndef _PEBBLE_COMMON_TIMER_H_
#define _PEBBLE_COMMON_TIMER_H_

#include <list>

#include "common/error.h"
#include "common/platform.h"


namespace pebble {


/// @brief Timer模块错误码定义
typedef enum {
    kTIMER_ERROR_BASE       = TIMER_ERROR_CODE_BASE,
    kTIMER_INVALID_PARAM    = kTIMER_ERROR_BASE - 1, // 参数错误
    kTIMER_NUM_OUT_OF_RANGE = kTIMER_ERROR_BASE - 2, // 定时器数量超出限制范围
    kTIMER_UNEXISTED        = kTIMER_ERROR_BASE - 3, // 定时器不存在
    kSYSTEM_ERROR           = kTIMER_ERROR_BASE - 4, // 系统错误
} TimerErrorCode;

class TimerErrorStringRegister {
public:
    static void RegisterErrorString() {
        SetErrorString(kTIMER_INVALID_PARAM, "invalid paramater");
        SetErrorString(kTIMER_NUM_OUT_OF_RANGE, "number of timer out of range");
        SetErrorString(kTIMER_UNEXISTED, "timer unexist");
        SetErrorString(kSYSTEM_ERROR, "system error");
    }
};


/// @brief Timer超时回调函数返回码定义
typedef enum {
    kTIMER_BE_REMOVED   = -1,   // 超时后timer被停止并remove
    kTIMER_BE_CONTINUED = 0,    // 超时后timer仍然保持，重新计时
    kTIMER_BE_RESETED           // 超时后timer仍然保持，使用返回值作为超时时间(ms)重新计时
} OnTimerCallbackReturnCode;

/// @brief 定时器超时回调函数定义
/// @return kTIMER_BE_REMOVED 停止并删除定时器
/// @return kTIMER_BE_CONTINUED 重启定时器
/// @return >0 使用返回值作为新的超时时间(ms)重启定时器
/// @note 超时回调中不能有阻塞操作
/// @see OnTimerCallbackReturnCode
typedef cxx::function<int32_t()> TimeoutCallback;


/// @brief 定时器接口
class Timer {
public:
    virtual ~Timer() {}

    /// @brief 启动定时器
    /// @param timeout_ms 超时时间(>0)，单位为毫秒
    /// @param cb 定时器超时回调函数
    /// @return >=0 定时器ID
    /// @return <0 创建失败 @see TimerErrorCode
    virtual int64_t StartTimer(uint32_t timeout_ms, const TimeoutCallback& cb) = 0;

    /// @brief 停止定时器
    /// @param timer_id StartTimer时返回的ID
    /// @return 0 成功
    /// @return <0 失败 @see TimerErrorCode
    virtual int32_t StopTimer(int64_t timer_id) = 0;

    /// @brief 定时器驱动
    /// @return 超时定时器数，为0时表示本轮无定时器超时
    virtual int32_t Update() = 0;

    /// @brief 返回最后一次的错误信息描述
    virtual const char* GetLastError() const { return NULL; }

    /// @brief 获取定时器数目
    virtual int64_t GetTimerNum() { return 0; }
};

#if 0
/// @brief 基于timerfd实现的定时器\n
///   复杂度:start O(1)，timeout O(1) 为保持timerid的一致性(非fd)，stop O(lgn)
/// @note 定时器数量取决于进程支持的fd数量
class FdTimer : public Timer {
public:
    FdTimer();
    virtual ~FdTimer();

    /// @see Timer::StartTimer
    /// @note 最大定时器数默认为1024个，需要增加请调用SetMaxTimer设置
    virtual int64_t StartTimer(uint32_t timeout_ms, const TimeoutCallback& cb);

    /// @see Timer::StopTimer
    virtual int32_t StopTimer(int64_t timer_id);

    /// @see Timer::Update
    virtual int32_t Update();

    /// @brief 设置最大支持的定时器数，默认为1024
    /// @param num 最大定时器数
    void SetMaxTimerNum(uint32_t num) {
        m_max_timer_num = num;
    }

    /// @see Timer::LastErrorStr
    virtual const char* GetLastError() const {
        return m_last_error;
    }

    /// @see Timer::GetTimerNum
    virtual int64_t GetTimerNum() {
        return m_timers.size();
    }

private:
    struct TimerItem {
        TimerItem() {
            fd = -1;
            id = -1;
        }

        int32_t fd;
        int64_t id;
        TimeoutCallback cb;
    };

private:
    int32_t  m_epoll_fd;
    uint32_t m_max_timer_num;
    int64_t  m_timer_seqid;
    std::tr1::unordered_map<int64_t, TimerItem*> m_timers;
    char m_last_error[256];
};
#endif

/// @brief 顺序定时器，按超时时间组织，每个超时时间维护一个列表，先加入先超时
///     适合一组离散的单次超时处理，如RPC的请求、协程的超时等
///     复杂度:start O(1gn)，timeout O(1)，stop O(1gn)
class SequenceTimer : public Timer {
public:
    SequenceTimer();
    virtual ~SequenceTimer();

    /// @see Timer::StartTimer
    virtual int64_t StartTimer(uint32_t timeout_ms, const TimeoutCallback& cb);

    /// @see Timer::StopTimer
    virtual int32_t StopTimer(int64_t timer_id);

    /// @see Timer::Update
    virtual int32_t Update();

    /// @see Timer::LastErrorStr
    virtual const char* GetLastError() const {
        return m_last_error;
    }

    /// @see Timer::GetTimerNum
    virtual int64_t GetTimerNum() {
        return m_id_2_timer.size();
    }

private:
    struct TimerItem {
        TimerItem() {
            stoped  = false;
            id      = -1;
            timeout = 0;
        }

        bool    stoped;
        int64_t id;
        int64_t timeout;
        TimeoutCallback cb;
    };

private:
    int64_t m_timer_seqid;
    // map<timeout_ms, list<TimerItem> >
    cxx::unordered_map<uint32_t, std::list<cxx::shared_ptr<TimerItem> > > m_timers;
    // map<timer_seqid, TimerItem>
    cxx::unordered_map<int64_t, cxx::shared_ptr<TimerItem> > m_id_2_timer;
    char m_last_error[256];
};

}  // namespace pebble

#endif  // _PEBBLE_COMMON_TIMER_H_

