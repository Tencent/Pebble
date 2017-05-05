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
#include <sys/time.h>
#include <stdexcept>
#include <stdint.h>
#include <string>

#include "common/condition_variable.h"


namespace pebble {

ConditionVariable::ConditionVariable() {
    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    pthread_cond_init(&m_hCondition, &cond_attr);
    pthread_condattr_destroy(&cond_attr);
}

ConditionVariable::~ConditionVariable() {
    pthread_cond_destroy(&m_hCondition);
}

void ConditionVariable::CheckValid() const
{
    assert(m_hCondition.__data.__total_seq != -1ULL && "this cond has been destructed");
}

void ConditionVariable::Signal() {
    CheckValid();
    pthread_cond_signal(&m_hCondition);
}

void ConditionVariable::Broadcast() {
    CheckValid();
    pthread_cond_broadcast(&m_hCondition);
}

void ConditionVariable::Wait(Mutex* mutex) {
    CheckValid();
    pthread_cond_wait(&m_hCondition, mutex->GetMutex());
}

int ConditionVariable::TimedWait(Mutex* mutex, int timeout_in_ms) {
    // -1 转为无限等
    if (timeout_in_ms < 0) {
        Wait(mutex);
        return 0;
    }

    timespec ts;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t usec = tv.tv_usec + timeout_in_ms * 1000LL;
    ts.tv_sec = tv.tv_sec + usec / 1000000;
    ts.tv_nsec = (usec % 1000000) * 1000;

    return pthread_cond_timedwait(&m_hCondition, mutex->GetMutex(), &ts);
}

} // namespace pebble
