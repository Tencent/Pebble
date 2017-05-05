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
#include <readline/history.h>
#include <readline/readline.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "client/pebble_client.h"
#include "common/mutex.h"
#include "common/string_utility.h"
#include "extension/extension.h"
#include "gflags/gflags.h"
#include "pebble_version.inh"
#include "src/server/control__PebbleControl.h"


// 考虑到控制命令客户端是一个比较小的程序，所有实现均写到一个cpp中

////////////////////////////////////////////////////////////////////////////////////////////////////////////


#define _CHECK_RESULT(condition, ret, error) \
    if ((condition)) { \
        snprintf(m_error, sizeof(m_error), "(%s:%d)(%s) ret=%d error=%s\n", \
            __FILE__, __LINE__, __FUNCTION__, static_cast<int32_t>(ret), (error)); \
        return -1; \
    }

////////////////////////////////////////////////////////////////////////////////////////////////////////////

// 控制命令RPC交互处理类定义
class ControlClient {
public:
    ControlClient();
    ~ControlClient();

    int32_t Init(const std::string& url, bool batch);

    int32_t RunCommand(const std::string& cmd, const std::vector<std::string>& args);

    int32_t Update();

    const char* GetLastError() { return m_error; }

private:
    void CbRunCommand(const std::string& cmd,
        int ret_code, const pebble::ControlResponse& response);

private:
    char m_error[256];
    bool    m_batch;
    pebble::PebbleClient* m_pebble_client;
    pebble::_PebbleControlClient* m_stub;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////

// 控制命令RPC交互处理类实现
ControlClient::ControlClient() {
    m_error[0]  = '\0';
    m_batch     = false;
    m_pebble_client = NULL;
    m_stub      = NULL;
}

ControlClient::~ControlClient() {
    delete m_stub;
    delete m_pebble_client;
}

int32_t ControlClient::Init(const std::string& url, bool batch) {
    m_batch = batch;

    // 建立连接
    int32_t ret = 0;
    bool use_address = false;
    if (pebble::StringUtility::StartsWith(url, "tbuspp")
        || pebble::StringUtility::StartsWith(url, "http")
        || pebble::StringUtility::StartsWith(url, "tbus")) {
        // tbuspp not opened
        // INSTALL_TBUSPP(ret);
        ret = -1;
        use_address = true;
    } else if (pebble::StringUtility::StartsWith(url, "tcp")
        || pebble::StringUtility::StartsWith(url, "udp")) {
        use_address = true;
    }
    _CHECK_RESULT(ret != 0, ret, "INSTALL TBUSPP failed, check the log for detail reason.");

    m_pebble_client = new pebble::PebbleClient();
    ret = m_pebble_client->Init();
    _CHECK_RESULT(ret != 0, ret, "PebbleClient init failed, check the log for detail reason.");

    if (use_address) {
        m_stub = m_pebble_client->NewRpcClientByAddress<pebble::_PebbleControlClient>(
            url, pebble::kPEBBLE_RPC_JSON);
    } else {
        m_stub = m_pebble_client->NewRpcClientByName<pebble::_PebbleControlClient>(
            url, pebble::kPEBBLE_RPC_JSON);
    }
    _CHECK_RESULT(m_stub == NULL, -1, "create stub failed, check the log for detail reason.");

    return 0;
}

int32_t ControlClient::RunCommand(const std::string& cmd, const std::vector<std::string>& args) {
    _CHECK_RESULT(cmd.empty(), -1, "cmd is null");
    _CHECK_RESULT(m_stub == NULL, -1, "control client not init");

    pebble::ControlRequest req;
    req.command = cmd;
    if (!args.empty()) {
        req.__set_options(args);
    }

    m_stub->RunCommand(req, cxx::bind(&ControlClient::CbRunCommand, this,
        cmd, cxx::placeholders::_1, cxx::placeholders::_2));
    return 0;
}

int32_t ControlClient::Update() {
    if (m_pebble_client) {
        return m_pebble_client->Update();
    }
    return 0;
}

void ControlClient::CbRunCommand(const std::string& cmd,
    int ret_code, const pebble::ControlResponse& response) {

    int result = ret_code;
    if (result != 0) {
        std::cout << "RPC Call failed : " << pebble::GetErrorString(result) << std::endl;
    } else {
        result = response.ret_code;
        std::cout << response.data << std::endl << std::endl;
    }

    if (m_batch) {
        exit(result != 0 ? -1 : 0);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////

pebble::Mutex g_mutex;
bool g_quit = false;
void* ControlIOThread(void* arg) {
    ControlClient* client = reinterpret_cast<ControlClient*>(arg);
    do {
        usleep(10000);
        pebble::AutoLocker lock(&g_mutex);
        if (g_quit) {
            break;
        }
        client->Update();
    } while (true);
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////


DEFINE_string(url, "tcp://127.0.0.1:9000",
    "specify server listen address, support tbuspp,http,tcp and udp:\n"
    "    tbuspp://appid.control/instanceid\n"
    "    http://127.0.0.1:8880/appid/control\n"
    "    tcp://127.0.0.1:9000\n"
    "    udp://127.0.0.1:9001\n");
DEFINE_string(execute, "", "execute command and quit.");
DEFINE_string(options, "", "specify the options of execute command, "
    "multi options are separated by commas(,) or space.");


int main(int argc,  char* argv[]) {
    // 预处理options参数，支持空格分隔多参数
    std::string ex_options;
    char option_str[] = "-options";
    for (int i = 1; i < argc; i++) {
        char* find = strstr(argv[i], option_str);
        if (find == NULL) {
            continue;
        }
        // --options "a" "b" "c"
        int offset = 2;
        if (*(find + strlen(option_str)) == '=') {
            // --options="a" "b" "c"
            offset = 1;
        }
        for (int j = i + offset; j < argc; j++) {
            if (argv[j][0] == '-') {
                break;
            }
            ex_options.append(",");
            ex_options.append(argv[j]);
        }
        break;
    }

    // 命令行处理
    std::string version(pebble::PebbleVersion::GetVersion());
    google::SetVersionString(version);

    google::SetUsageMessage("usage:");
    google::ParseCommandLineFlags(&argc, &argv, false);

    // 获取命令行参数
    std::string url(FLAGS_url);
    std::string cmd(FLAGS_execute);
    std::string options(FLAGS_options);
    options.append(ex_options);
    std::vector<std::string> params_vector;

    // 是否为直接执行命令模式(批处理模式)
    bool batch = !cmd.empty();

    // 创建ControlClient对象
    ControlClient client;
    int32_t ret = client.Init(url, batch);
    if (ret != 0) {
        std::cout << "control client init failed : " << client.GetLastError() << std::endl;
        return -1;
    }

    // 命令模式
    if (batch) {
        pebble::StringUtility::Split(options, ",", &params_vector);
        ret = client.RunCommand(cmd, params_vector);
        if (ret != 0) {
            std::cout << "client run cmd failed : "  << client.GetLastError() << std::endl;
            return ret;
        }
        do {
            client.Update();
            usleep(10000);
        } while (true);

        return 0;
    }

    // 交互模式
    pthread_t io_thread;
    ret = pthread_create(&io_thread, NULL, ControlIOThread, reinterpret_cast<void *>(&client));
    if (ret != 0) {
        std::cout << "pthread_create failed : " << ret << std::endl;
        return ret;
    }

    std::cout << "Welcome to Pebble Control Client " << version << std::endl
        << " \'help\' for help, \'quit\' for quit" << std::endl;

    rl_bind_key('\t', rl_insert);
    
    do {
        char* line = readline("> ");
        if (line == NULL) {
            continue;
        }
        std::string input(line);
        free(line);
        pebble::StringUtility::Trim(input);
        if (input.empty()) {
            continue;
        }

        ret = 0;
        cmd.clear();
        options.clear();
        params_vector.clear();

        // 控制端界面只支持quit和help命令，其他命令通过help获取
        // 服务端内置reload、stat命令，用户可添加
        if (strcasecmp(input.c_str(), "quit") == 0) {
            pebble::AutoLocker lock(&g_mutex);
            g_quit = true;
            break;
        } else {
            size_t pos = input.find_first_of(' ');
            cmd = input.substr(0, pos);
            if (pos != std::string::npos) {
                options = input.substr(pos + 1);
                pebble::StringUtility::Split(options, " ", &params_vector);
                pebble::StringUtility::Trim(&params_vector);
            }
            pebble::AutoLocker lock(&g_mutex);
            ret = client.RunCommand(cmd, params_vector);

            add_history(input.c_str());
            if (ret != 0) {
                std::cout << "client run cmd failed : "  << client.GetLastError() << std::endl;
            }
        }
        if (ret == 0) {
            usleep(300 * 1000);
        }
    } while (true);

    pthread_join(io_thread, NULL);
}

