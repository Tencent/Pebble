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

#include <algorithm>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include "common/time_utility.h"
#include "common/timer.h"


namespace pebble {


#if 0
FdTimer::FdTimer() {
    m_max_timer_num = 1024;
    m_timer_seqid   = 0;
    m_last_error[0] = 0;
    // 2.6.8以上内核版本参数无效，填128仅做内核版本兼容性保证
    m_epoll_fd = epoll_create(128);
    if (m_epoll_fd < 0) {
        _LOG_LAST_ERROR("epoll_create failed(%s)", strerror(errno));
    }
}

FdTimer::~FdTimer() {
    int32_t fd = -1;
    cxx::unordered_map<int64_t, TimerItem*>::iterator it = m_timers.begin();
    for (; it != m_timers.end(); ++it) {
        fd = it->second->fd;
        if (epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, NULL) < 0) {
            _LOG_LAST_ERROR("epoll_ctl del %d failed(%s)", fd, strerror(errno));
        }

        close(fd);

        delete it->second;
        it->second = NULL;
    }

    m_timers.clear();

    if (m_epoll_fd >= 0) {
        close(m_epoll_fd);
    }
}

int64_t FdTimer::StartTimer(uint32_t timeout_ms, const TimeoutCallback& cb) {
    if (!cb || 0 == timeout_ms) {
        _LOG_LAST_ERROR("param is invalid: timeout_ms = %u, cb = %d", timeout_ms, (cb ? true : false));
        return kTIMER_INVALID_PARAM;
    }
    if (m_timers.size() >= m_max_timer_num) {
        _LOG_LAST_ERROR("timer over the limit(%d)", m_max_timer_num);
        return kTIMER_NUM_OUT_OF_RANGE;
    }

    // 创建timer fd
    int32_t fd = timerfd_create(CLOCK_REALTIME, 0);
    if (fd < 0) {
        _LOG_LAST_ERROR("timerfd_create failed(%s)", strerror(errno));
        return kSYSTEM_ERROR;
    }

    // 使用fcntl替代create的第2个参数仅做内核版本兼容性保证
    int32_t flags = fcntl(fd, F_GETFL, 0);
    flags < 0 ? flags = O_NONBLOCK : flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0) {
        close(fd);
        _LOG_LAST_ERROR("fcntl %d failed(%s)", fd, strerror(errno));
        return kSYSTEM_ERROR;
    }

    // 设置超时时间
    struct itimerspec timeout;
    timeout.it_value.tv_sec     = timeout_ms / 1000;
    timeout.it_value.tv_nsec    = (timeout_ms % 1000) * 1000 * 1000;
    timeout.it_interval.tv_sec  = timeout.it_value.tv_sec;
    timeout.it_interval.tv_nsec = timeout.it_value.tv_nsec;

    if (timerfd_settime(fd, 0, &timeout, NULL) < 0) {
        close(fd);
        _LOG_LAST_ERROR("timerfd_settime %d failed(%s)", fd, strerror(errno));
        return kSYSTEM_ERROR;
    }

    TimerItem* timeritem = new TimerItem();
    timeritem->fd = fd;
    timeritem->cb = cb;
    timeritem->id = m_timer_seqid;

    struct epoll_event event;
    event.data.ptr = timeritem;
    event.events   = EPOLLIN | EPOLLET;
    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &event) < 0) {
        close(fd);
        delete timeritem;
        timeritem = NULL;
        _LOG_LAST_ERROR("epoll_ctl add %d failed(%s)", fd, strerror(errno));
        return kSYSTEM_ERROR;
    }

    m_timers[m_timer_seqid] = timeritem;

    return m_timer_seqid++;
}

int32_t FdTimer::StopTimer(int64_t timer_id) {
    cxx::unordered_map<int64_t, TimerItem*>::iterator it = m_timers.find(timer_id);
    if (m_timers.end() == it) {
        _LOG_LAST_ERROR("timer id %ld not exist", timer_id);
        return kTIMER_UNEXISTED;
    }

    int32_t fd = it->second->fd;
    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, NULL) < 0) {
        _LOG_LAST_ERROR("epoll_ctl del %d failed(%s)", fd, strerror(errno));
    }

    close(fd);

    delete it->second;
    it->second = NULL;

    m_timers.erase(it);

    return 0;
}

int32_t FdTimer::Update() {
    const size_t MAX_EVENTS = 256;
    const uint32_t BUFF_LEN = 128;
    struct epoll_event events[MAX_EVENTS];

    int32_t num = epoll_wait(m_epoll_fd, events, std::min(MAX_EVENTS, m_timers.size()), 0);
    if (num <= 0) {
        return 0;
    }

    char buf[BUFF_LEN]   = {0};
    int64_t timerid      = 0;
    int32_t fd           = -1;
    int32_t ret          = 0;
    TimerItem* timeritem = NULL;

    for (int32_t i = 0; i < num; i++) {
        timeritem = static_cast<TimerItem*>(events[i].data.ptr);
        while (read(timeritem->fd, buf, BUFF_LEN) > 0) {} // NOLINT

        // 定时器可能在回调中被停掉，这里记录有效值
        timerid = timeritem->id;
        fd      = timeritem->fd;
        ret     = timeritem->cb();

        // 返回 <0 删除定时器，=0 继续，>0按新的超时时间重启定时器
        if (ret < 0) {
            StopTimer(timerid);
        } else if (ret > 0) {
            struct itimerspec timeout;
            timeout.it_value.tv_sec     = ret / 1000;
            timeout.it_value.tv_nsec    = (ret % 1000) * 1000 * 1000;
            timeout.it_interval.tv_sec  = timeout.it_value.tv_sec;
            timeout.it_interval.tv_nsec = timeout.it_value.tv_nsec;
            timerfd_settime(fd, 0, &timeout, NULL);
        }
    }

    return num;
}
#endif

SequenceTimer::SequenceTimer() {
    m_timer_seqid   = 0;
    m_last_error[0] = 0;
}

SequenceTimer::~SequenceTimer() {
}

int64_t SequenceTimer::StartTimer(uint32_t timeout_ms, const TimeoutCallback& cb) {
    if (!cb || 0 == timeout_ms) {
        _LOG_LAST_ERROR("param is invalid: timeout_ms = %u, cb = %d", timeout_ms, (cb ? true : false));
        return kTIMER_INVALID_PARAM;
    }

    cxx::shared_ptr<TimerItem> item(new TimerItem);
    item->stoped   = false;
    item->id       = m_timer_seqid;
    item->timeout  = TimeUtility::GetCurrentMS() + timeout_ms;
    item->cb       = cb;

    m_timers[timeout_ms].push_back(item);
    m_id_2_timer[m_timer_seqid] = item;

    return m_timer_seqid++;
}

int32_t SequenceTimer::StopTimer(int64_t timer_id) {
    cxx::unordered_map<int64_t, cxx::shared_ptr<TimerItem> >::iterator it =
        m_id_2_timer.find(timer_id);
    if (m_id_2_timer.end() == it) {
        _LOG_LAST_ERROR("timer id %ld not exist", timer_id);
        return kTIMER_UNEXISTED;
    }

    it->second->stoped = true;
    m_id_2_timer.erase(it);

    return 0;
}

int32_t SequenceTimer::Update() {
    int32_t num = 0;
    int64_t now = TimeUtility::GetCurrentMS();
    int32_t ret = 0;
    uint32_t old_timeout = 0;
    uint32_t timer_map_size = m_timers.size();

    cxx::unordered_map<uint32_t, std::list<cxx::shared_ptr<TimerItem> > >::iterator mit =
        m_timers.begin();
    std::list<cxx::shared_ptr<TimerItem> >::iterator lit;
    while (mit != m_timers.end()) {
        // 暂不考虑主动清理，顺序定时器即使不清理，也不会占用太多内存，频繁清理反而会影响性能

        std::list<cxx::shared_ptr<TimerItem> >& timer_list = mit->second;
        while (!timer_list.empty()) {
            lit = timer_list.begin();
            if ((*lit)->stoped) {
                timer_list.erase(lit);
                continue;
            }

            if ((*lit)->timeout > now) {
                // 此队列后面的都未超时
                break;
            }

            old_timeout = mit->first;
            ret = (*lit)->cb();

            // 返回 <0 删除定时器，=0 继续，>0按新的超时时间重启定时器
            if (ret < 0) {
                m_id_2_timer.erase((*lit)->id);
                timer_list.erase(lit);
            } else {
                cxx::shared_ptr<TimerItem> back_item = *lit;
                timer_list.erase(lit);
                if (ret > 0) {
                    back_item->timeout = now + ret;
                    m_timers[ret].push_back(back_item);
                } else {
                    back_item->timeout = now + old_timeout;
                    timer_list.push_back(back_item);
                }
            }

            ++num;
        }

        if (timer_map_size != m_timers.size()) {
            // 暂时用此方法防止迭代器失效，m_timers目前实现只会增加不会减少
            break;
        }
        ++mit;
    }

    return num;
}

}  // namespace pebble

