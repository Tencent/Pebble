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


#include <stdlib.h>
#include <vector>
#include "source/broadcast/app_config.h"
#include "source/common/string_utility.h"


namespace pebble {
namespace broadcast {

AppConfig::AppConfig() {
    game_id = 0;
    timeout_ms = 5000;
}

AppConfig::~AppConfig() {
}

int32_t AppConfig::UnitId() {
    std::vector<std::string> vec;
    StringUtility::Split(app_id, ".", &vec);
    if (vec.size() != 3) {
        return -1;
    }

    return atoi(vec[0].c_str());
}

int32_t AppConfig::ServerId() {
    std::vector<std::string> vec;
    StringUtility::Split(app_id, ".", &vec);
    if (vec.size() != 3) {
        return -1;
    }

    return atoi(vec[1].c_str());
}

int32_t AppConfig::InstanceId() {
    std::vector<std::string> vec;
    StringUtility::Split(app_id, ".", &vec);
    if (vec.size() != 3) {
        return -1;
    }

    return atoi(vec[2].c_str());
}

} // namespace broadcast
}  // namespace pebble

