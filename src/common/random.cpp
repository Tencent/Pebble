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

#include <fcntl.h>
#include <unistd.h>
#include "common/random.h"

namespace pebble {

TrueRandom::TrueRandom()
    : m_fd(-1) {
    m_fd = open("/dev/urandom", O_RDONLY, 0);
    if (m_fd < 0) {
        abort();
    }
}

TrueRandom::~TrueRandom() {
    close(m_fd);
    m_fd = -1;
}

bool TrueRandom::NextBytes(void* buffer, size_t size) {
    return read(m_fd, buffer, size) == static_cast<int32_t>(size);
}

uint32_t TrueRandom::NextUInt32() {
    uint32_t random = -1;
    NextBytes(&random, sizeof(random));
    return random;
}

uint32_t TrueRandom::NextUInt32(uint32_t max_random) {
    return NextUInt32() % max_random;
}

} // namespace pebble
