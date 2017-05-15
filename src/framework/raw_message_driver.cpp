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

#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

#include "common/string_utility.h"
#include "framework/net_message.h"
#include "framework/raw_message_driver.h"


namespace pebble {

#define _CAST_TO_HANDLE(netaddr) static_cast<int64_t>(netaddr)
#define _CAST_TO_NETADDR(handle) static_cast<uint64_t>(handle)


int32_t UrlToNetAddress(const std::string& url, std::string* ip, uint16_t* port)
{
    if (NULL == ip || NULL == port) {
        return -1;
    }

    size_t pos = url.find_last_of(':');
    if (std::string::npos == pos || url.size() == pos) {
        return -1;
    }

    ip->assign(url.substr(0, pos));
    *port = static_cast<uint16_t>(atoi(url.substr(pos + 1).c_str()));
    return 0;
}


RawMessageDriver::RawMessageDriver() {
    m_net_message = NULL;
}

RawMessageDriver::~RawMessageDriver() {
    delete m_net_message;
}

int32_t RawMessageDriver::Init(uint32_t msg_buff_len) {
    if (m_net_message) {
        delete m_net_message;
    }

    m_net_message = new NetMessage();
    return m_net_message->Init(sizeof(TcpMsgHead),
        cxx::bind(&RawMessageDriver::ParseHead, this, cxx::placeholders::_1, cxx::placeholders::_2),
        msg_buff_len);
}

int64_t RawMessageDriver::Bind(const std::string& url) {
    std::string ip;
    uint16_t port = 0;
    if (UrlToNetAddress(url, &ip, &port) != 0) {
        return kMESSAGE_INVAILD_PARAM;
    }

    if (StringUtility::StartsWith(ip, "raw")) {
        ip.erase(0, strlen("raw"));
    }

    uint64_t handle = m_net_message->Bind(ip, port);

    return handle != INVAILD_HANDLE ? handle : -1;
}

int64_t RawMessageDriver::Connect(const std::string& url) {
    std::string ip;
    uint16_t port = 0;
    if (UrlToNetAddress(url, &ip, &port) != 0) {
        return kMESSAGE_INVAILD_PARAM;
    }

    if (StringUtility::StartsWith(ip, "raw")) {
        ip.erase(0, strlen("raw"));
    }

    uint64_t handle = m_net_message->Connect(ip, port);

    return handle != INVAILD_HANDLE ? handle : -1;
}

int32_t RawMessageDriver::Send(int64_t handle, const uint8_t* msg, uint32_t msg_len, int32_t flag) {
    if (m_net_message->IsTcpTransport(handle)) {
        TcpMsgHead head;
        head._magic    = htonl(head._magic);
        head._version  = htonl(head._version);
        head._data_len = htonl(msg_len);
        const uint8_t* frags[2] = { (uint8_t*)(&head),  msg     };
        uint32_t fragslen[2]    = { sizeof(TcpMsgHead), msg_len };
        return m_net_message->SendV(handle, 2, frags, fragslen);
    } else {
        return m_net_message->Send(handle, msg, msg_len);
    }
}

int32_t RawMessageDriver::SendV(int64_t handle, uint32_t msg_frag_num,
                          const uint8_t* msg_frag[], uint32_t msg_frag_len[], int32_t flag) {
    if (m_net_message->IsTcpTransport(handle)) {
        const uint8_t** tmp_frags = new const uint8_t*[msg_frag_num + 1];
        uint32_t* tmp_frag_len = new uint32_t[msg_frag_num + 1];

        int32_t msg_len = 0;
        for (uint32_t i = 0; i < msg_frag_num; i++) {
            msg_len += msg_frag_len[i];

            tmp_frags[ i + 1 ]    = msg_frag[i];
            tmp_frag_len[ i + 1 ] = msg_frag_len[i];
        }

        TcpMsgHead head;
        head._magic    = htonl(head._magic);
        head._version  = htonl(head._version);
        head._data_len = htonl(msg_len);

        tmp_frags[0]    = (uint8_t*)(&head);
        tmp_frag_len[0] = sizeof(TcpMsgHead);

        int32_t ret = m_net_message->SendV(handle, msg_frag_num + 1, tmp_frags, tmp_frag_len);
        delete [] tmp_frags;
        delete [] tmp_frag_len;
        return ret;
    } else {
        return m_net_message->SendV(handle, msg_frag_num, msg_frag, msg_frag_len);
    }
}

int32_t RawMessageDriver::Recv(int64_t handle, uint8_t* msg_buff, uint32_t* buff_len,
                         MsgExternInfo* msg_info) {
    if (!m_net_message->IsTcpTransport(handle)) {
        return m_net_message->Recv(handle, msg_buff, buff_len, msg_info);
    }

    // tcp
    const uint8_t* tmp_msg = NULL;
    uint32_t tmp_len = 0;
    int32_t ret = m_net_message->Peek(handle, &tmp_msg, &tmp_len, msg_info);
    if (ret < 0) {
        return ret;
    }

    uint32_t head_len = sizeof(TcpMsgHead);
    if (tmp_len < head_len) {
        Close(handle);
        return kMESSAGE_RECV_INVALID_DATA;
    }

    uint32_t data_len = tmp_len - head_len;
    if (*buff_len < data_len) {
        return kMESSAGE_RECV_BUFF_NOT_ENOUGH;
    }

    memcpy(msg_buff, tmp_msg + head_len, data_len);
    *buff_len = data_len;

    m_net_message->Pop(handle);

    return 0;
}

int32_t RawMessageDriver::Peek(int64_t handle,
    const uint8_t** msg, uint32_t* msg_len, MsgExternInfo* msg_info) {
    if (!m_net_message->IsTcpTransport(handle)) {
        return m_net_message->Peek(handle, msg, msg_len, msg_info);
    }

    // tcp
    const uint8_t* tmp_msg = NULL;
    uint32_t tmp_len = 0;
    int32_t ret = m_net_message->Peek(handle, &tmp_msg, &tmp_len, msg_info);
    if (ret < 0) {
        return ret;
    }

    uint32_t head_len = sizeof(TcpMsgHead);
    if (tmp_len < head_len) {
        Close(handle);
        return kMESSAGE_RECV_INVALID_DATA;
    }

    *msg = tmp_msg + head_len;
    *msg_len = tmp_len - head_len;

    return 0;
}

int32_t RawMessageDriver::Pop(int64_t handle) {
    return m_net_message->Pop(handle);
}

int32_t RawMessageDriver::Close(int64_t handle) {
    return m_net_message->Close(handle);
}

int32_t RawMessageDriver::Poll(int64_t* handle, int32_t* event, int32_t timeout_ms) {
    return m_net_message->Poll((uint64_t*)handle, event, timeout_ms);
}

int32_t RawMessageDriver::GetUsedSize(int64_t handle, uint32_t* remain_size, uint32_t* max_size) {
    return kMESSAGE_UNSUPPORT;
}

const char* RawMessageDriver::GetLastError() {
    if (m_net_message) {
        return m_net_message->GetLastError();
    }
    return "uninited.";
}

int32_t RawMessageDriver::ParseHead(const uint8_t* head, uint32_t head_len) {
    if (head == NULL || head_len < sizeof(TcpMsgHead)) {
        return -1;
    }
    TcpMsgHead* msg_head = (TcpMsgHead*)head;
    if (ntohl(msg_head->_magic) != TCP_HEAD_MAGIC) {
        return -2;
    }
    // 暂无版本检查
    return ntohl(msg_head->_data_len);
}


} // namespace pebble
