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

#include <sstream>
#include <stdio.h>
#include <stdlib.h>

#include "extension/extension.h"
#include "server/pebble.h"


#define ASSERT_EQ(expected, actual) \
    if ((expected) != (actual)) { \
        fprintf(stderr, "(%s:%d)(%s) %d != %d\n", \
            __FILE__, __LINE__, __FUNCTION__, (expected), (actual)); \
        exit(1); \
    }

void OnDumpCtrlCmd(const std::vector<std::string>& options,
    int32_t* ret_code, std::string* data) {

    *ret_code = 0;
    std::ostringstream oss;
    oss << "Dump ";
    for (std::vector<std::string>::const_iterator it = options.begin(); it != options.end(); ++it) {
        oss << *it << " ";
    }
    oss << "success";
    data->assign(oss.str());
}

int main(int argc, char** argv) {
    // 启动pebble server
    pebble::PebbleServer server;

    int ret = server.LoadOptionsFromIni("./cfg/pebble.ini");
    ASSERT_EQ(0, ret);

    pebble::Options* options = server.GetOptions();
    if (pebble::StringUtility::StartsWith(options->_app_ctrl_cmd_addr, "tbuspp")
        || pebble::StringUtility::StartsWith(options->_app_ctrl_cmd_addr, "http")
        || pebble::StringUtility::StartsWith(options->_app_ctrl_cmd_addr, "tbus")) {
        //INSTALL_TBUSPP(ret);
        ret = -1;
        ASSERT_EQ(0, ret);
    }

    ret = server.Init();
    ASSERT_EQ(0, ret);

    // 注册自定义控制命令
    ret = server.RegisterControlCommand(OnDumpCtrlCmd,
        "dump",
        "dump               # Dump the engine file to a directory\n"
        "                   # format: dump para1 para2 para3\n"
        "                   # example: dump\n"
        "                   # example: dump a\n"
        "                   # example: dump a b\n"
        "                   # example: dump a b c\n"
    );
    ASSERT_EQ(0, ret);

    server.Serve();

    return 0;
}



