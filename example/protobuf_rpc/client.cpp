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

#include "client/pebble_client.h"
#include "common/string_utility.h"
#include "example/protobuf_rpc/calculator.rpc.pb.h"

// 异步回调
void cb_add(::example::CalRequest request, int ret, const ::example::CalResponse& response) {
    std::cout << "receive rpc response: ret = " << ret << ", "
        << "error_msg = " << pebble::GetErrorString(ret) << ", "
        << request.a() << " + " << request.b() << " = " << response.c() << std::endl;
}

void usage() {
    std::cout << "usage   : ./client url" << std::endl
              << "default : url = tcp://127.0.0.1:9000" << std::endl;
}

int main(int argc,  char* argv[]) {
    usage();

    std::string url("tcp://127.0.0.1:9000");
    if (argc > 1) url.assign(argv[1]);

    // 初始化PebbleClient
    pebble::PebbleClient client;
    int ret = client.Init();
    if (ret != 0) {
        std::cout << "PebbleClient init failed, please check the detail info in log" << std::endl;
        return -1;
    }

    // 创建rpc stub对象
    ::example::CalculatorClient* calculator_client =
        client.NewRpcClientByAddress< ::example::CalculatorClient>(url, pebble::kPEBBLE_RPC_PROTOBUF);
    if (calculator_client == NULL) {
        std::cout << "new CalculatorClient failed, please check the detail info in log" << std::endl;
        return -1;
    }

    // 主循环
    int32_t i = 0;
    do {
        client.Update();

        // 异步调用
        ::example::CalRequest request;
        request.set_a(i);
        request.set_b(i + 1);
        cxx::function<void(int, const ::example::CalResponse&)> add_cb =
            cxx::bind(cb_add, request, cxx::placeholders::_1, cxx::placeholders::_2);
        calculator_client->add(request, add_cb);
        i++;

        usleep(10000);
    } while (true);

    delete calculator_client;
}

