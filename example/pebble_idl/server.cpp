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

#include "example/pebble_idl/idl_UserInfoManager.h"
#include "example/pebble_idl/idlex_UserInfoManagerEx.h"
#include "server/pebble_server.h"

#define ASSERT_EQ(expected, actual) \
    if ((expected) != (actual)) { \
        fprintf(stderr, "(%s:%d)(%s) %d != %d\n", __FILE__, __LINE__, __FUNCTION__, (expected), (actual)); \
        exit(1); \
    }

// UserInfoManagerEx服务接口的实现，UserInfoManagerEx派生了UserInfoManager，需要实现父服务的接口
class MyUserInfoManagerEx : public example_ex::UserInfoManagerExCobSvIf {
public:
    MyUserInfoManagerEx() : m_id(0) {}
    virtual ~MyUserInfoManagerEx() {}

    virtual void get_user(const uint64_t id,
        cxx::function<void(int32_t ret_code, const example::Message& response)>& rsp) {
        std::cout << "[server] " << __FUNCTION__ << std::endl;

        example::Message response;
        get_user(id, &response);
        // 调用rsp返回响应消息
        rsp(0, response);
    }

    virtual void add_user(const example::UserInfo& user, const std::string& comment,
        cxx::function<void(int32_t ret_code, uint64_t response)>& rsp) {
        std::cout << "[server] " << __FUNCTION__ << std::endl;

        User new_user;
        new_user._user    = user;
        new_user._comment = comment;
        m_users[m_id] = new_user;
        rsp(0, m_id);
        m_id++;
    }

    virtual void delete_user(const uint64_t id) {
        std::cout << "[server] " << __FUNCTION__ << std::endl;

        // oneway请求，不需要返回响应
        m_users.erase(id);
    }

    virtual void get_user_list(const std::vector<uint64_t> & ids,
        cxx::function<void(int32_t ret_code, const std::vector< ::example::Message> & response)>& rsp) {
        std::cout << "[server] " << __FUNCTION__ << std::endl;

        std::vector< ::example::Message> response;
        for (std::vector<uint64_t>::const_iterator it = ids.begin(); it != ids.end(); ++it) {
            example::Message tmp;
            get_user(*it, &tmp);
            response.push_back(tmp);
        }
        rsp(0, response);
    }

private:
    void get_user(uint64_t id, example::Message* response) {
        std::map<uint64_t, User>::iterator it = m_users.find(id);
        if (it == m_users.end()) {
            response->_result = example::NOT_FOUND;
        } else {
            response->_user = it->second._user;
            if (!it->second._comment.empty()) {
                // 对可选字段特殊处理
                response->__set__comment(it->second._comment);
                // 也可以这样处理
                // response->_comment = it->second._comment;
                // response->__isset._comment = true;
            }
        }
    }

private:
    struct User {
        example::UserInfo _user;
        std::string       _comment;
    };
    std::map<uint64_t, User> m_users;
    uint64_t                 m_id;
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
    MyUserInfoManagerEx service;

    ret = rpc->AddService(&service);
    ASSERT_EQ(0, ret);

    server.Serve();

    return 0;
}



