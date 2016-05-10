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
#include "source/common/string_utility.h"
#include "tutorial/pebble_server/tutorial_BaseService_if.h"

using namespace tutorial;
class BaseServiceHandler : public BaseServiceCobSvIf {
public:
    void heartbeat(const int64_t id, const HeartBeatInfo& data,
                   cxx::function<void(int64_t const& _return)> cob) {
        std::cout << "receive request : heartbeat(id = " << id << ", data = "
                  << data << ")" << std::endl;

        cob(id);
    }

    void log(const std::string& content) {
        std::cout << "receive request : log(" << content << ")" << std::endl;
    }
};

void usage() {
    std::cout << "usage   : ./server port" << std::endl
              << "default : port = 8100" << std::endl;
}

int main(int argc, char* argv[]) {
    usage();

    // 初始化PebbleServer
    pebble::PebbleServer pebble_server;
    pebble_server.Init(argc, argv, "cfg/pebble.ini");

    // 注册服务
    BaseServiceHandler base_service;
    pebble_server.RegisterService(&base_service);

    // 配置服务监听地址
    std::string listen_addr("tcp://127.0.0.1:");
    if (argc > 1) {
        listen_addr.append(argv[1]);
    } else {
        listen_addr.append("8100");
    }

    // 添加服务监听地址
    pebble_server.AddServiceManner(listen_addr, pebble::rpc::PROTOCOL_BINARY);

    // 启动server
    pebble_server.Start();

    return 0;
}

