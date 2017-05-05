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
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "common/memory.h"


namespace pebble {

int GetCurMemoryUsage(int* vm_size_kb, int* rss_size_kb) {
    if (!vm_size_kb || !rss_size_kb) {
        return -1;
    }

    char file_name[64] = { 0 };
    snprintf(file_name, sizeof(file_name), "/proc/%d/status", getpid());
    
    FILE* pid_status = fopen(file_name, "r");
    if (!pid_status) {
        return -1;
    }

    int ret = 0;
    char line[256] = { 0 };
    char tmp[32]   = { 0 };
    fseek(pid_status, 0, SEEK_SET);
    for (int i = 0; i < 16; i++) {
        if (fgets(line, sizeof(line), pid_status) == NULL) {
            ret = -2;
            break;
        }

        if (strstr(line, "VmSize") != NULL) {
            sscanf(line, "%s %d", tmp, vm_size_kb);
        } else if (strstr(line, "VmRSS") != NULL) {
            sscanf(line, "%s %d", tmp, rss_size_kb);
        }
    }

    fclose(pid_status);

    return ret;
}

} // namespace pebble
