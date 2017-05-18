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


#ifndef _PEBBLE_FRAMEWORK_REGISTER_ERROR_H_
#define _PEBBLE_FRAMEWORK_REGISTER_ERROR_H_


namespace pebble {

/// @brief 注册framework、common库的错误描述
///      * 若单独使用common库，需要自己注册common库的错误描述
///      * 对于扩展模块，在安装时注册
void RegisterErrorString();

} // namespace pebble

#endif // _PEBBLE_FRAMEWORK_REGISTER_ERROR_H_