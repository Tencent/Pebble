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


#ifndef _PEBBLE_COMMON_COROUTINE_H_
#define _PEBBLE_COMMON_COROUTINE_H_

#include <list>
#include <set>
#include <string.h>
#include <sys/poll.h>
#include <ucontext.h>

#include "common/error.h"
#include "common/platform.h"


namespace pebble {

/// @brief 协程模块错误码定义
typedef enum {
    kCO_ERROR_BASE              = COROUTINE_ERROR_CODE_BASE,
    kCO_INVALID_PARAM           = kCO_ERROR_BASE - 1, // 参数错误
    kCO_NOT_IN_COROUTINE        = kCO_ERROR_BASE - 2, // 不在协程中
    kCO_NOT_RUNNING             = kCO_ERROR_BASE - 3, // 协程为非运行状态
    kCO_START_TIMER_FAILED      = kCO_ERROR_BASE - 4, // 启动定时器错误
    kCO_TIMEOUT                 = kCO_ERROR_BASE - 5, // 协程超时
    kCO_CANNOT_RESUME_IN_COROUTINE = kCO_ERROR_BASE - 6, // 不支持在协程中resume其他协程
    kCO_COROUTINE_UNEXIST          = kCO_ERROR_BASE - 7, // 协程不存在
    kCO_COROUTINE_STATUS_ERROR     = kCO_ERROR_BASE - 8, // 协程状态错误
} CoroutineErrorCode;

class CoroutineErrorStringRegister {
public:
    static void RegisterErrorString() {
        SetErrorString(kCO_INVALID_PARAM, "invalid paramater");
        SetErrorString(kCO_NOT_IN_COROUTINE, "not in coroutine");
        SetErrorString(kCO_NOT_RUNNING, "coroutine not running");
        SetErrorString(kCO_START_TIMER_FAILED, "start timer failed");
        SetErrorString(kCO_TIMEOUT, "coroutine timeout");
        SetErrorString(kCO_CANNOT_RESUME_IN_COROUTINE, "cannot resume in coroutine");
        SetErrorString(kCO_COROUTINE_UNEXIST, "coroutine unexist");
        SetErrorString(kCO_COROUTINE_STATUS_ERROR, "coroute status error");
    }
};


#define COROUTINE_DEAD 0
#define COROUTINE_READY 1
#define COROUTINE_RUNNING 2
#define COROUTINE_SUSPEND 3

#define MAX_FREE_CO_NUM     1024
#define INVALID_CO_ID       -1

typedef void (*coroutine_func)(struct schedule *, void *ud);

struct coroutine {
    coroutine_func func;
    std::tr1::function<void()> std_func;
    void *ud;
    ucontext_t ctx;
    struct schedule * sch;
    int status;
    bool enable_hook;
    char* stack;                // 协程栈的内容
    int32_t result;             // 携带resume结果

    coroutine() {
        func = NULL;
        ud = NULL;
        sch = NULL;
        status = COROUTINE_DEAD;
        enable_hook = false;
        stack = NULL;
        result = 0;
        memset(&ctx, 0, sizeof(ucontext_t));
    }
};

/// @brief struct schedule 协程调度器的数据结构
struct schedule {
    ucontext_t main;
    int64_t nco;                // 下一个要创建的协程ID
    int64_t running;            // 当前正在运行的协程ID
    cxx::unordered_map<int64_t, coroutine*> co_hash_map;
    std::list<coroutine*> co_free_list;
    int32_t co_free_num;
    uint32_t stack_size;
};


/// @brief 协程库初始化函数
/// @param stack_size 协程的栈大小，默认是256k
/// @return 返回struct schedule* 类型的指针
/// @note 只能够在主线程调用
struct schedule * coroutine_open(uint32_t stack_size = 256 * 1024);

/// @brief 协程库关闭
/// @param 协程调度器结构体指针
/// @note 只能够在主线程调用
void coroutine_close(struct schedule *);

/// @brief 创建一个协程
/// @param 协程调度器结构体指针
/// @param 协程执行体的函数指针
/// @return 协程ID
/// @note 只能够在主线程调用
int64_t coroutine_new(struct schedule *, std::tr1::function<void()>& std_func);

/// @brief 创建一个协程
/// @param 协程调度器结构体指针
/// @param 协程执行体的函数指针
/// @param 该协程执行函数的参数
/// @return 协程ID
/// @note 只能够在主线程调用
int64_t coroutine_new(struct schedule *, coroutine_func, void *ud);

/// @brief 继续一个协程的运行
/// @param[in] 协程调度器结构体指针
/// @param[in] 协程ID
/// @param[in] resume时可传递结果，默认为0
/// @return 处理结果 @see CoroutineErrorCode
/// @note 只能够在主线程调用
int32_t coroutine_resume(struct schedule *, int64_t id, int32_t result = 0);

/// @brief 获取协程当前状态
/// @param 协程调度器结构体指针
/// @param 协程ID
/// @return 返回协程运行状态
int coroutine_status(struct schedule *, int64_t id);

/// @brief 获取当前正在运行的协程ID
/// @param 协程调度器结构体指针
/// @return 返回正在运行的协程ID
int64_t coroutine_running(struct schedule *);

/// @brief 暂停一个协程的运行
/// @param[in] 协程调度器结构体指针
/// @return 处理结果，@see CoroutineErrorCode
/// @note 只能够在协程内调用
int32_t coroutine_yield(struct schedule *);

coroutine* get_curr_coroutine(struct schedule *);
struct stCoEpoll_t;

coroutine* co_self();
int co_poll(stCoEpoll_t *ctx, struct pollfd fds[], nfds_t nfds, int timeout_ms);
void co_update();
stCoEpoll_t* co_get_epoll_ct();
void co_enable_hook_sys();
void co_disable_hook_sys();
bool co_is_enable_sys_hook();
void co_log_err(const char *fmt, ...);

class CoroutineSchedule;
class Timer;

/// @brief 类:CoroutineTask, 协程任务类
///
/// 与协程调度类CoroutineSchedule是友员\n
/// 用户使用协程时继承该类, 实现Run方法完成协程的使用
class CoroutineTask {
    friend class CoroutineSchedule;
public:
    /// @brief 构造函数
    CoroutineTask();

    /// @brief 析构函数
    virtual ~CoroutineTask();

    /// @brief 启动该协程任务, 执行Run方法
    /// @param is_immediately 是否立即执行
    /// @return 返回协程ID
    int64_t Start(bool is_immediately = true);

    /// @brief 协程任务的执行体, 要由使用者进行具体实现
    /// @note 在子类中实现该函数, 在函数体内可调用Yield
    virtual void Run() = 0;

    /// @brief 获得本协程任务的协程ID
    /// @return 协程ID
    inline int64_t id() {
        return id_;
    }

    /// @brief 挂起此协程
    /// @param timeout_ms 超时时间，单位为毫秒，默认-1，<0时表示不进行超时处理
    /// @return 处理结果，@see CoroutineErrorCode
    /// @note 此函数必须在协程中调用
    int32_t Yield(int32_t timeout_ms = -1);

    /// @brief 返回此协程的调度器对象
    /// @return 协程调度器对象
    CoroutineSchedule* schedule_obj();

private:
    int64_t id_;
    CoroutineSchedule* schedule_obj_;
};

/// @brief 基于function的通用的协程任务实现
class CommonCoroutineTask : public CoroutineTask {
public:
    CommonCoroutineTask() {}

    virtual ~CommonCoroutineTask() {}

    void Init(const cxx::function<void(void)>& run) { m_run = run; }

    virtual void Run() {
        m_run();
    }

private:
    cxx::function<void(void)> m_run;
};

/// @brief 类:CoroutineSchedule 协程调度类
///
/// 与协程任务类CoroutineTask是友员\n
/// 管理和调度协程, 是一个协程系统, 封装schedule, 管理多个协程
class CoroutineSchedule {
    friend class CoroutineTask;
public:
    /// @brief 构造函数
    /// @param logger 日志句柄
    explicit CoroutineSchedule();

    /// @brief 析构函数
    virtual ~CoroutineSchedule();

    /// @brief 初始化工作, new了一个新的schedule
    /// @param timer 定时器实例，使协程支持yield超时
    /// @param stack_size 协程的栈大小，默认是256k
    /// @return = 0 成功
    /// @return = -1 失败
    int Init(Timer* timer = NULL, uint32_t stack_size = 256 * 1024);

    /// @brief 关闭协程系统, 释放所有资源
    /// @return 还未结束的协程数
    int Close();

    /// @brief 返回当前协程数量
    /// @return 当前的协程数量
    int Size() const;

    /// @brief 获取当前正在运行的协程任务
    /// @return 正在运行的协程任务对象的指针
    /// @note 此函数必须在协程中调用
    CoroutineTask* CurrentTask() const;

    /// @brief 获取当前正在运行的协程ID
    /// @reurn 正在运行的协程ID
    /// @note 此函数必须在协程中调用
    int64_t CurrentTaskId() const;

    /// @brief 挂起当前协程
    /// @param timeout_ms 超时时间，单位为毫秒，默认-1，<=0时表示不进行超时处理
    /// @return 处理结果，@see CoroutineErrorCode
    /// @note 此函数必须在协程中调用
    int32_t Yield(int32_t timeout_ms = -1);

    /// @brief 激活指定ID的协程
    /// @param id 协程ID
    /// @param result resume时可传递结果，默认为0
    /// @return 处理结果，@see CoroutineErrorCode
    int32_t Resume(int64_t id, int32_t result = 0);

    /// @brief 返回指定id协程的状态
    /// @param id 协程ID
    /// @return 协程状态
    int Status(int64_t id);

    /// @brief 模版方法, 新建一个协程任务
    /// @note 使用此种方法生成的task对象指针会在协程结束后自动delete掉
    template<typename TASK>
    TASK* NewTask() {
        if (CurrentTaskId() != INVALID_CO_ID) {
            return NULL;
        }
        TASK* task = new TASK();
        if (AddTaskToSchedule(task)) {
            delete task;
            task = NULL;
        }
        return task;
    }

private:
    int AddTaskToSchedule(CoroutineTask* task);
    CoroutineTask* Find(int64_t id) const;
    int32_t OnTimeout(int64_t id);

    struct schedule* schedule_;
    Timer* timer_;
    cxx::unordered_map<int64_t, CoroutineTask*> task_map_;
    std::set<CoroutineTask*> pre_start_task_;
};

} // namespace pebble

#endif  // _PEBBLE_COMMON_COROUTINE_H_
