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

#include "serialize.h"
#include <cstring>
#include <framework/dr/protocol/binary_protocol.h>
#include <iostream>


namespace pebble { namespace dr { namespace detail {

uint32_t FixBuffer::read(uint8_t* buf, uint32_t len) {
    return readAll(buf, len);
}

uint32_t FixBuffer::readAll(uint8_t* buf, uint32_t len) {
    uint8_t *new_buf_pos = m_buf_pos + len;
    if (new_buf_pos > m_buf_bound) {
        throw ArrayOutOfBoundsException();
    }
    std::memcpy(buf, m_buf_pos, len);
    m_buf_pos = new_buf_pos;
    return len;
}

void FixBuffer::write(const uint8_t* buf, uint32_t len) {
    uint8_t *new_buf_pos = m_buf_pos + len;
    if (new_buf_pos > m_buf_bound) {
        throw ArrayOutOfBoundsException();
    }
    std::memcpy(m_buf_pos, buf, len);
    m_buf_pos = new_buf_pos;
}

int32_t FixBuffer::used() {
    return m_buf_pos - m_buf;
}

void AutoBuffer::write(const uint8_t* buf, uint32_t len) {
    uint8_t *new_buf_pos = m_buf_pos + len;
    if (new_buf_pos > m_buf_bound) {
        uint32_t org_used = used();
        uint32_t new_size = (org_used + len) * 2;
        m_buf = reinterpret_cast<uint8_t *>(realloc(m_buf, new_size));
        m_buf_pos = m_buf + org_used;
        m_buf_bound = m_buf + new_size;
        new_buf_pos = m_buf_pos + len;
    }
    std::memcpy(m_buf_pos, buf, len);
    m_buf_pos = new_buf_pos;
}

int32_t AutoBuffer::used() {
    return m_buf_pos - m_buf;
}

char *AutoBuffer::str() {
    return reinterpret_cast<char *>(m_buf);
}

// 找到字段话，返回字段长度，否则返回0
static int find_field(const std::vector<int16_t> &path, size_t path_idx,
    pebble::dr::protocol::TProtocol &protocol, uint32_t xfer_base,
    pebble::dr::protocol::TType &found_field_type, uint32_t &found_field_pos) {
    if (path_idx < path.size()) {
        int16_t fid_want = path[path_idx];

        uint32_t xfer = 0;
        std::string fname;
        ::pebble::dr::protocol::TType ftype;
        int16_t fid;

        xfer = protocol.readStructBegin(fname);
        while (true) {
            xfer += protocol.readFieldBegin(fname, ftype, fid);
            if (ftype == ::pebble::dr::protocol::T_STOP) {
                break;
            }
            if (fid == fid_want) {
                if (path.size() - 1 == path_idx) {
                    found_field_type = ftype;
                    found_field_pos = xfer_base + xfer;
                    return static_cast<int>(protocol.skip(ftype));
                } else {
                    if (ftype != ::pebble::dr::protocol::T_STRUCT) {
                        return 0;
                    }
                    return find_field(path, path_idx + 1, protocol, xfer_base + xfer,
                        found_field_type, found_field_pos);
                }
            } else {
                xfer += protocol.skip(ftype);
            }
            xfer += protocol.readFieldEnd();
        }
        // xfer += protocol.readStructEnd();
    }
    return 0;
}

} // namespace detail

static const size_t fix_type_len[pebble::dr::protocol::T_UTF16 + 1] = {
    0, 0, 1, 1, 8, 0, 2, 0, 4, 8, 8, 0, 0, 0, 0, 0, 0, 0
};

int UpdateField(const std::vector<int16_t> &path, char *old_data, size_t old_data_len,
    char *field, size_t field_len, char *new_data_buf, size_t new_data_buf_len) {

    uint32_t old_field_pos = 0;
    pebble::dr::protocol::TType ftype =
        pebble::dr::protocol::T_STOP;
    try {
        cxx::shared_ptr<detail::FixBuffer> f_buff(new detail::FixBuffer(
            reinterpret_cast<uint8_t*>(old_data), old_data_len));
        pebble::dr::protocol::TBinaryProtocol protocol(f_buff);

        int old_field_len = detail::find_field(path, 0, protocol, 0, ftype, old_field_pos);

        if (old_field_len == 0) {
            return pebble::dr::kINVALIDPATH;
        }

        if (ftype == pebble::dr::protocol::T_STOP ||
            ftype == pebble::dr::protocol::T_VOID) {
            return pebble::dr::kINVALIDPARAMETER;
        }

        if (new_data_buf_len < (old_data_len - old_field_len + field_len)) {
            return pebble::dr::kINSUFFICIENTBUFFER;
        }

        if (fix_type_len[ftype] && fix_type_len[ftype] != field_len) {
            return pebble::dr::kINVALIDFIELD;
        }

        memcpy(new_data_buf, old_data, old_field_pos);
        memcpy(new_data_buf + old_field_pos, field, field_len);
        memcpy(new_data_buf + old_field_pos + field_len, old_data + old_field_pos + old_field_len,
            old_data_len - old_field_pos - old_field_len);
        return old_data_len - old_field_len + field_len;
    } catch (pebble::dr::detail::ArrayOutOfBoundsException) {
        return pebble::dr::kINVALIDBUFFER;
    } catch (...) {
        return pebble::dr::kUNKNOW;
    }
}

} // namespace dr
} // namespace pebble
