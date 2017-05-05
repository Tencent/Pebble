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


#ifndef PEBBLE_DR_INTERNAL_FIELDPACK_H
#define PEBBLE_DR_INTERNAL_FIELDPACK_H

#include "framework/dr/protocol/binary_protocol.h"
#include "framework/dr/transport/virtual_transport.h"

namespace pebble { namespace dr { namespace internal {
class ArrayOutOfBoundsException : public std::exception {
};

// 仅供内部使用
class PackerBuffer : public pebble::dr::transport::TVirtualTransport<PackerBuffer> {
    public:
        PackerBuffer()
            : m_buf(NULL),
              m_buf_bound(NULL),
              m_buf_pos(NULL) {
        }

        uint32_t read(uint8_t* buf, uint32_t len);

        uint32_t readAll(uint8_t* buf, uint32_t len);

        void write(const uint8_t* buf, uint32_t len);

        void reset(uint8_t *buf, uint32_t buf_len);

        int32_t used();

    private:
        uint8_t *m_buf;

        uint8_t *m_buf_bound;

        uint8_t *m_buf_pos;
};

// 仅供内部使用
class FieldPackGlobal {
    public:
        static cxx::shared_ptr<pebble::dr::protocol::TBinaryProtocol> protocol;

        static void reset(uint8_t *buf, uint32_t buf_len);

        static int32_t used();

    private:
        static cxx::shared_ptr<PackerBuffer> packer_buffer;
};

} // namespace internal
} // namespace dr
} // namespace pebble


#endif

