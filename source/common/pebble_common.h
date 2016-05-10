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


#ifndef PEBBLE_COMMON_H
#define PEBBLE_COMMON_H

#include <tr1/functional>
#include <tr1/memory>

namespace pebble { namespace stdcxx {
#if __cplusplus < 201103L
    using ::std::tr1::function;
    using ::std::tr1::bind;
    using ::std::tr1::shared_ptr;
    using ::std::tr1::dynamic_pointer_cast;

    namespace placeholders {
        using ::std::tr1::placeholders::_1;
        using ::std::tr1::placeholders::_2;
        using ::std::tr1::placeholders::_3;
        using ::std::tr1::placeholders::_4;
        using ::std::tr1::placeholders::_5;
        using ::std::tr1::placeholders::_6;
        using ::std::tr1::placeholders::_7;
    } // pebble::rpc::stdcxx::placeholders
#else // c++ 11
    using ::std::function;
    using ::std::bind;
    using ::std::shared_ptr;
    using ::std::dynamic_pointer_cast;

    namespace placeholders {
        using ::std::placeholders::_1;
        using ::std::placeholders::_2;
        using ::std::placeholders::_3;
        using ::std::placeholders::_4;
        using ::std::placeholders::_5;
        using ::std::placeholders::_6;
        using ::std::placeholders::_7;
    } // pebble::rpc::stdcxx::placeholders
#endif
} // namespace stdcxx
} // namespace pebble
namespace cxx = pebble::stdcxx;


#endif // PEBBLE_COMMON_H
