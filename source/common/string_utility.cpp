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

#include "source/common/string_utility.h"

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

