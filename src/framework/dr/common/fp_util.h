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

#ifndef PEBBLE_DR_COMMON_FP_UTIL_H
#define PEBBLE_DR_COMMON_FP_UTIL_H

#include <limits>

namespace pebble { namespace dr { namespace floating_point {

template<size_t size>
class UIntWithSize
{
public:
    typedef void UInt;
};

template<>
class UIntWithSize<4>
{
public:
    typedef uint32_t UInt;
};

template<>
class UIntWithSize<8>
{
public:
    typedef uint64_t UInt;
};

template<typename IntType, typename FloatType>
inline IntType ReinterpretInt(FloatType from)
{
    union {FloatType _f; IntType _i;} val; // NOLINT
    val._f = from;
    return val._i;
}

// 将符号型整数归一化到无符号整数空间
// 32位整型 :
// negative integer [-2^31, -1]
//          convert from    unsigned integer [0x8000 0000, 0xFFFF FFFF]
//          to              unsigned integer [0x8000 0000, 0x0000 0001]
// positive integer [0, 2^31 - 1]
//          convert from    unsigned integer [0x0000 0000, 0x7FFF FFFF]
//          to              unsigned integer [0x8000 0000, 0xFFFF FFFF]
template<typename UIntType, size_t size>
inline UIntType SignToBiased(UIntType from)
{
    if ((static_cast<UIntType>(1) << (size - 1)) & from)
    {
        return (~from) + 1;
    }
    else
    {
        return (static_cast<UIntType>(1) << (size - 1)) | from;
    }
}

// 判断2个浮点数是否相等
// 如果2个浮点数的距离小于kMaxUlps则认为相等
// Notes：
//      INF和最大值相等
//      如果定义NAN_CAN_EQUAL，则NAN可以相等，否则一定不等
//      +0.0 和 -0.0的ULP为0，相等
//      kMaxUlps为4时，实际相等于5-6倍的FLT_EPSILON(DBL_EPSILON)
//      不判断NAN时速度比判断NAN快几十倍
template<typename FloatType>
bool AlmostEquals(FloatType lval, FloatType rval)
{
    typedef typename UIntWithSize<sizeof(FloatType)>::UInt UIntType;

    static const UIntType kMaxUlps = 4;

    UIntType luint = ReinterpretInt<UIntType, FloatType>(lval);
    UIntType ruint = ReinterpretInt<UIntType, FloatType>(rval);

#ifndef NAN_CAN_EQUAL
    static const size_t kBitCount = 8 * sizeof(FloatType);
    static const size_t kFractionBitCount = std::numeric_limits<FloatType>::digits - 1;
    static const size_t kExponentBitCount = kBitCount - kFractionBitCount - 1;
    static const UIntType kSignBitMask = static_cast<UIntType>(1) << (kBitCount - 1);
    static const UIntType kFractionBitMask = ~static_cast<UIntType>(0) >> (kExponentBitCount + 1);
    static const UIntType kExpoentBitMask = ~(kSignBitMask | kFractionBitMask);

#define CHECK_IS_INF(uint) \
    (kExpoentBitMask == (kExpoentBitMask & uint) && 0 == (kFractionBitMask & uint))
#define CHECK_IS_NAN(uint) \
    (kExpoentBitMask == (kExpoentBitMask & uint) && 0 != (kFractionBitMask & uint))

    if (CHECK_IS_NAN(luint) || CHECK_IS_NAN(ruint))
    {
        return false;
    }
#endif

    luint = SignToBiased<UIntType, 8 * sizeof(UIntType)>(luint);
    ruint = SignToBiased<UIntType, 8 * sizeof(UIntType)>(ruint);
    return ((luint > ruint) ? (luint < ruint + kMaxUlps) : (ruint < luint + kMaxUlps));
}

} // namespace floating_point
} // namespace dr
} // namespace pebble

#endif // PEBBLE_DR_COMMON_FP_UTIL_H

