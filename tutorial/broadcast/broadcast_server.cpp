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
#include "source/app/broadcast_mgr.h"
#include "source/app/pebble_server.h"
#include "tutorial/broadcast/idl/broadcast_tutorial_BroadcastTest_if.h"
#include "tutorial/broadcast/idl/broadcast_tutorial_BroadcastTrigger_if.h"


class BroadcastTriggerHandler : public broadcast::tutorial::BroadcastTriggerCobSvIf {
public:
    void start(const std::string& content, cxx::function<void(int32_t const& _return)> cob) {
        std::cout << "server " << serverid <<
            "receive a start broadcast request : (" << content << ")" << std::endl;
        cob(1);

        // 收到广播触发命令向TestChannel频道广播消息
        broadcast::tutorial::BroadcastTestClient
            client("broadcast://TestChannel", pebble::rpc::PROTOCOL_BINARY);
        client.broadcast(content);
    }

    std::string serverid;
};

class BroadcastTestHandler : public broadcast::tutorial::BroadcastTestCobSvIf {
public:
    void broadcast(const std::string& content) {
        std::cout << "server " << serverid <<
            " receive a broadcast msg : (" << content << ")" << std::endl;
    }

    std::string serverid;
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
    BroadcastTriggerHandler broadcast_trigger;
    BroadcastTestHandler broadcast_test;

    broadcast_trigger.serverid = pebble_server.GetConfig("tapp", "id", "");
    broadcast_test.serverid = pebble_server.GetConfig("tapp", "id", "");

    pebble_server.RegisterService(&broadcast_trigger);
    pebble_server.RegisterService(&broadcast_test);

    // 添加服务监听地址
    std::string listen_addr("tcp://127.0.0.1:");
    if (argc > 1) {
        listen_addr.append(argv[1]);
    } else {
        listen_addr.append("8123");
    }
    pebble_server.AddServiceManner(listen_addr, pebble::rpc::PROTOCOL_JSON);

    // 打开频道TestChannel
    pebble_server.GetBroadcastMgr()->OpenGlobalChannel("TestChannel");

    // 启动server
    pebble_server.Start();

    return 0;
}

