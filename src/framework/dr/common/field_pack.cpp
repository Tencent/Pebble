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


#include "field_pack.h"
#include <cstring>


namespace pebble { namespace dr { namespace internal {

uint32_t PackerBuffer::read(uint8_t* buf, uint32_t len) {
    return readAll(buf, len);
}

uint32_t PackerBuffer::readAll(uint8_t* buf, uint32_t len) {
    uint8_t *new_buf_pos = m_buf_pos + len;
    if (new_buf_pos > m_buf_bound) {
        throw ArrayOutOfBoundsException();
    }
    std::memcpy(buf, m_buf_pos, len);
    m_buf_pos = new_buf_pos;
    return len;
}

void PackerBuffer::write(const uint8_t* buf, uint32_t len) {
    uint8_t *new_buf_pos = m_buf_pos + len;
    if (new_buf_pos > m_buf_bound) {
        throw ArrayOutOfBoundsException();
    }
    std::memcpy(m_buf_pos, buf, len);
    m_buf_pos = new_buf_pos;
}

void PackerBuffer::reset(uint8_t *buf, uint32_t buf_len) {
    m_buf_pos = m_buf = buf;
    m_buf_bound = buf + buf_len;
}

int32_t PackerBuffer::used() {
    return m_buf_pos - m_buf;
}

cxx::shared_ptr<PackerBuffer> FieldPackGlobal::packer_buffer(new PackerBuffer);

cxx::shared_ptr<pebble::dr::protocol::TBinaryProtocol> FieldPackGlobal::protocol(
        new pebble::dr::protocol::TBinaryProtocol(packer_buffer));

void FieldPackGlobal::reset(uint8_t *buf, uint32_t buf_len) {
    packer_buffer->reset(buf, buf_len);
    protocol->reset();
}

int32_t FieldPackGlobal::used() {
    return packer_buffer->used();
}

} // namespace internal
} // namespace dr
} // namespace pebble


