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
#include "source/app/pebble_server.h"
#include "tutorial/helloworld/helloworld_Tutorial_if.h"


using namespace tutorial;


class TutorialHandler : public TutorialCobSvIf {
public:
    void helloworld(const std::string& who, cxx::function<void(std::string const& _return)> cob) {
        std::cout << who << " say helloworld!" << std::endl;
        cob("Hello " + who + "!");
    }
};

void usage() {
    std::cout << "usage   : ./server port" << std::endl
              << "default : port = 8300" << std::endl;
}

int main(int argc, char* argv[]) {
    usage();

    // 初始化PebbleServer
    pebble::PebbleServer pebble_server;
    int ret = pebble_server.Init(argc, argv, "cfg/pebble.ini");
    if (ret != 0) {
        std::cout << "game server init failed." << std::endl;
        return -1;
    }

    // 注册服务
    TutorialHandler service;
    pebble_server.RegisterService(&service);

    // 添加服务监听地址
    std::string listen_addr("http://0.0.0.0:");
    if (argc > 1) {
        listen_addr.append(argv[1]);
    } else {
        listen_addr.append("8300");
    }
    pebble_server.AddServiceManner(listen_addr, pebble::rpc::PROTOCOL_BINARY);

    // 启动server
    pebble_server.Start();

    return 0;
}

