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

#include "example/hello_world/hello_HelloWorld.h"

#include "gflags/gflags.h"
#include "server/pebble.h"

#define ASSERT_EQ(expected, actual) \
    if ((expected) != (actual)) { \
        fprintf(stderr, "(%s:%d)(%s) %d != %d\n", __FILE__, __LINE__, __FUNCTION__, (expected), (actual)); \
        exit(1); \
    }

// Calculator服务接口的实现，这个是示例RPC，用户可以添加自己的RPC服务
class HelloWorldService : public example::HelloWorldCobSvIf {
public:
    HelloWorldService() {}
    virtual ~HelloWorldService() {}

    virtual void hello(const std::string& hello,
        cxx::function<void(int32_t error_code, const std::string& _return)>& rsp)
    {
        std::cout << "receive rpc request: " << hello << std::endl;
        rsp(pebble::kRPC_SUCCESS, hello);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 自定义命令行参数
DEFINE_string(port, "8888", "service listen port.");


////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 程序事件处理
class MyAppEventHandler : public pebble::AppEventHandler {
    HelloWorldService* _service;
public:
    MyAppEventHandler() : _service(NULL) {}
    virtual ~MyAppEventHandler() { delete _service; }

    virtual int32_t OnInit(::pebble::PebbleServer* pebble_server) {
        // 业务初始化处理放到这里，成功返回0，失败返回<0

        // 下面为示例，请根据实际情况删除或修改--begin
        // 添加传输
        int64_t handle = pebble_server->Bind("tcp://0.0.0.0:" + FLAGS_port);
        ASSERT_EQ(true, handle >= 0);

        // 获取PebbleRpc实例
        pebble::PebbleRpc* rpc = pebble_server->GetPebbleRpc(pebble::kPEBBLE_RPC_BINARY);
        ASSERT_EQ(true, rpc != NULL);

        // 指定通道的处理器
        pebble_server->Attach(handle, rpc);

        // 添加服务
        _service = new HelloWorldService;
        int ret = rpc->AddService(_service);
        ASSERT_EQ(0, ret);
        // 上面为示例，请根据实际情况删除或修改--end


        // 添加其他初始化操作...
        return 0;
    }

    virtual int32_t OnStop() {
        // 程序退出事件处理，允许退出返回0，暂时不允许退出返回<0，返回<0后框架每Loop会再触发此函数
        return 0;
    }

    virtual int32_t OnUpdate() {
        // 框架每Loop触发调用此函数
        return 0;
    }

    virtual int32_t OnReload() {
        // 业务配置的reload处理放到这里，成功返回0，失败返回<0
        return 0;
    }

    virtual int32_t OnIdle() {
        // 程序空闲时触发调用此函数，返回<=0时，框架sleep 1ms，返回>0时不sleep
        return 0;
    }
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {

    // 命令行参数处理
    pebble::CmdLineHandler cmdline_handler;
    cmdline_handler.SetVersion("hello world server 0.1.0 build at " __TIME__ " " __DATE__);
    int ret = cmdline_handler.Process(argc, argv);
    ASSERT_EQ(0, ret);


    // 创建PebbleServer对象
    pebble::PebbleServer* server = new pebble::PebbleServer;

    // 首先load ini配置，配置文件可通过命令行指定，若不需要配置文件，可删除
    std::string cfg_file("./cfg/pebble.ini");
    if (cmdline_handler.ConfigFile() != NULL) {
        cfg_file.assign(cmdline_handler.ConfigFile());
    }
    ret = server->LoadOptionsFromIni(cfg_file);
    ASSERT_EQ(0, ret);

    // 初始化pebble server
    MyAppEventHandler* my_app_event_handler = new MyAppEventHandler;
    ret = server->Init(my_app_event_handler);
    ASSERT_EQ(0, ret);


    // 启动服务，进入主循环
    server->Serve();

    delete server;
    delete my_app_event_handler;

    return 0;
}



