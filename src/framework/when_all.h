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

#ifndef SRC_EXTENSION_WHEN_ALL_H
#define SRC_EXTENSION_WHEN_ALL_H
#pragma once

#include <vector>
#include "common/platform.h"
#include "common/log.h"

namespace pebble {

typedef cxx::function<int32_t(uint32_t*, uint32_t*)> Call;

template <typename ServiceMethod,
          typename Stub,
          typename Response,
          typename RetCode>
Call AddCall(const ServiceMethod& service_method,
             const Stub& stub,
             Response response,
             RetCode ret_code) {
    return cxx::bind(service_method,
                     stub,
                     ret_code,
                     response,
                     cxx::placeholders::_1,
                     cxx::placeholders::_2);
}

template <typename ServiceMethod,
          typename Stub,
          typename Response,
          typename RetCode,
          typename Request>
Call AddCall(const ServiceMethod& service_method,
             const Stub& stub,
             Response response,
             RetCode ret_code,
             const Request& request) {
    return cxx::bind(service_method,
                     stub,
                     cxx::ref(request),
                     ret_code,
                     response,
                     cxx::placeholders::_1,
                     cxx::placeholders::_2);
}

template <typename ServiceMethod,
          typename Stub,
          typename Response,
          typename RetCode,
          typename RequestArg1,
          typename RequestArg2>
Call AddCall(const ServiceMethod& service_method,
             const Stub& stub,
             Response response,
             RetCode ret_code,
             const RequestArg1& request_arg1,
             const RequestArg2& request_arg2) {
    return cxx::bind(service_method,
                     stub,
                     cxx::ref(request_arg1),
                     cxx::ref(request_arg2),
                     ret_code,
                     response,
                     cxx::placeholders::_1,
                     cxx::placeholders::_2);
}

template <typename ServiceMethod,
          typename Stub,
          typename Response,
          typename RetCode,
          typename RequestArg1,
          typename RequestArg2,
          typename RequestArg3>
Call AddCall(const ServiceMethod& service_method,
             const Stub& stub,
             Response response,
             RetCode ret_code,
             const RequestArg1& request_arg1,
             const RequestArg2& request_arg2,
             const RequestArg3& request_arg3) {
    return cxx::bind(service_method,
                     stub,
                     cxx::ref(request_arg1),
                     cxx::ref(request_arg2),
                     cxx::ref(request_arg3),
                     ret_code,
                     response,
                     cxx::placeholders::_1,
                     cxx::placeholders::_2);
}

template <typename ServiceMethod,
          typename Stub,
          typename Response,
          typename RetCode,
          typename RequestArg1,
          typename RequestArg2,
          typename RequestArg3,
          typename RequestArg4>
Call AddCall(const ServiceMethod& service_method,
             const Stub& stub,
             Response response,
             RetCode ret_code,
             const RequestArg1& request_arg1,
             const RequestArg2& request_arg2,
             const RequestArg3& request_arg3,
             const RequestArg4& request_arg4) {
    return cxx::bind(service_method,
                     stub,
                     cxx::ref(request_arg1),
                     cxx::ref(request_arg2),
                     cxx::ref(request_arg3),
                     cxx::ref(request_arg4),
                     ret_code,
                     response,
                     cxx::placeholders::_1,
                     cxx::placeholders::_2);
}

template <typename ServiceMethod,
          typename Stub,
          typename Response,
          typename RetCode,
          typename RequestArg1,
          typename RequestArg2,
          typename RequestArg3,
          typename RequestArg4,
          typename RequestArg5>
Call AddCall(const ServiceMethod& service_method,
             const Stub& stub,
             Response response,
             RetCode ret_code,
             const RequestArg1& request_arg1,
             const RequestArg2& request_arg2,
             const RequestArg3& request_arg3,
             const RequestArg4& request_arg4,
             const RequestArg5& request_arg5) {
    return cxx::bind(service_method,
                     stub,
                     cxx::ref(request_arg1),
                     cxx::ref(request_arg2),
                     cxx::ref(request_arg3),
                     cxx::ref(request_arg4),
                     cxx::ref(request_arg5),
                     ret_code,
                     response,
                     cxx::placeholders::_1,
                     cxx::placeholders::_2);
}

template <typename ServiceMethod,
          typename Stub,
          typename Response,
          typename RetCode,
          typename RequestArg1,
          typename RequestArg2,
          typename RequestArg3,
          typename RequestArg4,
          typename RequestArg5,
          typename RequestArg6>
Call AddCall(const ServiceMethod& service_method,
             const Stub& stub,
             Response response,
             RetCode ret_code,
             const RequestArg1& request_arg1,
             const RequestArg2& request_arg2,
             const RequestArg3& request_arg3,
             const RequestArg4& request_arg4,
             const RequestArg5& request_arg5,
             const RequestArg6& request_arg6) {
    return cxx::bind(service_method,
                     stub,
                     cxx::ref(request_arg1),
                     cxx::ref(request_arg2),
                     cxx::ref(request_arg3),
                     cxx::ref(request_arg4),
                     cxx::ref(request_arg5),
                     cxx::ref(request_arg6),
                     ret_code,
                     response,
                     cxx::placeholders::_1,
                     cxx::placeholders::_2);
}

template <typename ServiceMethod,
          typename Stub,
          typename Response,
          typename RetCode,
          typename RequestArg1,
          typename RequestArg2,
          typename RequestArg3,
          typename RequestArg4,
          typename RequestArg5,
          typename RequestArg6,
          typename RequestArg7>
Call AddCall(const ServiceMethod& service_method,
             const Stub& stub,
             Response response,
             RetCode ret_code,
             const RequestArg1& request_arg1,
             const RequestArg2& request_arg2,
             const RequestArg3& request_arg3,
             const RequestArg4& request_arg4,
             const RequestArg5& request_arg5,
             const RequestArg6& request_arg6,
             const RequestArg7& request_arg7) {
    return cxx::bind(service_method,
                     stub,
                     cxx::ref(request_arg1),
                     cxx::ref(request_arg2),
                     cxx::ref(request_arg3),
                     cxx::ref(request_arg4),
                     cxx::ref(request_arg5),
                     cxx::ref(request_arg6),
                     cxx::ref(request_arg7),
                     ret_code,
                     response,
                     cxx::placeholders::_1,
                     cxx::placeholders::_2);
}

template <typename ServiceMethod,
          typename Stub,
          typename Response,
          typename RetCode,
          typename RequestArg1,
          typename RequestArg2,
          typename RequestArg3,
          typename RequestArg4,
          typename RequestArg5,
          typename RequestArg6,
          typename RequestArg7,
          typename RequestArg8>
Call AddCall(const ServiceMethod& service_method,
             const Stub& stub,
             Response response,
             RetCode ret_code,
             const RequestArg1& request_arg1,
             const RequestArg2& request_arg2,
             const RequestArg3& request_arg3,
             const RequestArg4& request_arg4,
             const RequestArg5& request_arg5,
             const RequestArg6& request_arg6,
             const RequestArg7& request_arg7,
             const RequestArg8& request_arg8) {
    return cxx::bind(service_method,
                     stub,
                     cxx::ref(request_arg1),
                     cxx::ref(request_arg2),
                     cxx::ref(request_arg3),
                     cxx::ref(request_arg4),
                     cxx::ref(request_arg5),
                     cxx::ref(request_arg6),
                     cxx::ref(request_arg7),
                     cxx::ref(request_arg8),
                     ret_code,
                     response,
                     cxx::placeholders::_1,
                     cxx::placeholders::_2);
}

// WhenAll macro
#define WhenAllInit(num) \
    uint32_t num_parallel = num; \
    uint32_t num_called = num_parallel;

#define WhenAllCall(num) \
    f##num(&num_called, &num_parallel);

#define WhenAllCheck() \
    PLOG_IF_ERROR(num_called != 0, \
        "num_parallel: %u, num_called: %u", num_parallel, num_called);

void WhenAll(const std::vector<Call>& f_list);

// f1
void WhenAll(const Call& f1);

// f2
void WhenAll(const Call& f1,
             const Call& f2);

// f3
void WhenAll(const Call& f1,
             const Call& f2,
             const Call& f3);

// f4
void WhenAll(const Call& f1,
             const Call& f2,
             const Call& f3,
             const Call& f4);

// f5
void WhenAll(const Call& f1,
             const Call& f2,
             const Call& f3,
             const Call& f4,
             const Call& f5);

// f6
void WhenAll(const Call& f1,
             const Call& f2,
             const Call& f3,
             const Call& f4,
             const Call& f5,
             const Call& f6);

// f7
void WhenAll(const Call& f1,
             const Call& f2,
             const Call& f3,
             const Call& f4,
             const Call& f5,
             const Call& f6,
             const Call& f7);

// f8
void WhenAll(const Call& f1,
             const Call& f2,
             const Call& f3,
             const Call& f4,
             const Call& f5,
             const Call& f6,
             const Call& f7,
             const Call& f8);

} // namespace pebble
#endif // SRC_EXTENSION_WHEN_ALL_H
