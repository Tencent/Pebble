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


#ifndef PEBBLE_RPC_DEFINE_H
#define PEBBLE_RPC_DEFINE_H

#include <stdint.h>
#include <string>
#include <vector>


namespace pebble {
namespace rpc {

typedef enum {
    PROTOCOL_BASE    = 0x20,
    PROTOCOL_BINARY  = PROTOCOL_BASE,
    PROTOCOL_JSON    = 0x21,
    PROTOCOL_BSON    = 0x22,
    // add other protocol...
    PROTOCOL_BUTT
} PROTOCOL_TYPE;

namespace protocol {
/**
 * Enumerated definition of the message types that the Thrift protocol
 * supports.
 */
enum TMessageType {
    T_CALL       = 1,
    T_REPLY      = 2,
    T_EXCEPTION  = 3,
    T_ONEWAY     = 4,
};
} // namespace protocol

// TBus刷新时间间隔，单位（ms），默认为一分钟
static const int64_t kRefreshTbusIntervalMs = 60 *1000;

// 请求超时时间，单位毫秒
static const int64_t kRequestTimeOutMs = 10 * 1000;


} // namespace rpc
} // namespace pebble


#endif
