/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef PEBBLE_RPC_PROTOCOL_PROTOCOL_FACTORY_H
#define PEBBLE_RPC_PROTOCOL_PROTOCOL_FACTORY_H

#include "source/rpc/transport/transport.h"
#include "source/rpc/protocol/multiplexed_protocol.h"
#include "source/rpc/protocol/json_protocol.h"
#include "source/rpc/protocol/bson_protocol.h"
#include "source/rpc/protocol/binary_protocol.h"
#include "source/rpc/common/common.h"
#include <tr1/memory>

namespace pebble { namespace rpc { namespace protocol {

static const int MAX_STRING_SIZE = (8 * 1024 * 1024); // 8M
static const int MAX_CONTAINER_SIZE = (8 * 1024 * 1024); // 8M


/**
 * RPC框架protocol统一生成实现
 */
class ProtocolFactory
{
public:
    ProtocolFactory() {}

    virtual ~ProtocolFactory() {}

    // 根据协议类型和transport获取protocol
    virtual cxx::shared_ptr<TProtocol> getProtocol(rpc::PROTOCOL_TYPE protocol_type,
                                                   cxx::shared_ptr<TTransport> trans)
    {
        cxx::shared_ptr<TProtocol> prot;
        switch (protocol_type) {
            case PROTOCOL_BINARY:
                prot.reset(new TBinaryProtocol(trans,
                    MAX_STRING_SIZE, MAX_CONTAINER_SIZE, false, false));
                break;

            case PROTOCOL_JSON:
                prot.reset(new TJSONProtocol(trans));
                break;

            case PROTOCOL_BSON:
                prot.reset(new TBSONProtocol(trans));
                break;
            // add other protocol...

            default:
                break;
        }
        return prot;
    }

    // 根据协议类型、transport以及服务名获取可携带服务名的protocol
    virtual cxx::shared_ptr<TProtocol> getMultiplexedProtocol(rpc::PROTOCOL_TYPE protocol_type,
                                                              cxx::shared_ptr<TTransport> trans,
                                                              const std::string& service_name)
    {
        cxx::shared_ptr<TProtocol> prot(getProtocol(protocol_type, trans));
        if (prot)
        {
            return cxx::shared_ptr<TProtocol>(new TMultiplexedProtocol(prot, service_name));
        }
        return prot;
    }
};


}}} // apache::thrift::protocol

#endif // PEBBLE_RPC_PROTOCOL_PROTOCOL_FACTORY_H
