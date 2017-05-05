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


#ifndef _PEBBLE_COMMON_CPU_H_
#define _PEBBLE_COMMON_CPU_H_


namespace pebble {

/// @brief 获取当前进程的cpu使用时间
long long GetCurCpuTime();

/// @brief 获取整个系统的cpu使用时间
long long GetTotalCpuTime();

/// @brief 计算进程的cpu使用率
/// @param cur_cpu_time_start 当前进程cpu使用时间段-start
/// @param cur_cpu_time_stop 当前进程cpu使用时间段-stop
/// @param total_cpu_time_start 整个系统cpu使用时间段-start
/// @param total_cpu_time_start 整个系统cpu使用时间段-stop
float CalculateCurCpuUseage(long long cur_cpu_time_start, long long cur_cpu_time_stop,
    long long total_cpu_time_start, long long total_cpu_time_stop);

} // namespace pebble

#endif // _PEBBLE_COMMON_CPU_H_