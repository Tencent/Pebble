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


#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "time_utility.h"




int64_t TimeUtility::GetCurremtMs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    int64_t timestamp = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    return timestamp;
}

std::string TimeUtility::GetStringTime()
{
    time_t now = time(NULL);

    struct tm tm_now;
    struct tm* p_tm_now;

    p_tm_now = localtime_r(&now, &tm_now);

    char buff[256] = {0};
    snprintf(buff, sizeof(buff), "%04d-%02d-%02d% 02d:%02d:%02d",
        1900 + p_tm_now->tm_year,
        p_tm_now->tm_mon + 1,
        p_tm_now->tm_mday,
        p_tm_now->tm_hour,
        p_tm_now->tm_min,
        p_tm_now->tm_sec);

    return std::string(buff);
}

time_t TimeUtility::ChangeTime(const std::string &time) {
    tm tm_;
    char buf[128] = { 0 };
    strncpy(buf, time.c_str(), sizeof(buf)-1);
    buf[sizeof(buf) - 1] = 0;
    strptime(buf, "%Y-%m-%d %H:%M:%S", &tm_);
    tm_.tm_isdst = -1;
    return mktime(&tm_);
}

time_t  TimeUtility::GetTimeDiff(const std::string &t1, const std::string &t2) {
    time_t time1 = ChangeTime(t1);
    time_t time2 = ChangeTime(t2);
    time_t time = time1 - time2;
    return time;
}



