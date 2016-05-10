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


#include "source/app/pebble_server.h"
#include "source/app/subscriber.h"
#include <source/message/message.h>
#include <source/rpc/rpc.h>

namespace pebble {

Subscriber::Subscriber(pebble::PebbleServer* server)
    : broadcast::ISubscriber() {
    m_connection = NULL;
    m_server = server;

    BindConnection();
}

Subscriber::~Subscriber() {
    if (m_connection) {
        delete m_connection;
        m_connection = NULL;
    }
}

int Subscriber::BindConnection() {
    if (NULL == m_server) {
        return -1;
    }

    m_connection = new pebble::rpc::ConnectionObj();
    if (NULL == m_connection) {
        return -2;
    }

    int ret = m_server->GetCurConnectionObj(m_connection);
    if (ret != 0) {
        delete m_connection;
        m_connection = NULL;
        return -3;
    }

    // http 接入不支持广播
    if (pebble::net::Message::QueryDriver(m_connection->GetHandle())->Name() == "http") {
        delete m_connection;
        m_connection = NULL;
        return -4;
    }

    return 0;
}

int64_t Subscriber::Id() const {
    return m_id;
}

int Subscriber::Push(const std::string& msg, int32_t encode_type) {
    if (NULL == m_connection) {
        return -1;
    }

    if (encode_type != m_connection->GetProtocolType()) {
        return -2;
    }

    return pebble::net::Message::SendTo(m_connection->GetHandle(),
        msg.data(), msg.length(), m_connection->GetPeerAddr());
}

int Subscriber::IsDirectConnect() {
    if (NULL == m_connection) {
        return -1;
    }

    return (pebble::net::Message::QueryDriver(m_connection->GetHandle())->Name() != "pipe");
}

uint64_t Subscriber::PeerAddress() const {
    if (NULL == m_connection) {
        return 0;
    }

    return m_connection->GetPeerAddr();
}

int64_t Subscriber::SessionId() const {
    if (NULL == m_connection) {
        return -1;
    }

    return m_connection->GetSessionId();
}


}  // namespace pebble

