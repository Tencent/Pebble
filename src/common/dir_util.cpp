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

#include <errno.h>
#include <linux/limits.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common/dir_util.h"
#include "common/error.h"


namespace pebble {

char DirUtil::m_last_error[256] = {0};

int DirUtil::MakeDir(const std::string& path) {
    if (path.empty()) {
        _LOG_LAST_ERROR("param path is NULL");
        return -1;
    }

    if (access(path.c_str(), F_OK | W_OK) == 0) {
        return 0;
    }

    if (mkdir(path.c_str(), 0755) != 0) {
        _LOG_LAST_ERROR("mkdir %s failed(%s)", path.c_str(), strerror(errno));
        return -1;
    }

    return 0;
}

int DirUtil::MakeDirP(const std::string& path) {
    if (path.empty()) {
        _LOG_LAST_ERROR("param path is NULL");
        return -1;
    }
    if (path.size() > PATH_MAX) {
        _LOG_LAST_ERROR("path length %ld > PATH_MAX(%d)", path.size(), PATH_MAX);
        return -1;
    }

    int len = path.length();
    char tmp[PATH_MAX] = {0};
    snprintf(tmp, sizeof(tmp), "%s", path.c_str());

    for (int i = 1; i < len; i++) {
        if (tmp[i] != '/') {
            continue;
        }

        tmp[i] = '\0';
        if (MakeDir(tmp) != 0) {
            return -1;
        }
        tmp[i] = '/';
    }

    return MakeDir(path);
}

} // namespace pebble
