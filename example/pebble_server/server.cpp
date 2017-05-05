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

#include <cstdio>
#include <iostream>

#include "example/pebble_server/calculator_Calculator.h"
#include "server/pebble_server.h"

#define ASSERT_EQ(expected, actual) \
    if ((expected) != (actual)) { \
        fprintf(stderr, "(%s:%d)(%s) %d != %d\n", __FILE__, __LINE__, __FUNCTION__, (expected), (actual)); \
        exit(1); \
    }

// Calculator服务接口的实现
class CalculatorHandler : public example::CalculatorCobSvIf {
public:
    CalculatorHandler() {}
    virtual ~CalculatorHandler() {}

    virtual void add(const int32_t a, const int32_t b,
        cxx::function<void(int32_t error_code, int32_t _return)>& rsp)
    {
        int32_t c = a + b;
        std::cout << "receive rpc request: " << a << " + " << b << " = " << c << std::endl;
        rsp(pebble::kRPC_SUCCESS, c);
    }
};

void usage() {
    std::cout << "usage   : ./server url" << std::endl
              << "default : url = tcp://127.0.0.1:9000" << std::endl;
}

int main(int argc, const char** argv) {
    usage();

    std::string url("tcp://127.0.0.1:9000");
    if (argc > 1) url.assign(argv[1]);

    // 初始化PebbleServer
    pebble::PebbleServer server;
    int ret = server.Init();
    ASSERT_EQ(0, ret);

    // 添加传输
    int64_t handle = server.Bind(url);
    ASSERT_EQ(true, handle >= 0);

    // 获取PebbleRpc实例
    pebble::PebbleRpc* rpc = server.GetPebbleRpc(pebble::kPEBBLE_RPC_BINARY);
    ASSERT_EQ(true, rpc != NULL);

    // 指定通道的处理器
    server.Attach(handle, rpc);

    // 添加服务
    CalculatorHandler calculator;

    ret = rpc->AddService(&calculator);
    ASSERT_EQ(0, ret);

    server.Serve();

    return 0;
}



