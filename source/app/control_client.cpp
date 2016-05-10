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
#include <string>
#include "source/app/control_command.h"
#include "source/rpc/common/common.h"
#include "source/rpc/common/rpc_error_info.h"


int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("usage   : ./control_client service_url command\n");
        printf("example : ./control_client http://127.0.0.1:10001 Reload\n");
        printf("example : ./control_client http://127.0.0.1:10001 Reload config\n");
        return -1;
    }

    std::string service_url(argv[1]);
    std::string command_line(argv[2]);
    for (int i = 3; i < argc; ++i) {    // 附加命令参数
        command_line += " ";
        command_line += argv[i];
    }

    int result = -1;
    std::string description;
    int ret = pebble::ControlCommand::RunCommand(service_url, command_line,
                                                 &result, &description);
    if (ret == pebble::rpc::ErrorInfo::kRpcTimeoutError) {
        printf("RunCommand timeout.\n");
        return -1;
    } else if (ret != 0) {
        printf("RunCommand failed, ret:%d\n", ret);
        return -1;
    }

    printf("Result:%d\nDescription:%s\n", result, description.c_str());
    return 0;
}

