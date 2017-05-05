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


#ifndef _PEBBLE_COMMON_BASE64_H_
#define _PEBBLE_COMMON_BASE64_H_

#include <string>

namespace pebble {


class Base64
{
public:
    Base64() {}
    virtual ~Base64() {}

    static bool Encode(const std::string& src, std::string* dst);

    static bool Decode(const std::string& src, std::string* dst);

private:
    // 根据在Base64编码表中的序号求得某个字符
    static inline char Base2Chr(unsigned char n);

    // 求得某个字符在Base64编码表中的序号
    static inline unsigned char Chr2Base(char c);

    inline static int Base64EncodeLen(int n);
    inline static int Base64DecodeLen(int n);
};

} // namespace pebble

#endif // _PEBBLE_COMMON_BASE64_H_

