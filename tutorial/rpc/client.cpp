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
#include <unistd.h>
#include "./source/rpc/rpc.h"
#include "./tutorial/rpc/tutorial_BaseService_if.h"

using namespace tutorial;

void heartbeat_cb(int ret, int64_t _return, HeartBeatInfo* heartbeat_info) {
    std::cout << "heartbeat ack, ret = " << ret << " ackid = " << _return
              << " the original heartbeat info is (" << *heartbeat_info << ")"
              << std::endl;
}

void usage() {
    std::cout << "usage   : ./client port" << std::endl
              << "default : port = 8200" << std::endl;
}

int main(int argc, char* argv[]) {
    usage();

    // 配置参数
    std::string service_url("tcp://127.0.0.1:");
    if (argc > 1) {
        service_url.append(argv[1]);
    } else {
        service_url.append("8200");
    }

    // 初始化RPC
    pebble::rpc::Rpc* rpc = pebble::rpc::Rpc::Instance();
    rpc->Init("", -1, "");

    // 创建rpc client stub
    BaseServiceClient client(service_url, pebble::rpc::PROTOCOL_BINARY);

    // 同步调用
    int ret = client.log("pebble simple test : log");
    std::cout << "sync call, ret = " << ret << std::endl;

    // 异步调用
    int64_t heartbeatid = 0;
    HeartBeatInfo heartbeatinfo;
    heartbeatinfo.address = "0.0.2";

    do {
        heartbeatinfo.id = ++heartbeatid;
        cxx::function<void(int, int64_t)> cb = cxx::bind(heartbeat_cb,
                                                         cxx::placeholders::_1,
                                                         cxx::placeholders::_2,
                                                         &heartbeatinfo);

        client.heartbeat(heartbeatid, heartbeatinfo, cb);

        rpc->Update();
        sleep(1);
    } while (1);

    return 0;
}

