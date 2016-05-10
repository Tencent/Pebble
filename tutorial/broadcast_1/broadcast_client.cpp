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
#include "tutorial/broadcast_1/idl/broadcast_tutorial_BroadcastTest_if.h"
#include "tutorial/broadcast_1/idl/broadcast_tutorial_LoginService_if.h"

class BroadcastTestHandler : public broadcast::tutorial::BroadcastTestCobSvIf {
public:
    void broadcast(const std::string& content) {
        std::cout << "client " << name <<
            " receive a broadcast msg : (" << content << ")" << std::endl;
    }

    std::string name;
};

void usage() {
    std::cout << "usage   : ./broadcast_client name port" << std::endl
              << "default : name = client1 port = 8123" << std::endl;
}

int main(int argc, char* argv[]) {
    usage();

    // 初始化RPC
    pebble::rpc::Rpc* rpc = pebble::rpc::Rpc::Instance();
    if (rpc->Init(std::string(), -1, std::string()) != 0) {
        std::cout << "rpc client init failed" << std::endl;
        return -1;
    }

    std::string name(argv[1]);

    // 注册广播接收处理服务
    BroadcastTestHandler broadcasttest;
    broadcasttest.name = name;
    rpc->RegisterService(&broadcasttest);

    // 创建rpc client stub
    std::string service_url("tcp://127.0.0.1:");
    if (argc > 2) {
        service_url.append(argv[2]);
    } else {
        service_url.append("8123");
    }
    broadcast::tutorial::LoginServiceClient client(service_url, pebble::rpc::PROTOCOL_JSON);
    client.login(name);

    int wait = 0;
    do {
        rpc->Update();
        sleep(1);
    } while (wait++ < 5);

    return 0;
}

