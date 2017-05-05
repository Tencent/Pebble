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


#include <string.h>
#include <algorithm>

#include "common/string_utility.h"


namespace pebble {

void StringUtility::Split(const std::string& str,
    const std::string& delim,
    std::vector<std::string>* result) {
    if (str.empty()) {
        return;
    }
    if (delim[0] == '\0') {
        result->push_back(str);
        return;
    }

    size_t delim_length = delim.length();

    for (std::string::size_type begin_index = 0; begin_index < str.size();) {
        std::string::size_type end_index = str.find(delim, begin_index);
        if (end_index == std::string::npos) {
            result->push_back(str.substr(begin_index));
            return;
        }
        if (end_index > begin_index) {
            result->push_back(str.substr(begin_index, (end_index - begin_index)));
        }

        begin_index = end_index + delim_length;
    }
}

bool StringUtility::StartsWith(const std::string& str, const std::string& prefix) {
    if (prefix.length() > str.length()) {
        return false;
    }

    if (memcmp(str.c_str(), prefix.c_str(), prefix.length()) == 0) {
        return true;
    }

    return false;
}

bool StringUtility::EndsWith(const std::string& str, const std::string& suffix) {
    if (suffix.length() > str.length()) {
        return false;
    }

    return (str.substr(str.length() - suffix.length()) == suffix);
}

std::string& StringUtility::Ltrim(std::string& str) { // NOLINT
    std::string::iterator it = find_if(str.begin(), str.end(), std::not1(std::ptr_fun(::isspace)));
    str.erase(str.begin(), it);
    return str;
}

std::string& StringUtility::Rtrim(std::string& str) { // NOLINT
    std::string::reverse_iterator it = find_if(str.rbegin(),
        str.rend(), std::not1(std::ptr_fun(::isspace)));

    str.erase(it.base(), str.end());
    return str;
}

std::string& StringUtility::Trim(std::string& str) { // NOLINT
    return Rtrim(Ltrim(str));
}

void StringUtility::Trim(std::vector<std::string>* str_list) {
    if (NULL == str_list) {
        return;
    }

    std::vector<std::string>::iterator it;
    for (it = str_list->begin(); it != str_list->end(); ++it) {
        *it = Trim(*it);
    }
}

void StringUtility::string_replace(const std::string &sub_str1,
    const std::string &sub_str2, std::string *str) {
    std::string::size_type pos = 0;
    std::string::size_type a = sub_str1.size();
    std::string::size_type b = sub_str2.size();
    while ((pos = str->find(sub_str1, pos)) != std::string::npos) {
        str->replace(pos, a, sub_str2);
        pos += b;
    }
}

static const char ENCODECHARS[1024] = {
    3, '%', '0', '0', 3, '%', '0', '1', 3, '%', '0', '2', 3, '%', '0', '3',
    3, '%', '0', '4', 3, '%', '0', '5', 3, '%', '0', '6', 3, '%', '0', '7',
    3, '%', '0', '8', 3, '%', '0', '9', 3, '%', '0', 'A', 3, '%', '0', 'B',
    3, '%', '0', 'C', 3, '%', '0', 'D', 3, '%', '0', 'E', 3, '%', '0', 'F',
    3, '%', '1', '0', 3, '%', '1', '1', 3, '%', '1', '2', 3, '%', '1', '3',
    3, '%', '1', '4', 3, '%', '1', '5', 3, '%', '1', '6', 3, '%', '1', '7',
    3, '%', '1', '8', 3, '%', '1', '9', 3, '%', '1', 'A', 3, '%', '1', 'B',
    3, '%', '1', 'C', 3, '%', '1', 'D', 3, '%', '1', 'E', 3, '%', '1', 'F',
    1, '+', '2', '0', 3, '%', '2', '1', 3, '%', '2', '2', 3, '%', '2', '3',
    3, '%', '2', '4', 3, '%', '2', '5', 3, '%', '2', '6', 3, '%', '2', '7',
    3, '%', '2', '8', 3, '%', '2', '9', 3, '%', '2', 'A', 3, '%', '2', 'B',
    3, '%', '2', 'C', 1, '-', '2', 'D', 1, '.', '2', 'E', 3, '%', '2', 'F',
    1, '0', '3', '0', 1, '1', '3', '1', 1, '2', '3', '2', 1, '3', '3', '3',
    1, '4', '3', '4', 1, '5', '3', '5', 1, '6', '3', '6', 1, '7', '3', '7',
    1, '8', '3', '8', 1, '9', '3', '9', 3, '%', '3', 'A', 3, '%', '3', 'B',
    3, '%', '3', 'C', 3, '%', '3', 'D', 3, '%', '3', 'E', 3, '%', '3', 'F',
    3, '%', '4', '0', 1, 'A', '4', '1', 1, 'B', '4', '2', 1, 'C', '4', '3',
    1, 'D', '4', '4', 1, 'E', '4', '5', 1, 'F', '4', '6', 1, 'G', '4', '7',
    1, 'H', '4', '8', 1, 'I', '4', '9', 1, 'J', '4', 'A', 1, 'K', '4', 'B',
    1, 'L', '4', 'C', 1, 'M', '4', 'D', 1, 'N', '4', 'E', 1, 'O', '4', 'F',
    1, 'P', '5', '0', 1, 'Q', '5', '1', 1, 'R', '5', '2', 1, 'S', '5', '3',
    1, 'T', '5', '4', 1, 'U', '5', '5', 1, 'V', '5', '6', 1, 'W', '5', '7',
    1, 'X', '5', '8', 1, 'Y', '5', '9', 1, 'Z', '5', 'A', 3, '%', '5', 'B',
    3, '%', '5', 'C', 3, '%', '5', 'D', 3, '%', '5', 'E', 1, '_', '5', 'F',
    3, '%', '6', '0', 1, 'a', '6', '1', 1, 'b', '6', '2', 1, 'c', '6', '3',
    1, 'd', '6', '4', 1, 'e', '6', '5', 1, 'f', '6', '6', 1, 'g', '6', '7',
    1, 'h', '6', '8', 1, 'i', '6', '9', 1, 'j', '6', 'A', 1, 'k', '6', 'B',
    1, 'l', '6', 'C', 1, 'm', '6', 'D', 1, 'n', '6', 'E', 1, 'o', '6', 'F',
    1, 'p', '7', '0', 1, 'q', '7', '1', 1, 'r', '7', '2', 1, 's', '7', '3',
    1, 't', '7', '4', 1, 'u', '7', '5', 1, 'v', '7', '6', 1, 'w', '7', '7',
    1, 'x', '7', '8', 1, 'y', '7', '9', 1, 'z', '7', 'A', 3, '%', '7', 'B',
    3, '%', '7', 'C', 3, '%', '7', 'D', 1, '~', '7', 'E', 3, '%', '7', 'F',
    3, '%', '8', '0', 3, '%', '8', '1', 3, '%', '8', '2', 3, '%', '8', '3',
    3, '%', '8', '4', 3, '%', '8', '5', 3, '%', '8', '6', 3, '%', '8', '7',
    3, '%', '8', '8', 3, '%', '8', '9', 3, '%', '8', 'A', 3, '%', '8', 'B',
    3, '%', '8', 'C', 3, '%', '8', 'D', 3, '%', '8', 'E', 3, '%', '8', 'F',
    3, '%', '9', '0', 3, '%', '9', '1', 3, '%', '9', '2', 3, '%', '9', '3',
    3, '%', '9', '4', 3, '%', '9', '5', 3, '%', '9', '6', 3, '%', '9', '7',
    3, '%', '9', '8', 3, '%', '9', '9', 3, '%', '9', 'A', 3, '%', '9', 'B',
    3, '%', '9', 'C', 3, '%', '9', 'D', 3, '%', '9', 'E', 3, '%', '9', 'F',
    3, '%', 'A', '0', 3, '%', 'A', '1', 3, '%', 'A', '2', 3, '%', 'A', '3',
    3, '%', 'A', '4', 3, '%', 'A', '5', 3, '%', 'A', '6', 3, '%', 'A', '7',
    3, '%', 'A', '8', 3, '%', 'A', '9', 3, '%', 'A', 'A', 3, '%', 'A', 'B',
    3, '%', 'A', 'C', 3, '%', 'A', 'D', 3, '%', 'A', 'E', 3, '%', 'A', 'F',
    3, '%', 'B', '0', 3, '%', 'B', '1', 3, '%', 'B', '2', 3, '%', 'B', '3',
    3, '%', 'B', '4', 3, '%', 'B', '5', 3, '%', 'B', '6', 3, '%', 'B', '7',
    3, '%', 'B', '8', 3, '%', 'B', '9', 3, '%', 'B', 'A', 3, '%', 'B', 'B',
    3, '%', 'B', 'C', 3, '%', 'B', 'D', 3, '%', 'B', 'E', 3, '%', 'B', 'F',
    3, '%', 'C', '0', 3, '%', 'C', '1', 3, '%', 'C', '2', 3, '%', 'C', '3',
    3, '%', 'C', '4', 3, '%', 'C', '5', 3, '%', 'C', '6', 3, '%', 'C', '7',
    3, '%', 'C', '8', 3, '%', 'C', '9', 3, '%', 'C', 'A', 3, '%', 'C', 'B',
    3, '%', 'C', 'C', 3, '%', 'C', 'D', 3, '%', 'C', 'E', 3, '%', 'C', 'F',
    3, '%', 'D', '0', 3, '%', 'D', '1', 3, '%', 'D', '2', 3, '%', 'D', '3',
    3, '%', 'D', '4', 3, '%', 'D', '5', 3, '%', 'D', '6', 3, '%', 'D', '7',
    3, '%', 'D', '8', 3, '%', 'D', '9', 3, '%', 'D', 'A', 3, '%', 'D', 'B',
    3, '%', 'D', 'C', 3, '%', 'D', 'D', 3, '%', 'D', 'E', 3, '%', 'D', 'F',
    3, '%', 'E', '0', 3, '%', 'E', '1', 3, '%', 'E', '2', 3, '%', 'E', '3',
    3, '%', 'E', '4', 3, '%', 'E', '5', 3, '%', 'E', '6', 3, '%', 'E', '7',
    3, '%', 'E', '8', 3, '%', 'E', '9', 3, '%', 'E', 'A', 3, '%', 'E', 'B',
    3, '%', 'E', 'C', 3, '%', 'E', 'D', 3, '%', 'E', 'E', 3, '%', 'E', 'F',
    3, '%', 'F', '0', 3, '%', 'F', '1', 3, '%', 'F', '2', 3, '%', 'F', '3',
    3, '%', 'F', '4', 3, '%', 'F', '5', 3, '%', 'F', '6', 3, '%', 'F', '7',
    3, '%', 'F', '8', 3, '%', 'F', '9', 3, '%', 'F', 'A', 3, '%', 'F', 'B',
    3, '%', 'F', 'C', 3, '%', 'F', 'D', 3, '%', 'F', 'E', 3, '%', 'F', 'F',
};

void StringUtility::UrlEncode(const std::string& src_str, std::string* dst_str)
{
    dst_str->clear();
    for (size_t i = 0; i < src_str.length() ; i++) {
        unsigned short offset = static_cast<unsigned short>(src_str[i]) * 4;
        dst_str->append((ENCODECHARS + offset + 1), ENCODECHARS[offset]);
    }
}

static const char HEX2DEC[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    0 ,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

void StringUtility::UrlDecode(const std::string& src_str, std::string* dst_str)
{
    dst_str->clear();
    const unsigned char* src_begin = reinterpret_cast<const unsigned char*>(src_str.data());
    const unsigned char* src_end = src_begin + src_str.length();
    const unsigned char* src_last = src_end - 2;
    while (src_begin < src_last) {
        if ((*src_begin) == '%') {
            char dec1, dec2;
            if (-1 != (dec1 = HEX2DEC[*(src_begin + 1)])
                && -1 != (dec2 = HEX2DEC[*(src_begin + 2)])) {
                dst_str->append(1, (dec1 << 4) + dec2);
                src_begin += 3;
                continue;
            }
        } else if ((*src_begin) == '+') {
            dst_str->append(1, ' ');
            ++src_begin;
            continue;
        }
        dst_str->append(1, static_cast<char>(*src_begin));
        ++src_begin;
    }
    while (src_begin < src_end) {
        dst_str->append(1, static_cast<char>(*src_begin));
        ++src_begin;
    }
}

void StringUtility::ToUpper(std::string* str)
{
    std::transform(str->begin(), str->end(), str->begin(), ::toupper);
}

void StringUtility::ToLower(std::string* str)
{
    std::transform(str->begin(), str->end(), str->begin(), ::tolower);
}

bool StringUtility::StripSuffix(std::string* str, const std::string& suffix) {
    if (str->length() >= suffix.length()) {
        size_t suffix_pos = str->length() - suffix.length();
        if (str->compare(suffix_pos, std::string::npos, suffix) == 0) {
            str->resize(str->size() - suffix.size());
            return true;
        }
    }

    return false;
}

bool StringUtility::StripPrefix(std::string* str, const std::string& prefix) {
    if (str->length() >= prefix.length()) {
        if (str->substr(0, prefix.size()) == prefix) {
            *str = str->substr(prefix.size());
            return true;
        }
    }
    return false;
}


} // namespace pebble

