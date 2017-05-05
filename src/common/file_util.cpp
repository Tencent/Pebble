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

#include <linux/limits.h>
#include <sstream>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common/dir_util.h"
#include "common/file_util.h"
#include "common/error.h"

namespace pebble {

/// @brief 获取当前程序名
std::string GetSelfName() {
    char path[64]       = {0};
    char link[PATH_MAX] = {0};

    snprintf(path, sizeof(path), "/proc/%d/exe", getpid());
    readlink(path, link, sizeof(link));

    std::string filename(strrchr(link, '/') + 1);

    return filename;
}

RollUtil::RollUtil() : m_path("./log"), m_name(GetSelfName() + ".log") {
    m_last_error[0] = 0;
    m_file_size     = 10 * 1024 * 1024;
    m_roll_num      = 10;
    m_file          = NULL;
}

RollUtil::RollUtil(const std::string& path, const std::string& name)
    : m_path(path), m_name(name) {
    m_last_error[0] = 0;
    m_file_size     = 10 * 1024 * 1024;
    m_roll_num      = 10;
    m_file          = NULL;
}

RollUtil::~RollUtil() {
    if (m_file) {
        fclose(m_file);
    }
}

FILE* RollUtil::GetFile() {
    if (m_file != NULL) {
        long size = ftell(m_file);
        // ftell失败时重新滚动
        if (size < 0 || size > m_file_size) {
            Roll();
        }
    } else {
        if (DirUtil::MakeDirP(m_path) != 0) {
            _LOG_LAST_ERROR("mkdir %s failed %s", m_path.c_str(), DirUtil::GetLastError());
            return NULL;
        }

        std::string filename(m_path);
        filename.append("/");
        filename.append(m_name);
        m_file = fopen(filename.c_str(), "a+");
    }
    return m_file;
}

int32_t RollUtil::SetFileName(const std::string& name) {
    if (name.empty()) {
        _LOG_LAST_ERROR("name is empty");
        return -1;
    }
    m_name = name;
    return 0;
}

int32_t RollUtil::SetFilePath(const std::string& path) {
    if (path.empty()) {
        _LOG_LAST_ERROR("path is empty");
        return -1;
    }

    int32_t ret = DirUtil::MakeDirP(path);
    if (ret != 0) {
        _LOG_LAST_ERROR("mkdir %s failed %s", m_path.c_str(), DirUtil::GetLastError());
        return -1;
    }

    m_path.assign(path);
    return 0;
}

int32_t RollUtil::SetFileSize(uint32_t file_size) {
    if (0 == file_size) {
        _LOG_LAST_ERROR("file size = 0");
        return -1;
    }
    m_file_size = file_size;
    return 0;
}

int32_t RollUtil::SetRollNum(uint32_t roll_num) {
    if (0 == roll_num) {
        _LOG_LAST_ERROR("roll num = 0");
        return -1;
    }
    m_roll_num = roll_num;
    return 0;
}

void RollUtil::Roll() {
    if (m_file != NULL) {
        fclose(m_file);
    }

    uint32_t pos = 0;
    struct stat file_info;
    int64_t min_timestamp = INT64_MAX;
    for (uint32_t i = 1; i < m_roll_num; i++) {
        std::ostringstream oss;
        oss << m_path << "/" << m_name << "." << i;

        if (stat(oss.str().c_str(), &file_info) == 0) {
            if (file_info.st_mtime < min_timestamp) {
                min_timestamp = file_info.st_mtime;
                pos = i;
            }
        } else {
            pos = i;
            break;
        }
    }

    std::ostringstream oss;
    oss << m_path << "/" << m_name;
    std::string f0(oss.str());
    if (pos > 0) {
        oss << "." << pos;
        std::string fn(oss.str());
        remove(fn.c_str());             // rm fn
        rename(f0.c_str(), fn.c_str()); // mv f0 fn
    }
    m_file = fopen(f0.c_str(), "w+");
}

void RollUtil::Close() {
    if (m_file) {
        fclose(m_file);
        m_file = NULL;
    }
}

void RollUtil::Flush() {
    if (m_file) {
        fflush(m_file);
    }
}

} // namespace pebble

