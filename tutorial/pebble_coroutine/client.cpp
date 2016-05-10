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
#include <sstream>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <unistd.h>

#include "source/common/time_utility.h"
#include "source/rpc/rpc.h"
#include "tutorial/pebble_coroutine/miniserver_BaseService_if.h"

/// @brief 获取一个指定长度的随机字符串
/// @param len 指定生成的字符串的长度
/// @return 以字符串形式返回生成的随机字符串
/// @note 字符串中每一个字符来自于ASCII表中32-126的可见字符
std::string GetRadnStr(int len) {
    unsigned int seed = time(NULL);
    std::stringstream ss;
    for (int i = 0; i < len; i++) {
        char ret = static_cast<char>(rand_r(&seed) % 95 + 32);
        ss << ret;
    }

    return ss.str();
}

/// @breif RPC请求的回调函数
/// @param ret 回调函数执行状态，0为成功
/// @param _return RPC请求的返回结果
void EchoCb(int ret, std::string _return) {
    std::cout << "[" << TimeUtility::GetStringTime() << "] SendMsg_cb:ret = ["
              << ret << "], echo msg = [" << _return << "]." << std::endl;
}

void usage() {
    std::cout << "usage   : ./client port" << std::endl
              << "default : port = 8050" << std::endl;
}

// main函数
int main(int argc, char** argv) {
    usage();

    // set default parameter
    int wait_second = 5;
    std::string app_id("300.2.1");
    std::string service_url("http://127.0.0.1:");
    if (argc > 1) {
        service_url.append(argv[1]);
    } else {
        service_url.append("8050");
    }

    // 创建rpc客户端
    pebble::rpc::Rpc* rpc_client = pebble::rpc::Rpc::Instance();
    if (rpc_client->Init(app_id, -1, std::string()) != 0) {
        std::cout << "test_client|main|rpc client init failed" << std::endl;
        return -1;
    }

    mini_server::BaseServiceClient client(service_url, pebble::rpc::PROTOCOL_JSON);
    mini_server::MsgInfo send_msg;
    time_t start_time, now_time;
    time(&start_time);
    do {
        cxx::function<void(int, std::string)> cb = cxx::bind(
                &EchoCb, cxx::placeholders::_1, cxx::placeholders::_2);
        time(&now_time);
        // 发送数据, 默认每五秒发送一次, 可以通过命令行指定
        if (difftime(now_time, start_time) >= wait_second) {
            send_msg.echo_id++;
            send_msg.text = GetRadnStr(10);
            send_msg.comment = std::string("wait for echo");

            client.Echo(send_msg, cb);
            std::cout << "[" << TimeUtility::GetStringTime() << "] send id = ["
                      << send_msg.echo_id << "], text = [" << send_msg.text
                      << "]." << std::endl;
            start_time = now_time;
        }

        // 接收数据, 接收是在主循环中每10ms一次进行
        rpc_client->Update();  // 接收数据
        usleep(10000);   // 10ms
    } while (1);

    return 0;
}
