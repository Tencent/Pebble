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
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#include "example/pebble_idl/idl_UserInfoManager.h"
#include "example/pebble_idl/idlex_UserInfoManagerEx.h"
#include "server/pebble_server.h"


#define ASSERT_EQ(expected, actual) \
    if ((expected) != (actual)) { \
        fprintf(stderr, "(%s:%d)(%s) %d != %d\n", __FILE__, __LINE__, __FUNCTION__, (expected), (actual)); \
        exit(1); \
    }

// RPC请求示例，同步方式
void send_rpc_requests(example_ex::UserInfoManagerExClient* client) {
    // get
    example::Message result;
    int ret = client->get_user(999, &result);
    std::cout << "1. get user 999 : " << ret << " : " << result << std::endl << std::endl;

    // add
    uint64_t id = 0;
    example::UserInfo user;
    user.name = "tom";
    user.age  = 20;
    ret = client->add_user(user, "tom is a cat", &id);
    std::cout << "2. add user tom : " << ret << " : " << id << std::endl << std::endl;

    // add
    user.name = "jerry";
    user.age  = 5;
    ret = client->add_user(user, "jerry is a mouse", &id);
    std::cout << "3. add user jerry : " << ret << " : " << id << std::endl << std::endl;

    // get list
    std::vector<example::Message> results;
    std::vector<uint64_t> ids;
    ids.push_back(0);
    ids.push_back(1);
    ids.push_back(2);
    ret = client->get_user_list(ids, &results);
    std::cout << "4. get users " << " : " << ret << " : ";
    for (std::vector<example::Message>::iterator it = results.begin(); it != results.end(); ++it) {
        std::cout << *it << " | ";
    }
    std::cout << std::endl << std::endl;

    // delete
    ret = client->delete_user(0);
    std::cout << "5. delete user 0 : " << ret << std::endl << std::endl;

    // get list
    ret = client->get_user_list(ids, &results);
    std::cout << "6. get users " << " : " << ret << " : ";
    for (std::vector<example::Message>::iterator it = results.begin(); it != results.end(); ++it) {
        std::cout << *it << " | ";
    }
    std::cout << std::endl << std::endl;

    // over
    kill(getpid(), SIGUSR1);
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
    pebble::PebbleServer client;
    int ret = client.Init();
    ASSERT_EQ(0, ret);

    // 添加传输
    int64_t handle = client.Connect(url);
    ASSERT_EQ(true, handle >= 0);

    // 获取PebbleRpc实例
    pebble::PebbleRpc* rpc = client.GetPebbleRpc(pebble::kPEBBLE_RPC_BINARY);
    ASSERT_EQ(true, rpc != NULL);

    // 指定通道的处理器
    client.Attach(handle, rpc);

    // 创建RPC客户端
    example_ex::UserInfoManagerExClient rpc_client(rpc);
    rpc_client.SetHandle(handle);

    client.MakeCoroutine(cxx::bind(send_rpc_requests, &rpc_client));

    client.Serve();
}

