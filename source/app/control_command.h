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


#ifndef _SOURCE_APP_CONTROL_COMMAND_H_
#define _SOURCE_APP_CONTROL_COMMAND_H_

#include <string>
#include <stdint.h>

namespace pebble {
namespace rpc {
    class Rpc;
} // namespace rpc
} // namespace pebble

namespace pebble {

// 命令控制客户端接口，目前仅提供阻塞方式。
// 同时因底层rpc_client实现需要，目前不支持多线程下使用。
class ControlCommand {
public:
    ControlCommand() {}
    ~ControlCommand() {}

    // 同步调用控制命令，调用成功返回0。调用失败返回非零，
    // 错误码参考pebble::rpc::ErrorInfo中各错误码定义。
    // 本函数运行返回0后，表示控制命令已在对端Server内被执行，
    // 其运行结果将回填到输出参数result和description中，
    // result的含义及规则由业务自行约定。注意，只有本函数返回0后，result和description才有意义。
    static int RunCommand(const std::string& service_url,
                          const std::string& command,
                          int* result,
                          std::string* description);

private:
    // static int DoRunCommand(const std::string& service_url,
    //                        const std::string& command,
    //                        int* result,
    //                        std::string* description);
};



} // namespace pebble


#endif // _SOURCE_APP_CONTROL_COMMAND_H_



