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


#include <sstream>
#include "source/broadcast/channel_mgr.h"
#include "source/message/message.h"
#include "source/rpc/router/mod_router.h"
#include "source/rpc/router/random_router.h"
#include "source/rpc/transport/msg_buffer.h"


namespace pebble { namespace rpc { namespace transport {

bool MsgBuffer::isOpen() {
    if (m_sendFunc) {
        return true;
    }
    return false;
}

bool MsgBuffer::peek() {
    return m_input.peek();
}

void MsgBuffer::open() {
}

void MsgBuffer::close() {
    m_handle = -1;
    m_peer_url = "";
    if (m_sendFunc) {
        cxx::function<int(char*, size_t)> fun; // NOLINT
        m_sendFunc = fun;
    }
    m_input.resetBuffer();
    m_output.resetBuffer();
}

uint32_t MsgBuffer::read(uint8_t* buf, uint32_t len) {
    if (!isOpen()) {
        throw TTransportException(TTransportException::NOT_OPEN, "msgbuff not open.");
    }
    return m_input.read(buf, len);
}

uint32_t MsgBuffer::readAll(uint8_t* buf, uint32_t len) {
    if (!isOpen()) {
        throw TTransportException(TTransportException::NOT_OPEN, "msgbuff not open.");
    }
    return m_input.readAll(buf, len);
}

uint32_t MsgBuffer::readEnd() {
    if (0 != m_input.available_read()) {
        // 有些场景下可能未读完数据，直接清空(如收到不识别的消息)
    }
    uint32_t nread = m_input.readEnd();
    m_input.resetBuffer();

    return nread;
}

void MsgBuffer::write(const uint8_t* buf, uint32_t len) {
    if (!isOpen()) {
        throw TTransportException(TTransportException::NOT_OPEN, "msgbuff not open.");
    }
    m_output.write(buf, len);
}

uint32_t MsgBuffer::writeEnd() {
    uint8_t* buf = NULL;
    uint32_t len = 0;
    m_output.getBuffer(&buf, &len);
    uint32_t nwrite = m_output.writeEnd();
    m_output.resetBuffer();

    if ((0 == len) && pebble::net::Message::QueryDriver(m_handle)->Name() != "http") {
        // 对于oneway消息，只有http需要返回响应
        return 0;
    }

    int ret = m_sendFunc(reinterpret_cast<char*>(buf), len);
    if (ret != 0) {
        std::ostringstream oss;
        oss << "send msg failed." << ret;
        throw TTransportException(TTransportException::SEND_FAILED,  oss.str());
    }
    return nwrite;
}

void MsgBuffer::bind(int handle, const std::string& url, uint64_t addr) {
    // 服务地址刷新时，需要先调用close，再对新handle进行bind
    if (addr != 0) {
        m_sendFunc = cxx::bind(&pebble::net::Message::SendTo, handle, cxx::placeholders::_1,
                cxx::placeholders::_2, addr, 0);
    } else {
        m_sendFunc = cxx::bind(&pebble::net::Message::Send, handle, cxx::placeholders::_1,
                cxx::placeholders::_2, 0);
    }

    m_handle = handle;
    m_peer_url = url;
}

void MsgBuffer::bind(pebble::broadcast::ChannelMgr* channel_mgr,
    const std::string& channel_name, int encode_type) {
    if (!channel_mgr || channel_name.empty()) {
        return;
    }

    m_sendFunc = cxx::bind(&pebble::broadcast::ChannelMgr::Publish,
        channel_mgr, channel_name,
        cxx::placeholders::_1, cxx::placeholders::_2, encode_type, true);
}

void MsgBuffer::setMessage(const uint8_t* msgBuf, uint32_t msgBufLen) {
    // 这里使用拷贝模式，后续考虑针对底层传输协议进行优化
    // 只有同步需要COPY模式，异步不需要，待优化
    m_input.resetBuffer(const_cast<uint8_t*>(msgBuf), msgBufLen, TMemoryBuffer::COPY);
}

} // namespace transport
} // namespace rpc
} // namespace pebble

