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


#include "control_command.h"
#include "source/app/idl/control_service_ControlCommand_if.h"
#include "source/rpc/common/rpc_error_info.h"
#include "source/rpc/rpc.h"

namespace pebble {

// 同步调用控制命令，调用成功返回0。调用失败返回非零，
// 错误码参考pebble::rpc::ErrorInfo中各错误码定义
// 本函数运行返回0后，表示控制命令已在对端Server内被执行，
// 其运行结果将回填到输出参数result和description中。
// result的含义及规则由业务自行约定。
// 注意，只有本函数返回0后，result和description才有意义。
int ControlCommand::RunCommand(const std::string& service_url,
                               const std::string& command,
                               int* result,
                               std::string* description) {
    pebble::rpc::Rpc* rpc_client = pebble::rpc::Rpc::Instance();
    int ret = rpc_client->Init("", 0, "");
    if (ret != 0) {
        return ret;
    }
    // 当无法连接Server时，Client构造函数中会跑出异常，需要上层捕捉
    control_service::ControlCommandClient
        client(service_url, pebble::rpc::PROTOCOL_BINARY);
    control_service::ReqRunControlCommand req;
    control_service::ResRunControlCommand res;
    req.command = command;
    req.seq = 0;    // 同步调用不需要序列号
    ret = client.RunControlCommand(req, res);
    if (ret != 0) {
        return ret;
    }
    if (result) {
        *result = res.result;
    }
    if (description) {
        *description = res.description;
    }
    return 0;
}


} // namespace pebble


