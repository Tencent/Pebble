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


#ifndef SOURCE_COMMON_COROUTINE_H_
#define SOURCE_COMMON_COROUTINE_H_

#include <string.h>
#include <stdint.h>
#include <ucontext.h>
#include <map>
#include <set>
#include "source/log/log.h"

#define COROUTINE_DEAD 0
#define COROUTINE_READY 1
#define COROUTINE_RUNNING 2
#define COROUTINE_SUSPEND 3

#define STACK_SIZE (20 * 1024 * 1024)

class coroutine;

/// @brief struct schedule 协程调度器的数据结构
struct schedule {
    char stack[STACK_SIZE];
    ucontext_t main;
    int64_t nco;                // 下一个要创建的协程ID
    int64_t running;            // 当前正在运行的协程ID
    std::map<int64_t, coroutine*> co_hash_map;
};

typedef void (*coroutine_func)(struct schedule *, void *ud);

/// @brief 协程库初始化函数
/// @param logger 日志句柄
/// @return 返回struct schedule* 类型的指针
/// @note 只能够在主线程调用
struct schedule * coroutine_open();

/// @brief 协程库关闭
/// @param 协程调度器结构体指针
/// @note 只能够在主线程调用
void coroutine_close(struct schedule *);

/// @brief 创建一个协程
/// @param 协程调度器结构体指针
/// @param 协程执行体的函数指针
/// @param 该协程对应的CoroutineTask对象的指针
/// @return 协程ID
/// @note 只能够在主线程调用
int64_t coroutine_new(struct schedule *, coroutine_func, void *ud);

/// @brief 继续一个协程的运行
/// @param[in] 协程调度器结构体指针
/// @param[in] 协程ID
/// @note 只能够在主线程调用
void coroutine_resume(struct schedule *, int64_t id);

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
/// @note 只能够在协程内调用
void coroutine_yield(struct schedule *);

class CoroutineSchedule;

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
    /// @note 此函数必须在协程中调用
    void Yield();

    /// @brief 返回此协程的调度器对象
    /// @return 协程调度器对象
    CoroutineSchedule* schedule_obj();

private:
    int64_t id_;
    CoroutineSchedule* schedule_obj_;
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
    /// @param logger 日志句柄
    /// @return = 0 成功
    /// @return = -1 失败
    int Init();

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
    /// @note 此函数必须在协程中调用
    void Yield();

    /// @brief 激活指定ID的协程
    /// @param id 协程ID
    void Resume(int64_t id);

    /// @brief 返回指定id协程的状态
    /// @param id 协程ID
    /// @return 协程状态
    int Status(int64_t id);

    /// @brief 模版方法, 新建一个协程任务
    /// @note 使用此种方法生成的task对象指针会在协程结束后自动delete掉
    template<typename TASK>
    TASK* NewTask() {
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

    struct schedule* schedule_;
    std::map<int64_t, CoroutineTask*> task_map_;
    std::set<CoroutineTask*> pre_start_task_;
};

#endif  // SOURCE_COMMON_COROUTINE_H_
