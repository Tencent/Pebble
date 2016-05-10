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
#include "source/rpc/protocol/group_protocol.h"
#include "source/service/service_manage.h"


namespace pebble { namespace rpc { namespace protocol {

GroupProtocol::GroupProtocol()
     : m_route_type(1), m_route_key(0), m_poll_cnt(0) {
}

GroupProtocol::~GroupProtocol() {
}

void GroupProtocol::addProtocol(cxx::shared_ptr<TProtocol> prot) {
    m_protocols.push_back(prot);
}

int GroupProtocol::rmvProtocol(cxx::shared_ptr<TProtocol> prot) {
    std::vector< cxx::shared_ptr<TProtocol> >::iterator it;
    int i = 0;
    for (it = m_protocols.begin(); it != m_protocols.end(); ++it, ++i) {
        if (*it == prot) {
            m_protocols.erase(it);
            return 0;
        }
    }
    return -1;
}

std::vector< cxx::shared_ptr<TProtocol> > GroupProtocol::getProtocols() {
    return m_protocols;
}

void GroupProtocol::setRouteType(int route_type) {
    if (route_type > pebble::service::kUnknownRoute
        && route_type < pebble::service::kMAXRouteVal) {
        m_route_type = route_type;
    }
}

void GroupProtocol::setRouteKey(uint64_t route_key) {
    m_route_key = route_key;
}

int GroupProtocol::selectProtocol() {
    if (m_protocols.empty()) {
        throw TProtocolException(TProtocolException::NULL_PROTOCOL);
    }
    int index = 0;
    switch(m_route_type) {
        case pebble::service::kModRoute:
            index = routeByMod();
            break;
        case pebble::service::kRoundRoute:
            index = routeByPoll();
            break;
        case pebble::service::kHashRoute:
        default:
            break;
    }
    protocol = m_protocols[index];
    return 0;
}

int GroupProtocol::routeByMod() {
    return (m_route_key % m_protocols.size());
}

int GroupProtocol::routeByPoll() {
    return (m_poll_cnt++ % m_protocols.size());
}


} // namespace protocol
} // namespace rpc
} // namespace pebble

