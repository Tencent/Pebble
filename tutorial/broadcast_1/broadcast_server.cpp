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
#include <map>
#include <string>
#include "source/app/broadcast_mgr.h"
#include "source/app/pebble_server.h"
#include "source/app/subscriber.h"
#include "tutorial/broadcast_1/idl/broadcast_tutorial_BroadcastTest_if.h"
#include "tutorial/broadcast_1/idl/broadcast_tutorial_LoginService_if.h"


class LoginServiceHandler : public broadcast::tutorial::LoginServiceCobSvIf {
public:
    LoginServiceHandler() : server(NULL) {}
    ~LoginServiceHandler() {
        std::map<std::string, pebble::Subscriber*>::iterator it;
        for (it = clients.begin(); it != clients.end(); ++it) {
            delete it->second;
            it->second = NULL;
        }
    }

    void login(const std::string& name) {
        std::cout << "[server] " << name << "login." << std::endl;

        // 收到login请求，把client加入到频道TestChannel
        pebble::Subscriber* subscriber = new pebble::Subscriber(server);
        server->GetBroadcastMgr()->JoinChannel("TestChannel", subscriber);
        clients[name] = subscriber;

        // 有用户加入后向TestChannel频道发送广播
        broadcast::tutorial::BroadcastTestClient
            client("broadcast://TestChannel", pebble::rpc::PROTOCOL_JSON);
        client.broadcast(name + " login.");
    }

    pebble::PebbleServer* server;
    std::map<std::string, pebble::Subscriber*> clients;
};

void usage() {
    std::cout << "usage   : ./broadcast_server port" << std::endl
              << "default : port = 8123" << std::endl;
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
    LoginServiceHandler login_service;
    login_service.server = &pebble_server;
    pebble_server.RegisterService(&login_service);

    // 添加服务监听地址
    std::string listen_addr("tcp://127.0.0.1:");
    if (argc > 1) {
        listen_addr.append(argv[1]);
    } else {
        listen_addr.append("8123");
    }
    pebble_server.AddServiceManner(listen_addr, pebble::rpc::PROTOCOL_JSON);

    // 打开频道TestChannel
    pebble_server.GetBroadcastMgr()->OpenLocalChannel("TestChannel");

    // 启动server
    pebble_server.Start();

    return 0;
}

