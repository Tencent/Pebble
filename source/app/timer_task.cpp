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


#include <iostream>
#include <map>
#include <vector>
#include <limits>
#include "source/app/timer_task.h"

namespace pebble {
CoroutineTimerTask::CoroutineTimerTask()
        : task_(NULL),
          interval_(0),
          last_time_(0),
          task_id_(-1) {
    // DO NOTHING
}

int CoroutineTimerTask::Init(TimerTaskInterface* task, int interval,
                             int task_id, uint64_t now) {
    if (task == NULL || interval <= 0) {
        return -1;
    }
    task_ = task;
    interval_ = interval * 1000;  // interval的单位是毫秒，内部数据是微秒
    task_id_ = task_id;
    last_time_ = now;
    return 0;
}

void CoroutineTimerTask::Run() {
    do {
        if (task_ == NULL)
            return;
        task_->OnTimer();
        this->Yield();
    } while (true);
}

void CoroutineTimerTask::Stop() {
    task_ = NULL;
    this->schedule_obj()->Resume(this->id());
}

const int TaskTimer::kDefaultTimerTaskNumber = 1024;
TaskTimer::TaskTimer()
        : schedule_(NULL),
          sys_time_(NULL),
          max_tasks_(kDefaultTimerTaskNumber),
          last_task_id_(0) {
// DO NOTHING
}
int TaskTimer::Init(CoroutineSchedule* schedule, uint64_t* sys_time, int max_task_num) {
    schedule_ = schedule;
    sys_time_ = sys_time;
    max_tasks_ = max_task_num;
    return 0;
}

TaskTimer::~TaskTimer() {
    // 清理task_
    std::map<int, CoroutineTimerTask*>::iterator tasks_it;
    for (tasks_it = tasks_.begin(); tasks_it != tasks_.end(); tasks_it++) {
        CoroutineTimerTask* co_task = tasks_it->second;
        co_task->Stop();  // 无需delete协程对象指针，结束协程后会自动删除
    }
}

int TaskTimer::CreateTaskId() {
    int ret = -1;
    ++last_task_id_;
    if (last_task_id_ >= std::numeric_limits<int>::max())
        last_task_id_ = 1;
    ret = last_task_id_;
    std::map<int, CoroutineTimerTask*>::iterator tasks_it = tasks_.find(ret);
    if (tasks_it == tasks_.end()) {
        return ret;
    } else {
        ret = CreateTaskId();
    }
    return ret;
}

int TaskTimer::AddTask(pebble::TimerTaskInterface* task, int interval) {
    size_t cur_task_num = tasks_.size();
    if (cur_task_num >= max_tasks_) {
        PLOG_ERROR("Too many running tasks, max number is set to %zu", max_tasks_);
        return -1;
    }

    int task_id = CreateTaskId();
    PreStartTask pre_task = { task, interval, task_id };
    pre_start_tasks_.push_back(pre_task);
    return task_id;
}

int TaskTimer::RemoveTask(int task_id) {
    std::map<int, CoroutineTimerTask*>::iterator tasks_it;
    tasks_it = tasks_.find(task_id);
    if (tasks_it == tasks_.end())
        return -1;

    wait_close_tasks_.push_back(task_id);

    return 0;
}

int TaskTimer::Update(PebbleServer* pebble_server) {
    if (schedule_ == NULL) {
        PLOG_ERROR("Schedule can not be NULL!");
        return -1;
    }

    bool is_idle = true;

    // 1. 处理pre_start_tasks_
    std::vector<PreStartTask>::iterator pre_start_tasks_it;
    for (pre_start_tasks_it = pre_start_tasks_.begin();
            pre_start_tasks_it != pre_start_tasks_.end();
            pre_start_tasks_it++) {
        is_idle = false;
        PreStartTask pre_task = *pre_start_tasks_it;
        CoroutineTimerTask* co_task = schedule_->NewTask<CoroutineTimerTask>();
        if (co_task == NULL) {
            PLOG_ERROR("New coroutine task error, got a NULL object!");
            continue;
        }

        if (co_task->Init(pre_task.task, pre_task.interval, pre_task.task_id,
                          *sys_time_)) {
            PLOG_ERROR("Can not init a timer by interval %d or null task, task id is %d",
                       pre_task.interval, pre_task.task_id);
            delete co_task;
            continue;
        }
        tasks_[pre_task.task_id] = co_task;
        co_task->Start(false);
    }
    if (!is_idle)
        pre_start_tasks_.clear();

    // 2. 停定时任务
    CloseTask();

    // 3. 处理tasks_
    std::map<int, CoroutineTimerTask*>::iterator tasks_it;
    for (tasks_it = tasks_.begin(); tasks_it != tasks_.end(); tasks_it++) {
        CoroutineTimerTask* co_task = tasks_it->second;
        if (co_task->IsTimeout(*sys_time_)) {
            is_idle = false;
            schedule_->Resume(co_task->id());
        }
    }
    return is_idle ? -1 : 0;
}

int TaskTimer::CloseTask() {
    std::map<int, CoroutineTimerTask*>::iterator tasks_it;
    CoroutineTimerTask* co_task = NULL;
    for (std::vector<int>::iterator it = wait_close_tasks_.begin();
            it != wait_close_tasks_.end(); ++it) {

        tasks_it = tasks_.find(*it);
        if (tasks_it == tasks_.end()) {
            continue;
        }

        co_task = tasks_it->second;
        if (co_task) {
            co_task->Stop();
        }

        tasks_.erase(tasks_it);
    }

    wait_close_tasks_.clear();

    return 0;
}

}  // namespace pebble

