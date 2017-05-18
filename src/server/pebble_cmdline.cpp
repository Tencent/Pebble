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

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <unistd.h>

#include "common/dir_util.h"
#include "gflags/gflags.h"
#include "pebble_cmdline.h"
#include "server/pebble_server.h"


namespace pebble {

// 命令行参数定义
DEFINE_bool(start,   false,  "start this process as a daemon.");
DEFINE_bool(stop,    false,  "request previous process to stop safely.");
DEFINE_bool(reload,  false,  "request previous process to reload configuration.");
DEFINE_string(conf_file, "", "specify config file.");
DEFINE_string(pid_file,  "", "specify the file for storing the process's pid.");


CmdLineHandler::CmdLineHandler() {
    m_set_version = false;
}

CmdLineHandler::~CmdLineHandler() {
    google::ShutDownCommandLineFlags();
}

void CmdLineHandler::SetVersion(const char* version) {
    // 设置版本号
    std::string ver("\n");
    if (version) {
        ver.append(version);
    }
    ver.append("\n");
    ver.append(GetVersion());
    google::SetVersionString(ver.c_str());
    m_set_version = true;
}

int CmdLineHandler::Process(int argc, char** argv) {
    // 用户未设置版本号，只显示pebble的版本号
    if (!m_set_version) {
        google::SetVersionString(GetVersion());
    }

    // 设置help
    google::SetUsageMessage("");

    // 解析命令行
    google::ParseCommandLineFlags(&argc, &argv, false);

    // 获取参数 --pid_file 的值
    m_pid_file.assign(FLAGS_pid_file);
    if (m_pid_file.empty()) {
        m_pid_file.assign(argv[0]);
        m_pid_file.append(".pid");
    }

    // 获取参数 --conf_file 的值
    m_conf_file.assign(FLAGS_conf_file);

    // 处理控制命令
    int ret = 0;
    if (FLAGS_start) {
        return OnStart();
    } else if (FLAGS_stop) {
        ret = OnStop();
        exit(ret);
    } else if (FLAGS_reload) {
        ret = OnReload();
        exit(ret);
    }

    return 0;
}

void CmdLineHandler::MakeDaemon() {
    pid_t pid = fork();
    if (pid != 0) {
        exit(0);
    }

    signal(SIGINT,  SIG_IGN);
    signal(SIGHUP,  SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

}

int CmdLineHandler::OnStart() {
    MakeDaemon();

    return WritePidFile();
}

int CmdLineHandler::OnStop() {
    // 读pid文件
    pid_t pid = 0;
    int ret = ReadPidFile(&pid);
    if (ret < 0) {
        fprintf(stderr, "stop failed.\n");
        return -1;
    }

    // 发送信号
    ret = kill(pid, SIGUSR1);
    if (ret != 0) {
        fprintf(stderr, "stop failed because %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

int CmdLineHandler::OnReload() {
    // 读pid文件
    pid_t pid = 0;
    int ret = ReadPidFile(&pid);
    if (ret < 0) {
        fprintf(stderr, "reload failed.\n");
        return -1;
    }

    // 发送信号
    ret = kill(pid, SIGUSR2);
    if (ret != 0) {
        fprintf(stderr, "reload failed because %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

const char* CmdLineHandler::ConfigFile() const {
    if (m_conf_file.empty()) {
        return NULL;
    }
    return m_conf_file.c_str();
}

int CmdLineHandler::ReadPidFile(pid_t* pid) {
    FILE* file = fopen(m_pid_file.c_str(), "r");
    if (NULL == file) {
        fprintf(stderr, "open %s failed because %s\n", m_pid_file.c_str(), strerror(errno));
        return -1;
    }

    char pidstr[32] = {0};
    int len = fread(pidstr, sizeof(pidstr), 1, file);
    if (len < 0) {
        fprintf(stderr, "read %s failed because %s\n", m_pid_file.c_str(), strerror(errno));
        return -1;
    }

    *pid = atoi(pidstr);
    fclose(file);
    return 0;
}

class AutoCloseFile {
public:
    explicit AutoCloseFile(FILE* file) : m_file(file) {}
    ~AutoCloseFile() {
        if (m_file) {
            fclose(m_file);
        }
    }

private:
    FILE* m_file;
};

int CmdLineHandler::WritePidFile() {
    // 用户指定的目录不存在时自动创建目录
    std::string path = m_pid_file.substr(0, m_pid_file.find_last_of('/'));
    if (!path.empty()) {
        if (access(path.c_str(), F_OK | W_OK) != 0) {
            if (pebble::DirUtil::MakeDirP(path) != 0) {
                fprintf(stderr, "mkdir %s failed(%s)", path.c_str(),
                    pebble::DirUtil::GetLastError());
                return -1;
            }
        }
    }

    FILE* file = fopen(m_pid_file.c_str(), "a+");
    if (NULL == file) {
        fprintf(stderr, "open %s failed because %s\n", m_pid_file.c_str(), strerror(errno));
        return -1;
    }

    // 程序退出统一close file
    static AutoCloseFile auto_close_file(file);

    int ret = flock(fileno(file), LOCK_EX | LOCK_NB);
    if (ret != 0) {
        fprintf(stderr, "lock %s failed because %s\n", m_pid_file.c_str(), strerror(errno));
        return -1;
    }

    ret = ftruncate(fileno(file), 0);
    if (ret != 0) {
        fprintf(stderr, "truncate %s failed because %s\n", m_pid_file.c_str(), strerror(errno));
        return -1;
    }

    ret = lseek(fileno(file), 0, SEEK_SET);
    if (ret != 0) {
        fprintf(stderr, "seek %s failed because %s\n", m_pid_file.c_str(), strerror(errno));
        return -1;
    }

    char pidstr[32] = {0};
    int len = snprintf(pidstr, sizeof(pidstr), "%d", getpid());
    len = fwrite(pidstr, len, 1, file);
    if (len <= 0) {
        fprintf(stderr, "write %s to file %s failed because %s\n",
            pidstr, m_pid_file.c_str(), strerror(errno));
        return -1;
    }

    fflush(file);

    return 0;
}

} // namespace pebble

