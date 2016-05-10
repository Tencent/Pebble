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


#ifndef SOURCE_APP_TIMER_TASK_H_
#define SOURCE_APP_TIMER_TASK_H_

#include <iostream>
#include <map>
#include <vector>
#include "source/app/pebble_server.h"
#include "source/common/coroutine.h"

class CoroutineSchedule;
class CoroutineTask;

namespace pebble {
class TimerTaskInterface;

class CoroutineTimerTask : public CoroutineTask {
    friend class TaskTimer;
public:
    CoroutineTimerTask();

    int Init(TimerTaskInterface* task, int interval, int task_id, uint64_t now);
    virtual void Run();
    void Stop();

    bool IsTimeout(uint64_t now) {
        uint64_t this_interval = now - last_time_;
        if (this_interval > interval_) {
            last_time_ = now;
            return true;
        }
        return false;
    }

    inline int task_id() {
        return task_id_;
    }

private:
    TimerTaskInterface* task_;
    uint64_t interval_;
    uint64_t last_time_;
    int task_id_;
};

struct PreStartTask {
    pebble::TimerTaskInterface* task;
    int interval;
    int task_id;
};

class TaskTimer : public UpdaterInterface {
public:
    static const int kDefaultTimerTaskNumber;

    TaskTimer();
    int Init(CoroutineSchedule* schedule, uint64_t* sys_time,
             int max_task_num = kDefaultTimerTaskNumber);
    virtual ~TaskTimer();
    virtual int Update(PebbleServer *pebble_server);
    int AddTask(pebble::TimerTaskInterface* task, int interval);
    int RemoveTask(int task_id);
private:
    int CreateTaskId();  // 找一个数字作为task_id
    int CloseTask();

    std::map<int, CoroutineTimerTask*> tasks_;  // 正在运行的定时任务
    std::vector<PreStartTask> pre_start_tasks_;  // 准备启动的任务
    std::vector<int> wait_close_tasks_;  // 等待关闭的任务
    CoroutineSchedule* schedule_;  // 协程管理器对象
    uint64_t* sys_time_;  // 从此指针读取系统时间，单位微秒
    size_t max_tasks_;  // 最大任务数限制
    int last_task_id_;  // 最大任务ID，生成ID用
};

}  // namespace pebble

#endif  // SOURCE_APP_TIMER_TASK_H_
