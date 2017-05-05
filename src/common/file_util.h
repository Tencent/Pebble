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


#ifndef _PEBBLE_COMMON_FILE_UTIL_H_
#define _PEBBLE_COMMON_FILE_UTIL_H_

#include <stdio.h>
#include <string>
#include "common/platform.h"

namespace pebble {

std::string GetSelfName();

class RollUtil {
public:
    RollUtil();
    RollUtil(const std::string& path, const std::string& name);
    ~RollUtil();
    FILE* GetFile();
    int32_t SetFileName(const std::string& name);
    int32_t SetFilePath(const std::string& path);
    int32_t SetFileSize(uint32_t file_size);
    int32_t SetRollNum(uint32_t roll_num);
    const char* GetLastError() {
        return m_last_error;
    }

    void Close();
    void Flush();

private:
    void Roll();

private:
    char m_last_error[256];
    std::string m_path;
    std::string m_name;
    uint32_t    m_file_size;
    uint32_t    m_roll_num;
    FILE*       m_file;
};

} // namespace pebble

#endif // _PEBBLE_COMMON_FILE_UTIL_H_