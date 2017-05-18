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


#include <assert.h>
#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <iostream>
#include <set>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/syscall.h>
#include <sys/time.h>


#include "common/coroutine.h"
#include "common/log.h"
#include "common/timer.h"


namespace pebble {


struct stCoEpoll_t;
struct stCoRoutineEnv_t {
    schedule* co_schedule;
    stCoEpoll_t *pEpoll;
};

struct co_epoll_res {
    int size;
    struct epoll_event *events;
    struct kevent *eventlist;
};

struct co_epoll_res *co_epoll_res_alloc(int n) {
    struct co_epoll_res * ptr =
        (struct co_epoll_res *)malloc(sizeof(struct co_epoll_res));

    ptr->size = n;
    ptr->events = (struct epoll_event*)calloc(1, n * sizeof(struct epoll_event));

    return ptr;
}

void co_epoll_res_free(struct co_epoll_res * ptr) {
    if (!ptr) return;
    if (ptr->events) free(ptr->events);
    free(ptr);
}

struct stTimeoutItemLink_t;
struct stTimeoutItem_t;
struct stCoEpoll_t {
    int iEpollFd;
    static const int _EPOLL_SIZE = 1024 * 10;

    struct stTimeout_t *pTimeout;

    struct stTimeoutItemLink_t *pstTimeoutList;

    struct stTimeoutItemLink_t *pstActiveList;

    co_epoll_res *result;
};
typedef void (*OnPreparePfn_t)(stTimeoutItem_t *, const struct epoll_event &ev,
        stTimeoutItemLink_t *active);

typedef void (*OnProcessPfn_t)(stTimeoutItem_t * t);
struct stTimeoutItem_t {

    enum
    {
        eMaxTimeout = 20 * 1000 // 20s
    };
    stTimeoutItem_t *pPrev;
    stTimeoutItem_t *pNext;
    stTimeoutItemLink_t *pLink;

    unsigned long long ullExpireTime;

    OnPreparePfn_t pfnPrepare;
    OnProcessPfn_t pfnProcess;

    int64_t co_id;
    bool bTimeout;
};
struct stTimeoutItemLink_t {
    stTimeoutItem_t *head;
    stTimeoutItem_t *tail;
};
struct stTimeout_t {
    stTimeoutItemLink_t *pItems;
    int iItemSize;

    unsigned long long ullStart;
    long long llStartIdx;
};

static unsigned long long GetTickMS();
stTimeout_t *AllocTimeout(int iSize) {
    stTimeout_t *lp = reinterpret_cast<stTimeout_t*>(calloc(1, sizeof(stTimeout_t)));
    lp->iItemSize = iSize;
    lp->pItems = reinterpret_cast<stTimeoutItemLink_t*>
        (calloc(1, sizeof(stTimeoutItemLink_t) * lp->iItemSize));
    lp->ullStart = GetTickMS();
    lp->llStartIdx = 0;

    return lp;
}

void FreeTimeout(stTimeout_t *apTimeout) {
    free(apTimeout->pItems);
    free(apTimeout);
}

stCoEpoll_t *AllocEpoll() {
    stCoEpoll_t *ctx = reinterpret_cast<stCoEpoll_t*>(calloc(1, sizeof(stCoEpoll_t)));

    ctx->iEpollFd = epoll_create(stCoEpoll_t::_EPOLL_SIZE);
    ctx->pTimeout = AllocTimeout(60 * 1000);

    ctx->pstActiveList = reinterpret_cast<stTimeoutItemLink_t*>
        (calloc(1, sizeof(stTimeoutItemLink_t)));
    ctx->pstTimeoutList = reinterpret_cast<stTimeoutItemLink_t*>
        (calloc(1, sizeof(stTimeoutItemLink_t)));

    return ctx;
}

void SetEpoll(stCoRoutineEnv_t *env, stCoEpoll_t *ev) {
    env->pEpoll = ev;
}

void FreeEpoll(stCoEpoll_t *ctx) {
    if (ctx) {
        free(ctx->pstActiveList);
        free(ctx->pstTimeoutList);
        FreeTimeout(ctx->pTimeout);
        co_epoll_res_free(ctx->result);
    }
    free(ctx);
}


static stCoRoutineEnv_t* g_arrCoEnvPerThread[102400] = {0};
static pid_t GetPid()
{
    static __thread pid_t pid = 0;
    static __thread pid_t tid = 0;
    if (!pid || !tid || pid != getpid()) {
        pid = getpid();
        tid = syscall(__NR_gettid);
    }
    return tid;
}

struct coroutine *
_co_new(struct schedule *S, std::tr1::function<void()>& std_func) {
    if (NULL == S) {
        assert(0);
        return NULL;
    }

    struct coroutine * co = NULL;
    if (S->co_free_list.empty()) {
        co = new coroutine;
        co->stack = new char[S->stack_size];
    } else {
        co = S->co_free_list.front();
        S->co_free_list.pop_front();

        S->co_free_num--;
    }

    co->std_func = std_func;
    co->func = NULL;
    co->ud = NULL;
    co->sch = S;
    co->status = COROUTINE_READY;

    return co;
}

struct coroutine *
_co_new(struct schedule *S, coroutine_func func, void *ud) {
    if (NULL == S) {
        assert(0);
        return NULL;
    }

    struct coroutine * co = NULL;

    if (S->co_free_list.empty()) {
        co = new coroutine;
        co->stack = new char[S->stack_size];
    } else {
        co = S->co_free_list.front();
        S->co_free_list.pop_front();

        S->co_free_num--;
    }
    co->func = func;
    co->ud = ud;
    co->sch = S;
    co->status = COROUTINE_READY;

    return co;
}

void _co_delete(struct coroutine *co) {
    delete [] co->stack;
    delete co;
}

struct schedule *
coroutine_open(uint32_t stack_size) {
    if (0 == stack_size) {
        stack_size = 256 * 1024;
    }
    pid_t pid = GetPid();
    stCoRoutineEnv_t *env = g_arrCoEnvPerThread[pid];
    if (env) {
        return env->co_schedule;
    }

    void* p = calloc(1, sizeof(stCoRoutineEnv_t));
    g_arrCoEnvPerThread[pid] = reinterpret_cast<stCoRoutineEnv_t*>(p);

    env = g_arrCoEnvPerThread[pid];
    PLOG_INFO("init pid %ld env %p\n", (long)pid, env);

    struct schedule *S = new schedule;
    S->nco = 0;
    S->running = -1;
    S->co_free_num = 0;
    S->stack_size = stack_size;

    env->co_schedule = S;

    stCoEpoll_t *ev = AllocEpoll();
    SetEpoll(env, ev);

    PLOG_INFO("coroutine_open is called.");
    return S;
}

void coroutine_close(struct schedule *S) {

    if (NULL == S) {
        return;
    }

    pid_t pid = GetPid();
    stCoRoutineEnv_t *env = g_arrCoEnvPerThread[pid];
    if (env == NULL) {
        return;
    }

    FreeEpoll(env->pEpoll);

    // 遍历所有的协程，逐个释放
    cxx::unordered_map<int64_t, coroutine*>::iterator pos = S->co_hash_map.begin();
    for (; pos != S->co_hash_map.end(); pos++) {
        if (pos->second) {
            _co_delete(pos->second);
        }
    }

    std::list<coroutine*>::iterator p = S->co_free_list.begin();
    for (; p != S->co_free_list.end(); p++) {
        _co_delete(*p);
    }

    // 释放掉整个调度器
    delete S;
    S = NULL;

    free(env);
    env = NULL;

    g_arrCoEnvPerThread[pid] = 0;
}
int64_t coroutine_new(struct schedule *S, std::tr1::function<void()>& std_func) {
    if (NULL == S) {
        return -1;
    }
    struct coroutine *co = _co_new(S, std_func);
    int64_t id = S->nco;
    S->co_hash_map[id] = co;
    S->nco++;

    PLOG_TRACE("coroutine %ld is created.", id);
    return id;
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
    if (C->func != NULL) {
        C->func(S, C->ud);
    } else {
        C->std_func();
    }
    S->co_free_list.push_back(C);
    S->co_free_num++;

    if (S->co_free_num > MAX_FREE_CO_NUM) {
        coroutine* co = S->co_free_list.front();
        _co_delete(co);

        S->co_free_list.pop_front();
        S->co_free_num--;
    }

    S->co_hash_map.erase(id);
    S->running = -1;
    PLOG_TRACE("coroutine %ld is deleted.", id);
}

int32_t coroutine_resume(struct schedule * S, int64_t id, int32_t result) {
    if (NULL == S) {
        return kCO_INVALID_PARAM;
    }
    if (S->running != -1) {
        return kCO_CANNOT_RESUME_IN_COROUTINE;
    }
    cxx::unordered_map<int64_t, coroutine*>::iterator pos = S->co_hash_map.find(id);

    // 如果在协程哈希表中没有找到，或者找到的协程内容为空
    if (pos == S->co_hash_map.end()) {
        PLOG_ERROR("coroutine %ld can't find in co_map", id);
        return kCO_COROUTINE_UNEXIST;
    }

    struct coroutine *C = pos->second;
    if (NULL == C) {
        PLOG_ERROR("coroutine %ld instance is NULL", id);
        return kCO_COROUTINE_UNEXIST;
    }

    C->result = result;
    int status = C->status;
    switch (status) {
        case COROUTINE_READY: {
            PLOG_TRACE("coroutine %ld status is COROUTINE_READY, begin to execute...", id);

            getcontext(&C->ctx);
            C->ctx.uc_stack.ss_sp = C->stack;
            C->ctx.uc_stack.ss_size = S->stack_size;
            C->ctx.uc_stack.ss_flags = 0;
            C->ctx.uc_link = &S->main;
            S->running = id;
            C->status = COROUTINE_RUNNING;
            uintptr_t ptr = (uintptr_t) S;
            makecontext(&C->ctx, (void (*)(void)) mainfunc, 2,
            (uint32_t)ptr,  // NOLINT
            (uint32_t)(ptr>>32));  // NOLINT

            swapcontext(&S->main, &C->ctx);

            break;
        }
        case COROUTINE_SUSPEND: {
            PLOG_TRACE("coroutine %ld status is COROUTINE_SUSPEND,"
                    "begin to resume...", id);

            S->running = id;
            C->status = COROUTINE_RUNNING;
            swapcontext(&S->main, &C->ctx);

            break;
        }

        default:
            PLOG_DEBUG("coroutine %ld status is failed, can not to be resume...", id);
            return kCO_COROUTINE_STATUS_ERROR;
    }

    return 0;
}

int32_t coroutine_yield(struct schedule * S) {
    if (NULL == S) {
        return kCO_INVALID_PARAM;
    }

    int64_t id = S->running;
    if (id < 0) {
        PLOG_ERROR("have no running coroutine, can't yield.");
        return kCO_NOT_IN_COROUTINE;
    }

    assert(id >= 0);
    struct coroutine * C = S->co_hash_map[id];

    if (C->status != COROUTINE_RUNNING) {
        PLOG_ERROR("coroutine %ld status is SUSPEND, can't yield again.", id);
        return kCO_NOT_RUNNING;
    }

    C->status = COROUTINE_SUSPEND;
    S->running = -1;

    PLOG_TRACE("coroutine %ld will be yield, swith to main loop...", id);
    swapcontext(&C->ctx, &S->main);

    return C->result;
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

    cxx::unordered_map<int64_t, coroutine*>::iterator pos = S->co_hash_map.find(id);

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

coroutine* get_curr_coroutine(struct schedule * S) {
    if (NULL == S) {
        return NULL;
    }

    cxx::unordered_map<int64_t, coroutine*>::iterator pos = S->co_hash_map.find(S->running);

    // 如果在协程哈希表中没有找到，或者找到的协程内容为空
    if (pos == S->co_hash_map.end()) {
        PLOG_FATAL("coroutine %ld can't find in co_map", S->running);
        return NULL;
    }

    return pos->second;
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
    if (is_immediately && schedule_obj_->CurrentTaskId() != INVALID_CO_ID) {
        delete this;
        return -1;
    }
    id_ = coroutine_new(schedule_obj_->schedule_, DoTask, this);
    if (id_ < 0)
        id_ = -1;
    int64_t id = id_;
    schedule_obj_->task_map_[id_] = this;
    schedule_obj_->pre_start_task_.erase(this);
    if (is_immediately) {
        int32_t ret = coroutine_resume(schedule_obj_->schedule_, id_);
        if (ret != 0) {
            id = -1;
        }
    }
    return id;
}

CoroutineSchedule::CoroutineSchedule()
        : schedule_(NULL),
          timer_(NULL),
          task_map_(),
          pre_start_task_() {
    // DO NOTHING
}

CoroutineSchedule::~CoroutineSchedule() {
    if (schedule_ != NULL)
        Close();
}

int CoroutineSchedule::Init(Timer* timer, uint32_t stack_size) {
    timer_ = timer;
    schedule_ = coroutine_open(stack_size);
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

    timer_ = NULL;

    ret += pre_start_task_.size();
    std::set<CoroutineTask*>::iterator pre_it;
    for (pre_it = pre_start_task_.begin(); pre_it != pre_start_task_.end();
            ++pre_it) {
        delete *pre_it;
    }

    ret += task_map_.size();
    cxx::unordered_map<int64_t, CoroutineTask*>::iterator it;
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
    cxx::unordered_map<int64_t, CoroutineTask*>::const_iterator it = task_map_.find(id);
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

int32_t CoroutineSchedule::Yield(int32_t timeout_ms) {
    int64_t timerid = -1;
    int64_t co_id   = INVALID_CO_ID;
    if (timer_ && timeout_ms > 0) {
        co_id = CurrentTaskId();
        if (INVALID_CO_ID == co_id) {
            return kCO_NOT_IN_COROUTINE;
        }
        timerid = timer_->StartTimer(timeout_ms,
            cxx::bind(&CoroutineSchedule::OnTimeout, this, co_id));
        if (timerid < 0) {
            return kCO_START_TIMER_FAILED;
        }
    }

    int32_t ret = coroutine_yield(this->schedule_);

    if (timerid >= 0) {
        timer_->StopTimer(timerid);
    }

    return ret;
}

int32_t CoroutineSchedule::Resume(int64_t id, int32_t result) {
    return coroutine_resume(this->schedule_, id, result);
}

int CoroutineSchedule::Status(int64_t id) {
    return coroutine_status(this->schedule_, id);
}

int32_t CoroutineSchedule::OnTimeout(int64_t id) {
    Resume(id, kCO_TIMEOUT);
    return kTIMER_BE_REMOVED;
}

int32_t CoroutineTask::Yield(int32_t timeout_ms) {
    return schedule_obj_->Yield(timeout_ms);
}

CoroutineSchedule* CoroutineTask::schedule_obj() {
    return schedule_obj_;
}


void co_log_err(const char *fmt, ...)
{
}

static unsigned long long counter(void);
static unsigned long long getCpuKhz() {
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (!fp) return 1;
    char buf[4096] = {0};
    fread(buf, 1, sizeof(buf), fp);
    fclose(fp);

    char *lp = strstr(buf, "cpu MHz");
    if (!lp) return 1;
    lp += strlen("cpu MHz");
    while (*lp == ' ' || *lp == '\t' || *lp == ':') {
        ++lp;
    }

    double mhz = atof(lp);
    unsigned long long u = (unsigned long long)(mhz * 1000);
    return u;
}

static unsigned long long GetTickMS() {
    static uint32_t khz = getCpuKhz();
    return counter() / khz;
}

static unsigned long long counter(void)
{
    register uint32_t lo, hi;
    register unsigned long long o;
    __asm__ __volatile__ (
        "rdtsc" : "=a"(lo), "=d"(hi)
       );
    o = hi;
    o <<= 32;
    return (o | lo);
}
/*
static pid_t GetPid()
{
    char **p = (char**)pthread_self();
    return p ? *(pid_t*)(p + 18) : getpid();
}
*/
template <class T, class TLink>
void RemoveFromLink(T *ap)
{
    TLink *lst = ap->pLink;
    if (!lst) return;
    assert(lst->head && lst->tail);

    if (ap == lst->head) {
        lst->head = ap->pNext;
        if (lst->head) {
            lst->head->pPrev = NULL;
        }
    } else {
        if (ap->pPrev) {
            ap->pPrev->pNext = ap->pNext;
        }
    }

    if (ap == lst->tail) {
        lst->tail = ap->pPrev;
        if (lst->tail) {
            lst->tail->pNext = NULL;
        }
    } else {
        ap->pNext->pPrev = ap->pPrev;
    }

    ap->pPrev = ap->pNext = NULL;
    ap->pLink = NULL;
}

template <class TNode, class TLink>
void inline AddTail(TLink*apLink, TNode *ap)
{
    if (ap->pLink) {
        return;
    }
    if (apLink->tail) {
        apLink->tail->pNext = reinterpret_cast<TNode*>(ap);
        ap->pNext = NULL;
        ap->pPrev = apLink->tail;
        apLink->tail = ap;
    } else {
        apLink->head = apLink->tail = ap;
        ap->pNext = ap->pPrev = NULL;
    }
    ap->pLink = apLink;
}
template <class TNode, class TLink>
void inline PopHead(TLink*apLink)
{
    if (!apLink->head) {
        return;
    }
    TNode *lp = apLink->head;
    if (apLink->head == apLink->tail) {
        apLink->head = apLink->tail = NULL;
    } else {
        apLink->head = apLink->head->pNext;
    }

    lp->pPrev = lp->pNext = NULL;
    lp->pLink = NULL;

    if (apLink->head) {
        apLink->head->pPrev = NULL;
    }
}

template <class TNode, class TLink>
void inline Join(TLink*apLink, TLink *apOther)
{
    if (!apOther->head) {
        return;
    }
    TNode *lp = apOther->head;
    while (lp) {
        lp->pLink = apLink;
        lp = lp->pNext;
    }
    lp = apOther->head;
    if (apLink->tail) {
        apLink->tail->pNext = reinterpret_cast<TNode*>(lp);
        lp->pPrev = apLink->tail;
        apLink->tail = apOther->tail;
    } else {
        apLink->head = apOther->head;
        apLink->tail = apOther->tail;
    }

    apOther->head = apOther->tail = NULL;
}


int AddTimeout(stTimeout_t *apTimeout, stTimeoutItem_t *apItem, unsigned long long allNow)
{
    if (apTimeout->ullStart == 0) {
        apTimeout->ullStart = allNow;
        apTimeout->llStartIdx = 0;
    }
    if (allNow < apTimeout->ullStart) {
        co_log_err("CO_ERR: AddTimeout line %d allNow %llu apTimeout->ullStart %llu",
                    __LINE__, allNow, apTimeout->ullStart);

        return __LINE__;
    }
    if (apItem->ullExpireTime < allNow) {
        co_log_err("CO_ERR: AddTimeout line %d apItem->ullExpireTime %llu "
                "allNow %llu apTimeout->ullStart %llu",
                    __LINE__, apItem->ullExpireTime, allNow, apTimeout->ullStart);

        return __LINE__;
    }
    int diff = apItem->ullExpireTime - apTimeout->ullStart;

    if (diff >= apTimeout->iItemSize) {
        co_log_err("CO_ERR: AddTimeout line %d diff %d",
                    __LINE__, diff);

        return __LINE__;
    }
    AddTail(apTimeout->pItems + (apTimeout->llStartIdx + diff) % apTimeout->iItemSize , apItem);

    return 0;
}
inline void TakeAllTimeout(stTimeout_t *apTimeout,
        unsigned long long allNow,
        stTimeoutItemLink_t *apResult) {
    if (apTimeout->ullStart == 0) {
        apTimeout->ullStart = allNow;
        apTimeout->llStartIdx = 0;
    }

    if (allNow < apTimeout->ullStart) {
        return;
    }

    int cnt = allNow - apTimeout->ullStart + 1;
    if (cnt > apTimeout->iItemSize) {
        cnt = apTimeout->iItemSize;
    }

    if (cnt < 0) {
        return;
    }

    for (int i = 0; i < cnt; i++) {
        int idx = (apTimeout->llStartIdx + i) % apTimeout->iItemSize;
        Join<stTimeoutItem_t, stTimeoutItemLink_t>(apResult, apTimeout->pItems + idx);
    }
    apTimeout->ullStart = allNow;
    apTimeout->llStartIdx += cnt - 1;
}


struct stPollItem_t;
struct stPoll_t : public stTimeoutItem_t {
    struct pollfd *fds;
    nfds_t nfds;

    stPollItem_t *pPollItems;

    int iAllEventDetach;

    int iEpollFd;

    int iRaiseCnt;
};
struct stPollItem_t : public stTimeoutItem_t {
    struct pollfd *pSelf;
    stPoll_t *pPoll;

    struct epoll_event stEvent;
};
/*
 *   EPOLLPRI         POLLPRI    // There is urgent data to read.
 *   EPOLLMSG         POLLMSG
 *
 *                   POLLREMOVE
 *                   POLLRDHUP
 *                   POLLNVAL
 *
 * */
static uint32_t PollEvent2Epoll(short events) {
    uint32_t e = 0;
    if (events & POLLIN)   e |= EPOLLIN;
    if (events & POLLOUT)  e |= EPOLLOUT;
    if (events & POLLHUP)  e |= EPOLLHUP;
    if (events & POLLERR)  e |= EPOLLERR;
    return e;
}
static short EpollEvent2Poll(uint32_t events) {
    short e = 0;
    if (events & EPOLLIN)  e |= POLLIN;
    if (events & EPOLLOUT) e |= POLLOUT;
    if (events & EPOLLHUP) e |= POLLHUP;
    if (events & EPOLLERR) e |= POLLERR;
    return e;
}

stCoRoutineEnv_t *co_get_curr_thread_env() {
    return g_arrCoEnvPerThread[GetPid()];
}

void OnPollProcessEvent(stTimeoutItem_t * ap)
{
    coroutine_resume(co_get_curr_thread_env()->co_schedule, ap->co_id);
}

void OnPollPreparePfn(stTimeoutItem_t * ap,
                const struct epoll_event &e,
                stTimeoutItemLink_t *active)
{
    stPollItem_t *lp = reinterpret_cast<stPollItem_t *>(ap);
    lp->pSelf->revents = EpollEvent2Poll(e.events);


    stPoll_t *pPoll = lp->pPoll;
    pPoll->iRaiseCnt++;

    if (!pPoll->iAllEventDetach) {
        pPoll->iAllEventDetach = 1;
        RemoveFromLink<stTimeoutItem_t, stTimeoutItemLink_t>(pPoll);
        AddTail(active, pPoll);
    }
}

void co_update()
{
    stCoEpoll_t* ctx = co_get_epoll_ct();
    if (!ctx) {
        return;
    }
    if (!ctx->result) {
        ctx->result = co_epoll_res_alloc(stCoEpoll_t::_EPOLL_SIZE);
    }
    co_epoll_res *result = ctx->result;
    int ret = epoll_wait(ctx->iEpollFd, result->events, stCoEpoll_t::_EPOLL_SIZE, 1);

    stTimeoutItemLink_t *active = (ctx->pstActiveList);
    stTimeoutItemLink_t *timeout = (ctx->pstTimeoutList);

    memset(active, 0, sizeof(stTimeoutItemLink_t));
    memset(timeout, 0, sizeof(stTimeoutItemLink_t));

    for (int i = 0; i < ret; i++)
    {
        stTimeoutItem_t *item = reinterpret_cast<stTimeoutItem_t*>(result->events[i].data.ptr);
        if (item->pfnPrepare) {
            item->pfnPrepare(item, result->events[i], active);
        } else {
            AddTail(active, item);
        }
    }

    unsigned long long now = GetTickMS();
    TakeAllTimeout(ctx->pTimeout, now, timeout);

    stTimeoutItem_t *lp = timeout->head;
    while (lp) {
        lp->bTimeout = true;
        lp = lp->pNext;
    }

    Join<stTimeoutItem_t, stTimeoutItemLink_t>(active, timeout);

    lp = active->head;
    while (lp) {

        PopHead<stTimeoutItem_t, stTimeoutItemLink_t>(active);
        if (lp->pfnProcess) {
            lp->pfnProcess(lp);
        }

        lp = active->head;
    }
}
void OnCoroutineEvent(stTimeoutItem_t * ap) {
    coroutine_resume(co_get_curr_thread_env()->co_schedule, ap->co_id);
}


int64_t get_curr_co_id()
{
    stCoRoutineEnv_t *env = co_get_curr_thread_env();
    if (!env) return -1;
    return coroutine_running(env->co_schedule);
}

int co_poll(stCoEpoll_t *ctx, struct pollfd fds[], nfds_t nfds, int timeout)
{
    if (timeout > stTimeoutItem_t::eMaxTimeout) {
        timeout = stTimeoutItem_t::eMaxTimeout;
    }
    int epfd = ctx->iEpollFd;

    // 1.struct change
    stPoll_t arg;
    memset(&arg, 0, sizeof(arg));

    arg.iEpollFd = epfd;
    arg.fds = fds;
    arg.nfds = nfds;

    stPollItem_t arr[2];
    if (nfds < sizeof(arr) / sizeof(arr[0])) {
        arg.pPollItems = arr;
    } else {
        arg.pPollItems = reinterpret_cast<stPollItem_t*>(malloc(nfds * sizeof(stPollItem_t)));
    }
    memset(arg.pPollItems, 0, nfds * sizeof(stPollItem_t));

    arg.pfnProcess = OnPollProcessEvent;
    arg.co_id = get_curr_co_id();

    // 2.add timeout
    unsigned long long now = GetTickMS();
    arg.ullExpireTime = now + timeout;
    int ret = AddTimeout(ctx->pTimeout, &arg, now);
    if (ret != 0) {
        co_log_err("CO_ERR: AddTimeout ret %d now %lld timeout %d arg.ullExpireTime %lld",
                    ret, now, timeout, arg.ullExpireTime);
        errno = EINVAL;
        return -__LINE__;
    }

    for (nfds_t i = 0; i < nfds; i++) {
        arg.pPollItems[i].pSelf = fds + i;
        arg.pPollItems[i].pPoll = &arg;

        arg.pPollItems[i].pfnPrepare = OnPollPreparePfn;
        struct epoll_event &ev = arg.pPollItems[i].stEvent;

        if (fds[i].fd > -1) {
            ev.data.ptr = arg.pPollItems + i;
            ev.events = PollEvent2Epoll(fds[i].events);

            epoll_ctl(epfd, EPOLL_CTL_ADD, fds[i].fd, &ev);
        }
    }

    coroutine_yield(co_get_curr_thread_env()->co_schedule);

    RemoveFromLink<stTimeoutItem_t, stTimeoutItemLink_t>(&arg);
    for (nfds_t i = 0; i < nfds; i++) {
        int fd = fds[i].fd;
        if (fd > -1) {
            epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &arg.pPollItems[i].stEvent);
        }
    }

    if (arg.pPollItems != arr) {
        free(arg.pPollItems);
        arg.pPollItems = NULL;
    }
    return arg.iRaiseCnt;
}


stCoEpoll_t *co_get_epoll_ct() {
    if (!co_get_curr_thread_env()) {
        return NULL;
    }
    return co_get_curr_thread_env()->pEpoll;
}

void co_enable_hook_sys() {
    if (!co_get_curr_thread_env()) {
        return;
    }
    coroutine *co = get_curr_coroutine(co_get_curr_thread_env()->co_schedule);
    if (co) {
        co->enable_hook = true;
    }
}

void co_disable_hook_sys()
{
    if (!co_get_curr_thread_env()) {
        return;
    }
    coroutine *co = get_curr_coroutine(co_get_curr_thread_env()->co_schedule);
    if (co) {
        co->enable_hook = false;
    }
}
bool co_is_enable_sys_hook() {
    if (!co_get_curr_thread_env()) {
        return false;
    }
    coroutine *co = get_curr_coroutine(co_get_curr_thread_env()->co_schedule);
    return (co && co->enable_hook);
}

coroutine *co_self() {
    if (!co_get_curr_thread_env()) {
        return NULL;
    }
    return get_curr_coroutine(co_get_curr_thread_env()->co_schedule);
}

} // namespace pebble
