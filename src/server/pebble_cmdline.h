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

#ifndef  _PEBBLE_EXTENSION_PEBBLE_CMDLINE_H_
#define  _PEBBLE_EXTENSION_PEBBLE_CMDLINE_H_

#include <string>

#include "common/platform.h"


namespace pebble {

class CmdLineHandler {
public:
    CmdLineHandler();
    virtual ~CmdLineHandler();

    /// @brief 设置业务程序的版本号
    /// @param version 业务程序的版本号字符串
    void SetVersion(const char* version);

    /// @brief 处理命令行参数
    /// @return 0 成功，非0失败
    virtual int Process(int argc, char** argv);

    /// @brief 返回命令行参数 --conf_file 的值
    const char* ConfigFile() const;

private:
    void MakeDaemon();
    int OnStart();
    int OnStop();
    int OnReload();
    int ReadPidFile(int* pid);
    int WritePidFile();

private:
    std::string m_pid_file;
    std::string m_conf_file;
    bool m_set_version;
};

} // namespace pebble

#endif // _PEBBLE_EXTENSION_PEBBLE_CMDLINE_H_

