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

#include "example/rollback_rpc/calculator_Calculator.h"
#include "example/rollback_rpc/calculator_PushService.h"
#include "server/pebble.h"

#define ASSERT_EQ(expected, actual) \
    if ((expected) != (actual)) { \
        fprintf(stderr, "(%s:%d)(%s) %d != %d\n", __FILE__, __LINE__, __FUNCTION__, (expected), (actual)); \
        exit(1); \
    }

int64_t g_last_handle = -1;
int32_t On1sTimeout(example::PushServiceClient* rollback_client) {
    if (rollback_client) {
        rollback_client->SetHandle(g_last_handle);
        rollback_client->push("[" + pebble::TimeUtility::GetStringTime() + "] rollback data");
    }
    return 0;
}

// Calculator服务接口的实现
class CalculatorHandler : public example::CalculatorCobSvIf {
public:
    pebble::PebbleServer* _server;
    CalculatorHandler() : _server(NULL) {}
    virtual ~CalculatorHandler() {}

    virtual void add(const int32_t a, const int32_t b,
        cxx::function<void(int32_t error_code, int32_t _return)>& rsp)
    {
        int32_t c = a + b;
        std::cout << "receive rpc request: " << a << " + " << b << " = " << c << std::endl;
        rsp(pebble::kRPC_SUCCESS, c);

        // 处理请求时记录请求的来源，用于反向RPC调用，不过注意这个handle是可能失效的
        g_last_handle = _server->GetLastMessageInfo()->_remote_handle;
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
    calculator._server = &server;

    ret = rpc->AddService(&calculator);
    ASSERT_EQ(0, ret);

    // 创建反向RPC的client实例
    example::PushServiceClient rollback_client(rpc);

    // 创建定时器，定时向客户端推消息
    server.GetTimer()->StartTimer(1000, cxx::bind(On1sTimeout, &rollback_client));

    server.Serve();

    return 0;
}



