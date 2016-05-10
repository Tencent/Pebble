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


#include "./coroutine.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>
#include <map>
#include <set>
#include <cstdio>
#include <cstdlib>
#include <iostream>

class coroutine;

struct coroutine {
    coroutine_func func;
    void *ud;
    ucontext_t ctx;
    struct schedule * sch;
    ptrdiff_t cap;              // 协程栈的容量
    ptrdiff_t size;             // 协程栈的大小
    int status;
    char *stack;                // 协程栈的内容
};

struct coroutine *
_co_new(struct schedule *S, coroutine_func func, void *ud) {
    struct coroutine * co = new coroutine;
    co->func = func;
    co->ud = ud;
    co->sch = S;
    co->cap = 0;
    co->size = 0;
    co->status = COROUTINE_READY;
    co->stack = NULL;
    return co;
}

void _co_delete(struct coroutine *co) {
    delete[] co->stack;
    delete co;
}

struct schedule *
coroutine_open() {
    struct schedule *S = new schedule;
    S->nco = 0;
    S->running = -1;

    PLOG_INFO("coroutine_open is called.");
    return S;
}

void coroutine_close(struct schedule *S) {
    if (NULL == S) {
        return;
    }
    // 遍历所有的协程，逐个释放
    std::map<int64_t, coroutine*>::iterator pos = S->co_hash_map.begin();

    for (; pos != S->co_hash_map.end(); pos++) {
        if (pos->second) {
            _co_delete(pos->second);
        }
    }

    // 释放掉整个调度器
    delete S;
    S = NULL;
}

int64_t coroutine_new(struct schedule *S, coroutine_func func, void *ud) {
    if (NULL == S || NULL == func) {
        return -1;
    }
    struct coroutine *co = _co_new(S, func, ud);
    int64_t id = S->nco;
    S->co_hash_map[id] = co;
    S->nco++;

    PLOG_TRACE("coroutine %ld is created.", id);
    return id;
}

static void mainfunc(uint32_t low32, uint32_t hi32) {
    uintptr_t ptr = (uintptr_t) low32 | ((uintptr_t) hi32 << 32);
    struct schedule *S = (struct schedule *) ptr;
    int64_t id = S->running;
    struct coroutine *C = S->co_hash_map[id];
    C->func(S, C->ud);
    _co_delete(C);
    S->co_hash_map.erase(id);
    S->running = -1;
    PLOG_TRACE("coroutine %ld is deleted.", id);
}

void coroutine_resume(struct schedule * S, int64_t id) {
    if (NULL == S) {
        return;
    }
    assert(S->running == -1);
    std::map<int64_t, coroutine*>::iterator pos = S->co_hash_map.find(id);

    // 如果在协程哈希表中没有找到，或者找到的协程内容为空
    if (pos == S->co_hash_map.end()) {
        PLOG_FATAL("coroutine %ld can't find in co_map", id);
        return;
    }

    struct coroutine *C = pos->second;
    if (NULL == C) {
        PLOG_FATAL("coroutine %ld instance is NULL", id);
        return;
    }

    int status = C->status;
    switch (status) {
        case COROUTINE_READY: {
            PLOG_TRACE("coroutine %ld status is COROUTINE_READY, begin to execute...", id);

            getcontext(&C->ctx);
            C->ctx.uc_stack.ss_sp = S->stack;
            C->ctx.uc_stack.ss_size = STACK_SIZE;
            C->ctx.uc_link = &S->main;
            S->running = id;
            C->status = COROUTINE_RUNNING;
            uintptr_t ptr = (uintptr_t) S;
            makecontext(&C->ctx, (void (*)(void)) mainfunc, 2,
            (uint32_t)ptr,  // NOLINT
            (uint32_t)(ptr>>32));  // NOLINT
            swapcontext(&S->main, &C->ctx);
        }
            break;
        case COROUTINE_SUSPEND: {
            PLOG_TRACE("coroutine %ld status is COROUTINE_SUSPEND,"
                    " stack size is %td, begin to resume...",
                    id, C->size);

            // 备份协程的堆栈
            memcpy(S->stack + STACK_SIZE - C->size, C->stack, C->size);
            S->running = id;
            C->status = COROUTINE_RUNNING;
            swapcontext(&S->main, &C->ctx);
        }
            break;
        default:
            PLOG_DEBUG("coroutine %ld status is failed, can not to be resume...", id);
            return;
    }
}

static void _save_stack(struct coroutine *C, char *top) {
    char dummy = 0;
    assert(top - &dummy <= STACK_SIZE);
    if (C->cap < top - &dummy) {
        delete[] C->stack;
        C->cap = top - &dummy;
        C->stack = new char[C->cap];
    }
    C->size = top - &dummy;

    memcpy(C->stack, &dummy, C->size);
}

void coroutine_yield(struct schedule * S) {
    if (NULL == S) {
        return;
    }

    int64_t id = S->running;
    if (id < 0) {
        PLOG_ERROR("have no running coroutine, can't yield.");
        return;
    }

    assert(id >= 0);
    struct coroutine * C = S->co_hash_map[id];
    assert(reinterpret_cast<char*>(&C) > S->stack);

    if (C->status != COROUTINE_RUNNING) {
        PLOG_ERROR("coroutine %ld status is SUSPEND, can't yield again.", id);
        return;
    }

    // 备份调度器的堆栈
    _save_stack(C, S->stack + STACK_SIZE);

    C->status = COROUTINE_SUSPEND;
    S->running = -1;

    PLOG_TRACE("coroutine %ld will be yield, swith to main loop...", id);
    swapcontext(&C->ctx, &S->main);
}

int coroutine_status(struct schedule * S, int64_t id) {
    // assert(id >= 0 && id <= S->nco);
    if (NULL == S) {
        return COROUTINE_DEAD;
    }

    if (id < 0 || id > S->nco) {
        PLOG_DEBUG("coroutine %ld not exist", id);
        return COROUTINE_DEAD;
    }

    std::map<int64_t, coroutine*>::iterator pos = S->co_hash_map.find(id);

    // 如果在协程哈希表中没有找到，或者找到的协程内容为空
    if (pos == S->co_hash_map.end()) {
        PLOG_DEBUG("cann't find coroutine %ld", id);
        return COROUTINE_DEAD;
    }

    return (pos->second)->status;
}

int64_t coroutine_running(struct schedule * S) {
    if (NULL == S) {
        return -1;
    }

    return S->running;
}

void DoTask(struct schedule*, void *ud) {
    CoroutineTask* task = static_cast<CoroutineTask*>(ud);
    assert(task != NULL);
    task->Run();
    delete task;
}

CoroutineTask::CoroutineTask()
        : id_(-1),
          schedule_obj_(NULL) {
    // DO NOTHING
}

CoroutineTask::~CoroutineTask() {
    if (schedule_obj_ == NULL)
        return;

    // 如果schedule_obj_没进入Close()流程
    if (schedule_obj_->schedule_ != NULL) {
        if (id_ == -1) {
            schedule_obj_->pre_start_task_.erase(this);
        } else {
            // 防止schedule_在清理时重复delete自己
            schedule_obj_->task_map_.erase(id_);
        }
    }
}

int64_t CoroutineTask::Start(bool is_immediately) {
    id_ = coroutine_new(schedule_obj_->schedule_, DoTask, this);
    if (id_ < 0)
        id_ = -1;
    schedule_obj_->task_map_[id_] = this;
    schedule_obj_->pre_start_task_.erase(this);
    if (is_immediately)
        coroutine_resume(schedule_obj_->schedule_, id_);
    return id_;
}

CoroutineSchedule::CoroutineSchedule()
        : schedule_(NULL),
          task_map_(),
          pre_start_task_() {
    // DO NOTHING
}

CoroutineSchedule::~CoroutineSchedule() {
    if (schedule_ != NULL)
        Close();
}

int CoroutineSchedule::Init() {
    schedule_ = coroutine_open();
    if (schedule_ == NULL)
        return -1;
    return 0;
}

int CoroutineSchedule::Close() {
    int ret = 0;
    if (schedule_ != NULL) {
        coroutine_close(schedule_);
        schedule_ = NULL;
    }

    ret += pre_start_task_.size();
    std::set<CoroutineTask*>::iterator pre_it;
    for (pre_it = pre_start_task_.begin(); pre_it != pre_start_task_.end();
            ++pre_it) {
        delete *pre_it;
    }

    ret += task_map_.size();
    std::map<int64_t, CoroutineTask*>::iterator it;
    for (it = task_map_.begin(); it != task_map_.end();) {
        delete it->second;

        // 为了安全的删除迭代器指向的对象，所以先++后调用
        task_map_.erase(it++);
    }

    return ret;
}

int CoroutineSchedule::Size() const {
    int ret = 0;
    ret += task_map_.size();
    ret += pre_start_task_.size();
    return ret;
}

CoroutineTask* CoroutineSchedule::CurrentTask() const {
    int64_t id = coroutine_running(schedule_);
    return Find(id);
}

CoroutineTask* CoroutineSchedule::Find(int64_t id) const {
    CoroutineTask* ret = NULL;
    std::map<int64_t, CoroutineTask*>::const_iterator it = task_map_.find(id);
    if (it != task_map_.end())
        ret = it->second;
    return ret;
}

int64_t CoroutineSchedule::CurrentTaskId() const {
    return coroutine_running(schedule_);
}

int CoroutineSchedule::AddTaskToSchedule(CoroutineTask* task) {
    task->schedule_obj_ = this;
    pre_start_task_.insert(task);
    return 0;
}

void CoroutineSchedule::Yield() {
    coroutine_yield(this->schedule_);
}

void CoroutineSchedule::Resume(int64_t id) {
    coroutine_resume(this->schedule_, id);
}

int CoroutineSchedule::Status(int64_t id) {
    return coroutine_status(this->schedule_, id);
}

void CoroutineTask::Yield() {
    schedule_obj_->Yield();
}

CoroutineSchedule* CoroutineTask::schedule_obj() {
    return schedule_obj_;
}
