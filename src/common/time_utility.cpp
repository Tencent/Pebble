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
#include <sys/time.h>
#include <time.h>

#include "common/time_utility.h"


namespace pebble {

int64_t TimeUtility::GetCurrentMS() {
    int64_t timestamp = GetCurrentUS();
    return timestamp / 1000;
}

int64_t TimeUtility::GetCurrentUS() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    int64_t timestamp = tv.tv_sec * 1000000 + tv.tv_usec;
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

const char* TimeUtility::GetStringTimeDetail() {
    static char buff[64] = {0};
    static struct timeval tv_now;
    static time_t now;
    static struct tm tm_now;
    static struct tm* p_tm_now;

    gettimeofday(&tv_now, NULL);
    now = (time_t)tv_now.tv_sec;
    p_tm_now = localtime_r(&now, &tm_now);

    snprintf(buff, sizeof(buff), "%04d-%02d-%02d %02d:%02d:%02d.%06d",
        1900 + p_tm_now->tm_year,
        p_tm_now->tm_mon + 1,
        p_tm_now->tm_mday,
        p_tm_now->tm_hour,
        p_tm_now->tm_min,
        p_tm_now->tm_sec,
        static_cast<int>(tv_now.tv_usec));

    return buff;
}

time_t TimeUtility::GetTimeStamp(const std::string &time) {
    tm tm_;
    char buf[128] = { 0 };
    strncpy(buf, time.c_str(), sizeof(buf)-1);
    buf[sizeof(buf) - 1] = 0;
    strptime(buf, "%Y-%m-%d %H:%M:%S", &tm_);
    tm_.tm_isdst = -1;
    return mktime(&tm_);
}

time_t  TimeUtility::GetTimeDiff(const std::string &t1, const std::string &t2) {
    time_t time1 = GetTimeStamp(t1);
    time_t time2 = GetTimeStamp(t2);
    time_t time = time1 - time2;
    return time;
}

} // namespace pebble


