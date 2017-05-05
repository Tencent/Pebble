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


#ifndef _PEBBLE_COMMON_DIR_UTIL_H_
#define _PEBBLE_COMMON_DIR_UTIL_H_

#include <string>

namespace pebble {

class DirUtil {
public:
    /// @brief 创建目录，仅在目录不存在时创建(同mkdir)
    /// @param path 要创建的目录
    /// @return 0成功，非0失败
    static int MakeDir(const std::string& path);

    /// @brief 创建多级目录，连同父目录一起创建(同mkdir -p)
    /// @param path 要创建的目录
    /// @return 0成功，非0失败
    static int MakeDirP(const std::string& path);

    static const char* GetLastError() {
        return m_last_error;
    }

private:
    static char m_last_error[256];
};

} // namespace pebble

#endif // _PEBBLE_COMMON_DIR_UTIL_H_

