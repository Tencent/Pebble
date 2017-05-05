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


#include "thread.h"

namespace pebble {

Thread::Thread() {
    m_thread_id = 0;
}

Thread::~Thread() {
}
static void* ThreadEntry(void* arg) {
    Thread* thread = reinterpret_cast<Thread*>(arg);

    thread->Run();

    return NULL;
}

bool Thread::Start() {
    if (::pthread_create(&m_thread_id, NULL, ThreadEntry, this) != 0) {
        return false;
    }

    return true;
}

bool Thread::Join() {
    return (::pthread_join(m_thread_id, NULL) == 0);
}


void Thread::Exit() {
    ::pthread_exit(NULL);
}

} // namespace pebble
