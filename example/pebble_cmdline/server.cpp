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
#include <stdio.h>
#include <stdlib.h>

#include "gflags/gflags.h"
#include "server/pebble_cmdline.h"
#include "server/pebble.h"


#define ASSERT_EQ(expected, actual) \
    if ((expected) != (actual)) { \
        fprintf(stderr, "(%s:%d)(%s) %d != %d\n", \
            __FILE__, __LINE__, __FUNCTION__, (expected), (actual)); \
        exit(1); \
    }

// 用户自定义命令行参数: 增加一行定义即可
DEFINE_string(port,  "8800", "service listen port.");

int main(int argc, char** argv) {
    // 在程序开始处处理命令行参数
    pebble::CmdLineHandler cmdline_handler;
    cmdline_handler.SetVersion("pebble cmdline test 0.1.0");
    int ret = cmdline_handler.Process(argc, argv);
    ASSERT_EQ(0, ret);

    // 获取用户自定义命令行参数
    std::cout << "cmd para port = " << FLAGS_port << std::endl;

    // 启动pebble server
    pebble::PebbleServer server;
    ret = server.Init();
    ASSERT_EQ(0, ret);

    server.Serve();

    return 0;
}



