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
#include "tutorial/reverse_rpc/rerpc_BaseService_if.h"
#include "tutorial/reverse_rpc/rerpc_PushService_if.h"


using namespace rerpc;

class PushServiceHandler : public PushServiceCobSvIf {
public:
    void push_server_info(const ServerInfo& info,
        cxx::function<void(int32_t const& _return)> cob) {
        std::cout << "push_server_info : " << info << std::endl;
        cob(1);
    }

    void sync_time(const std::string& time) {
        std::cout << "sync time : " << time << std::endl;
    }
};

void heartbeat_cb(int ret, int64_t _return, HeartBeatInfo* heartbeat_info) {
    std::cout << "heartbeat ack, ret = " << ret << " ackid = " << _return
        << " the original heartbeat info is (" << *heartbeat_info << ")" << std::endl;
}

void usage() {
    std::cout << "usage   : ./rerpc_server port" << std::endl
              << "default : port = 8300" << std::endl;
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
        service_url.append("8300");
    }
    BaseServiceClient client(service_url, pebble::rpc::PROTOCOL_BINARY);

    // 注册反向rpc服务
    PushServiceHandler push_service;
    rpc->RegisterService(&push_service);

    // 同步调用
    int ret = client.log("pebble simple test : log");
    std::cout << "sync call, ret = " << ret << std::endl;

    // 异步调用
    int64_t heartbeatid = 0;
    HeartBeatInfo heartbeatinfo;
    heartbeatinfo.address = service_url;

    do {
        heartbeatinfo.id = ++heartbeatid;
        cxx::function<void(int, int64_t)> cb = cxx::bind(heartbeat_cb,
            cxx::placeholders::_1, cxx::placeholders::_2, &heartbeatinfo);

        client.heartbeat(heartbeatid, heartbeatinfo, cb);

        rpc->Update();
        sleep(1);
    } while (1);

    return 0;
}

