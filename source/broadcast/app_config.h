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


#ifndef PEBBLE_BROADCAST_APP_CONFIG_H
#define PEBBLE_BROADCAST_APP_CONFIG_H
#include <string>
#include <stdint.h>

namespace pebble {
namespace broadcast {

/// @brief 应用配置，如果要使用全局广播的话需要配置应用参数\n
///      全局广播时会把频道、订阅者等数据存储到zk，依赖下面参数
class AppConfig {
public:
    int64_t game_id;
    std::string game_key;
    std::string app_id;
    std::string zk_host;
    int32_t timeout_ms;

    int32_t UnitId();
    int32_t ServerId();
    int32_t InstanceId();

    AppConfig();
    ~AppConfig();
};

}  // namespace broadcast
}  // namespace pebble

#endif  // PEBBLE_BROADCAST_APP_CONFIG_H

