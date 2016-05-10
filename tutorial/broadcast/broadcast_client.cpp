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


#include <iostream>
#include <string>
#include "source/rpc/rpc.h"
#include "source/log/log.h"
#include "tutorial/broadcast/idl/broadcast_tutorial_BroadcastTrigger_if.h"

void usage() {
    std::cout << "usage   : ./broadcast_client port" << std::endl
              << "default : port = 8123" << std::endl;
}

int main(int argc, char* argv[]) {
    usage();

    // 初始化RPC
    pebble::rpc::Rpc* rpc = pebble::rpc::Rpc::Instance();
    if (rpc->Init(std::string(), -1, std::string()) != 0) {
        std::cout << "rpc client init failed" << std::endl;
        return -1;
    }

    // 创建rpc client stub
    std::string service_url("tcp://127.0.0.1:");
    if (argc > 1) {
        service_url.append(argv[1]);
    } else {
        service_url.append("8123");
    }
    broadcast::tutorial::BroadcastTriggerClient client(service_url, pebble::rpc::PROTOCOL_JSON);

    int result = 0;
    // 触发3次广播
    client.start("broadcast 1", result);

    sleep(1);

    client.start("broadcast 2", result);

    sleep(1);

    client.start("broadcast 3", result);

    return 0;
}

