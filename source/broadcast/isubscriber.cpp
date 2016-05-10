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

#include "source/broadcast/isubscriber.h"

namespace pebble {
namespace broadcast {

ISubscriber::ISubscriber() {
    m_id = GenID();
}

ISubscriber::~ISubscriber() {
}

int64_t ISubscriber::Id() const {
    return m_id;
}

int64_t ISubscriber::GenID() {
    // 框架内用无多线程问题
    static int64_t id = 0;
    return id++;
}

uint64_t ISubscriber::PeerAddress() const {
    return 0;
}

int64_t ISubscriber::SessionId() const {
    return -1;
}


} // namespace broadcast
} // namespace pebble

