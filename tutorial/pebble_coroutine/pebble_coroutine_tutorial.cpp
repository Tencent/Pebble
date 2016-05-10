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


#include <assert.h>
#include <iostream>
#include <map>
#include <sstream>
#include <stdio.h>
#include <time.h>

#include "source/app/pebble_server.h"
#include "source/common/coroutine.h"
#include "tutorial/pebble_coroutine/miniserver_BaseService_if.h"

// 记录Yield的协程id和其对应的开始时间，Resume的时候会删除对应节点
std::map<int64_t, time_t> g_map;

/// @brief 类：BaseServiceHandler RPC server的请求处理类
///
/// 实现IDL语言中定义的方法Echo
class BaseServiceHandler : public mini_server::BaseServiceCobSvIf {
public:
    /// @brief 构造函数
    /// @param pebble_server PebbleServer的指针
    /// @note 需要提供pebble_server，从而才能获取协程ID和对协程Yield
    explicit BaseServiceHandler(pebble::PebbleServer *pebble_server) {
        pebble_server_ = pebble_server;
    }

    /// @brief Echo 请求处理函数
    /// @param msg client发送来的结构体类型的数据
    /// @param cob client收到应答后的回调函数
    /// @note 协程在该函数中Yield
    void Echo(const mini_server::MsgInfo& msg,
              cxx::function<void(std::string const& _return)> cob) {
        std::cout << "receive request: MsgInfo.echo_id = [" << msg.echo_id
                  << "], MsgInfo.text = [" << msg.text
                  << "], MsgInfo.comment = [" << msg.comment << "]"
                  << std::endl;

        // 1. 获取协程 ID
        int64_t coroutine_id = pebble_server_->GetCoroutineSchedule()
                ->CurrentTaskId();
        // 2. 保存协程 ID 用以恢复
        time_t now_time;
        time(&now_time);
        g_map[coroutine_id] = now_time;

        // 3. 协程Yield
        std::cout << "--Yield--, coroutine Id:" << coroutine_id << std::endl;
        pebble_server_->GetCoroutineSchedule()->Yield();
        // 4. echo 回声不能瞬间听到，需要等一段时间后才能返回声音的起点
        std::stringstream ss_echo;
        ss_echo << "coroutine id:" << coroutine_id << " + echo_id:"
                << msg.echo_id << " + text:" << msg.text;
        cob(ss_echo.str());
    }

private:
    pebble::PebbleServer *pebble_server_;
};

/// @brief 类：RpcUpdater 添加到PebbleServer中的updater
///
/// 需要实现Update方法
class RpcUpdater : public pebble::UpdaterInterface {
public:
    /// @brief 此方法每个主循环都被调用
    virtual int Update(pebble::PebbleServer* pebble_server) {
        // 1. 遍历map, 对历时三秒的 echo 进行 resume
        std::map<int64_t, time_t>::iterator it;
        for (it = g_map.begin(); it != g_map.end(); it++) {
            if (IsTimeToResume(it->second)) {
                std::cout << "--Resume--, coroutine Id:" << it->first
                          << std::endl;
                pebble_server->GetCoroutineSchedule()->Resume(it->first);
                g_map.erase(it->first);
            }
        }

        return 0;
    }

private:
    bool IsTimeToResume(time_t start_time) {
        time_t now;
        time(&now);
        if (difftime(now, start_time) >= 3) {
            return true;
        } else {
            return false;
        }
    }
};

void usage() {
    std::cout << "usage   : ./server port" << std::endl
              << "default : port = 8050" << std::endl;
}

// main函数
int main(int argc, char** argv) {
    usage();

    // 1. 初始化 pebble server
    pebble::PebbleServer pebble_server;
    int ret = pebble_server.Init(argc, argv, "cfg/pebble.ini");
    if (ret != 0) {
        std::cout << "pebble server init failed." << std::endl;
        return -1;
    }

    // 2. add manner
    std::string listen_addr("http://127.0.0.1:");
    if (argc > 1) {
        listen_addr.append(argv[1]);
    } else {
        listen_addr.append("8050");
    }
    pebble_server.AddServiceManner(listen_addr, pebble::rpc::PROTOCOL_JSON);

    // 3. 注册 service
    BaseServiceHandler base_service(&pebble_server);
    pebble_server.RegisterService(&base_service);

    // 4. add update
    RpcUpdater* update = new RpcUpdater();
    ret = pebble_server.AddUpdater(update);
    if (-1 == ret) {
        std::cout << "AddUpdater fail" << std::endl;
    }

    // 5. 启动服务,
    pebble_server.Start();

    // 6. remove update
    ret = pebble_server.RemoveUpdater(update);
    assert(1 == ret);

    return 0;
}
