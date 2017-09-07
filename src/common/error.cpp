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

#include "common/error.h"


namespace pebble {

// 避免全局变量构造、析构顺序问题
static cxx::unordered_map<int32_t, const char*> * g_error_string_map = NULL;
struct ErrorInfo {
    ErrorInfo() {
        g_error_string_map = &_error_info;
    }
    ~ErrorInfo() {
        g_error_string_map = NULL;
    }
    cxx::unordered_map<int32_t, const char*> _error_info;
};

const char* GetErrorString(int32_t error_code) {
    if (0 == error_code) return "no error";

    if (!g_error_string_map) {
        return "error module not inited.";
    }

    cxx::unordered_map<int32_t, const char*>::iterator it = g_error_string_map->find(error_code);
    if (it != g_error_string_map->end()) {
        return it->second;
    }

    return "unregister error description";
}

void SetErrorString(int32_t error_code, const char* error_string) {
    static ErrorInfo s_error_info;
    (*g_error_string_map)[error_code] = error_string;
}

} // namespace pebble

