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


#ifndef _COMMON_STRING_UTILITY_H_
#define _COMMON_STRING_UTILITY_H_

#include <string>
#include <vector>

namespace pebble {

class StringUtility {
public:
    static void Split(const std::string& str,
        const std::string& delim,
        std::vector<std::string>* result);

    // 判断字符串str是否是以prefix开头
    static bool StartsWith(const std::string& str, const std::string& prefix);

    static bool EndsWith(const std::string& str, const std::string& suffix);

    static std::string& Ltrim(std::string& str); // NOLINT

    static std::string& Rtrim(std::string& str); // NOLINT

    static std::string& Trim(std::string& str); // NOLINT

    static void Trim(std::vector<std::string>* str_list);

    // 子串替换
    static void string_replace(const std::string &sub_str1,
        const std::string &sub_str2, std::string *str);

    static void UrlEncode(const std::string& src_str, std::string* dst_str);

    static void UrlDecode(const std::string& src_str, std::string* dst_str);

    // 大小写转换
    static void ToUpper(std::string* str);

    static void ToLower(std::string* str);

    // 去头去尾
    static bool StripSuffix(std::string* str, const std::string& suffix);

    static bool StripPrefix(std::string* str, const std::string& prefix);

    // bin和hex转换
    static bool Hex2Bin(const char* hex_str, std::string* bin_str);

    static bool Bin2Hex(const char* bin_str, std::string* hex_str);
};

} // namespace pebble


#endif // _COMMON_STRING_UTILITY_H_
