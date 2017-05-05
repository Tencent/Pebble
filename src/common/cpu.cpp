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
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "common/cpu.h"


namespace pebble {


// /proc/pid/stat字段定义
struct pid_stat_fields {
    int64_t pid;
    char comm[PATH_MAX];
    char state;
    int64_t ppid;
    int64_t pgrp;
    int64_t session;
    int64_t tty_nr;
    int64_t tpgid;
    int64_t flags;
    int64_t minflt;
    int64_t cminflt;
    int64_t majflt;
    int64_t cmajflt;
    int64_t utime;
    int64_t stime;
    int64_t cutime;
    int64_t cstime;
    // ...
};

// /proc/stat/cpu信息字段定义
struct cpu_stat_fields {
    char cpu_label[16];
    int64_t user;
    int64_t nice;
    int64_t system;
    int64_t idle;
    int64_t iowait;
    int64_t irq;
    int64_t softirq;
    // ...
};

long long GetCurCpuTime() {
    char file_name[64] = { 0 };
    snprintf(file_name, sizeof(file_name), "/proc/%d/stat", getpid());
    
    FILE* pid_stat = fopen(file_name, "r");
    if (!pid_stat) {
        return -1;
    }

    pid_stat_fields result;
    int ret = fscanf(pid_stat, "%ld %s %c %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld",
        &result.pid, result.comm, &result.state, &result.ppid, &result.pgrp, &result.session,
        &result.tty_nr, &result.tpgid, &result.flags, &result.minflt, &result.cminflt,
        &result.majflt, &result.cmajflt, &result.utime, &result.stime, &result.cutime, &result.cstime);

    fclose(pid_stat);

    if (ret <= 0) {
        return -1;
    }

    return result.utime + result.stime + result.cutime + result.cstime;
}

long long GetTotalCpuTime() {
    char file_name[] = "/proc/stat";
    FILE* stat = fopen(file_name, "r");
    if (!stat) {
        return -1;
    }

    cpu_stat_fields result;
    int ret = fscanf(stat, "%s %ld %ld %ld %ld %ld %ld %ld",
        result.cpu_label, &result.user, &result.nice, &result.system, &result.idle,
        &result.iowait, &result.irq, &result.softirq);

    fclose(stat);

    if (ret <= 0) {
        return -1;
    }

    return result.user + result.nice + result.system + result.idle +
        result.iowait + result.irq + result.softirq;
}

float CalculateCurCpuUseage(long long cur_cpu_time_start, long long cur_cpu_time_stop,
    long long total_cpu_time_start, long long total_cpu_time_stop) {
    long long cpu_result = total_cpu_time_stop - total_cpu_time_start;
    if (cpu_result <= 0) {
        return 0;
    }

    return (sysconf(_SC_NPROCESSORS_ONLN) * 100.0f *(cur_cpu_time_stop - cur_cpu_time_start)) / cpu_result;
}


} // namespace pebble
