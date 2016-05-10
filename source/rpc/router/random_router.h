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


#ifndef PEBBLE_RPC_RANDOM_ROUTER_H
#define PEBBLE_RPC_RANDOM_ROUTER_H

#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <vector>

#include "router.h"

namespace pebble {
    namespace rpc {


        class RandomRouter: public IRouter {
        public:
            RandomRouter() {}
            virtual ~RandomRouter() {}

            virtual std::string Route(uint64_t key, const std::vector<std::string>& address)
            {
                if (address.empty())
                {
                    return "";
                }

                int index = rand() % address.size(); // NOLINT
                return address[index];
            }

            virtual int Route(uint64_t key, int size)
            {
                if (0 == size)
                {
                    return -1;
                }

                int index = rand() % size; // NOLINT
                return index;
            }
        };

} // namespace rpc
} // namespace pebble


#endif
