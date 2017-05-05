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


#ifndef _PEBBLE_COMMON_MEMORY_H_
#define _PEBBLE_COMMON_MEMORY_H_


namespace pebble {

/// @brief 获取进程当前内存使用情况
/// @param vm_size_kb 输出参数，虚存，单位为K
/// @param rss_size_kb 输出参数，物理内存，单位为K
int GetCurMemoryUsage(int* vm_size_kb, int* rss_size_kb);

} // namespace pebble

#endif // _PEBBLE_COMMON_MEMORY_H_