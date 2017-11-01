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


#ifndef _PEBBLE_COMMON_PLATFORM_H_
#define _PEBBLE_COMMON_PLATFORM_H_

/// @brief 此文件只包含平台及编译器差异信息

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <stdint.h>
#include <unistd.h>

// 源码依赖场景下，可以适配编译器版本
// 包依赖场景下，统一使用std::tr1，避免c++11编译器编译时pebble的头文件和lib不一致
#define BUILD_WITH_SRC 0

#if __cplusplus < 201103L || !(BUILD_WITH_SRC)
#include <tr1/functional>
#include <tr1/memory>
#include <tr1/unordered_map>
#include <tr1/tuple>
#else
#include <functional>
#include <memory>
#include <unordered_map>
#include <tuple>
#endif


#ifndef  INT8_MAX
#define  INT8_MAX   0x7f
#endif
#ifndef  INT8_MIN
#define  INT8_MIN   (-INT8_MAX - 1)
#endif
#ifndef  UINT8_MAX
#define  UINT8_MAX   (INT8_MAX * 2 + 1)
#endif
#ifndef  INT16_MAX
#define  INT16_MAX   0x7fff
#endif
#ifndef  INT16_MIN
#define  INT16_MIN   (-INT16_MAX - 1)
#endif
#ifndef  UINT16_MAX
#define  UINT16_MAX  0xffff
#endif
#ifndef  INT32_MAX
#define  INT32_MAX   0x7fffffffL
#endif
#ifndef  INT32_MIN
#define  INT32_MIN   (-INT32_MAX - 1L)
#endif
#ifndef  UINT32_MAX
#define  UINT32_MAX  0xffffffffUL
#endif
#ifndef  INT64_MAX
#define  INT64_MAX   0x7fffffffffffffffLL
#endif
#ifndef  INT64_MIN
#define  INT64_MIN   (-INT64_MAX - 1LL)
#endif
#ifndef  UINT64_MAX
#define  UINT64_MAX  0xffffffffffffffffULL
#endif

namespace pebble {
namespace stdcxx {

#if __cplusplus < 201103L || !(BUILD_WITH_SRC)

using ::std::tr1::function;
using ::std::tr1::bind;
using ::std::tr1::shared_ptr;
using ::std::tr1::weak_ptr;
using ::std::tr1::dynamic_pointer_cast;

namespace placeholders {
    using ::std::tr1::placeholders::_1;
    using ::std::tr1::placeholders::_2;
    using ::std::tr1::placeholders::_3;
    using ::std::tr1::placeholders::_4;
    using ::std::tr1::placeholders::_5;
    using ::std::tr1::placeholders::_6;
    using ::std::tr1::placeholders::_7;
} // namespace placeholders

using ::std::tr1::unordered_map;
using ::std::tr1::ref;
using ::std::tr1::tuple;
using ::std::tr1::make_tuple;

#else // c++ 11

using ::std::function;
using ::std::bind;
using ::std::shared_ptr;
using ::std::weak_ptr;
using ::std::dynamic_pointer_cast;

namespace placeholders {
    using ::std::placeholders::_1;
    using ::std::placeholders::_2;
    using ::std::placeholders::_3;
    using ::std::placeholders::_4;
    using ::std::placeholders::_5;
    using ::std::placeholders::_6;
    using ::std::placeholders::_7;
} // namespace placeholders

using ::std::unordered_map;
using ::std::ref;
using ::std::tuple;
using ::std::make_tuple;

#endif
} // namespace stdcxx
} // namespace pebble

namespace cxx = pebble::stdcxx;


#endif // _PEBBLE_COMMON_PLATFORM_H_
