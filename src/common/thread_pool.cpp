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

#include "common/thread_pool.h"

namespace pebble {


ThreadPool::ThreadPool() : m_exit(false), m_initialized(false),
    m_thread_num(0), m_mode(PENDING) {
}


ThreadPool::~ThreadPool() {
    Terminate();
}

int ThreadPool::Init(int32_t thread_num, int32_t mode) {
    if (m_initialized) {
        return -1;
    }

    if (thread_num <= 0) {
        return -2;
    }

    if (thread_num > 256) {
        thread_num = 256;
    }
    m_thread_num = thread_num;

    if (mode >= NO_PENDING || mode < PENDING) {
        m_mode = mode;
    }

    for (int32_t i = 0; i < thread_num; i++) {

        InnerThread* thread = new InnerThread(&m_pending_queue, &m_finished_queue, &m_working_queue);
        m_threads.push_back(thread);

        thread->Start();
    }

    m_initialized = true;

    return 0;
}

int ThreadPool::AddTask(cxx::function<void()>& fun, int64_t task_id) {
    if (!m_initialized) {
        return -1;
    }

    if (m_mode == NO_PENDING) {
        Stats stat;
        GetStatus(&stat);
        if (stat.working_thread_num + stat.pending_task_num >= m_thread_num) {
            return -2;
        }
    }

    Task t;
    t.fun = fun;
    t.task_id = task_id;

    m_pending_queue.PushBack(t);

    return 0;
}

void ThreadPool::GetStatus(Stats* stat) {
    if (stat == NULL) {
        return;
    }
    stat->pending_task_num = m_pending_queue.Size();
    stat->working_thread_num = m_working_queue.Size();
}

void ThreadPool::Terminate(bool waiting /* = true */) {
    m_exit = true;
    for (size_t i = 0; i < m_threads.size(); i++) {
        m_threads[i]->Terminate(waiting);
    }

    for (size_t i = 0; i < m_threads.size(); i++) {
        m_threads[i]->Join();
        delete m_threads[i];
    }

    m_pending_queue.Clear();
    m_finished_queue.Clear();
    m_threads.clear();
}

bool ThreadPool::GetFinishedTaskID(int64_t* task_id) {
    if (m_finished_queue.IsEmpty()) {
        return false;
    }

    bool ret = m_finished_queue.TimedPopFront(task_id, 1);
    return ret;
}

ThreadPool::InnerThread::InnerThread(BlockingQueue<Task>* pending_queue,
    BlockingQueue<int64_t>* finished_queue, BlockingQueue<int64_t>* working_queue) :
        m_pending_queue(pending_queue),
        m_finished_queue(finished_queue),
        m_working_queue(working_queue),
        m_exit(false),
        m_waiting(true) {
}

void ThreadPool::InnerThread::Run() {
    while (1) {

        if (m_exit && ((!m_waiting) || (m_waiting && m_pending_queue->IsEmpty()))) {
            break;
        }

        Task t;
        bool ret = m_pending_queue->TimedPopFront(&t, 1000);
        if (ret) {
            m_working_queue->PushBack(0); // 不关心元素的值，只是通过队列size记录忙的线程数
            t.fun();
            if (t.task_id >= 0) {
                m_finished_queue->PushBack(t.task_id);
            }
            int64_t tmp;
            m_working_queue->TryPopBack(&tmp);
        }
    }
}

void ThreadPool::InnerThread::Terminate(bool waiting /* = true */) {
    m_exit = true;
    m_waiting = waiting;
}


} // namespace pebble
