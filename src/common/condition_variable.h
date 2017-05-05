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


#ifndef _PEBBLE_COMMON_CONDITION_VARIABLE_H_
#define _PEBBLE_COMMON_CONDITION_VARIABLE_H_

#include <assert.h>
#include <pthread.h>

#include "common/mutex.h"

namespace pebble {


class ConditionVariable
{
public:
    ConditionVariable();
    ~ConditionVariable();

    void Signal();

    void Broadcast();

    void Wait(Mutex* mutex);

    // If timeout_in_ms < 0, it means infinite waiting until condition is signaled
    // by another thread
    int TimedWait(Mutex* mutex, int timeout_in_ms = -1);
private:
    void CheckValid() const;
private:
    pthread_cond_t m_hCondition;
};

} // namespace pebble

#endif // _PEBBLE_COMMON_CONDITION_VARIABLE_H_

