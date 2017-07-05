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
#include <execinfo.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <sstream>
#include <string>

#include "common/file_util.h"
#include "common/log.h"
#include "common/string_utility.h"
#include "common/time_utility.h"


namespace pebble {

#define ARRAYSIZE(a) (sizeof(a) / sizeof(*(a)))

static const char*  g_device_str[]   = { "STDOUT", "FILE" };
static const char*  g_priority_str[] = { "TRACE", "DEBUG", "INFO", "ERROR", "FATAL" };


////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 信号处理部分:
static const struct {
        int number;
        const char* name;
    } g_failure_signals[] = {
        { SIGSEGV, "SIGSEGV" },
        { SIGILL,  "SIGILL"  },
        { SIGFPE,  "SIGFPE"  },
        { SIGABRT, "SIGABRT" },
        { SIGBUS,  "SIGBUS"  },
        { SIGTERM, "SIGTERM" }
    };

static struct sigaction g_sigaction_bak[ARRAYSIZE(g_failure_signals)];

// 获取调用栈
int GetStackTrace(void** result, int max_depth, int skip_num) {
    static const int stack_len = 64;
    void * stack[stack_len]    = {0};

    int size = backtrace(stack, stack_len);

    int remain = size - (++skip_num);
    if (remain < 0) {
        remain = 0;
    }
    if (remain > max_depth) {
        remain = max_depth;
    }

    for (int i = 0; i < remain; i++) {
        result[i] = stack[i + skip_num];
    }

    return remain;
}

// 信号处理
void SignalHandler(int signum, siginfo_t* siginfo, void* ucontext)
{
    // 获取时间
    std::ostringstream oss;
    oss << "[" << TimeUtility::GetStringTimeDetail() << "]";

    // 获取信号名
    const char* signame = "";
    uint32_t i = 0;
    for (; i < ARRAYSIZE(g_failure_signals); i++) {
        if (g_failure_signals[i].number == signum) {
            signame = g_failure_signals[i].name;
            break;
        }
    }
    oss << "[(SIGNAL)(" << signame << ")][FATAL]";

    // 获取栈
    void* stack[32] = {0};
    int depth = GetStackTrace(stack, ARRAYSIZE(stack), 1);
    char** symbols = backtrace_symbols(stack, depth);

    oss << "\nstack trace:\n";
    for (int i = 0; i < depth; i++) {
        oss << "#" << i << " " << symbols[i] << "\n";
    }

    oss << "\n";

    // 输出
    PLOG_FATAL("%s", oss.str().c_str());

    // 恢复默认处理
    sigaction(signum, &g_sigaction_bak[i], NULL);
    kill(getpid(), signum);
}

// 注册信号处理
void InstallSignalHandler()
{
    static bool _installed = false;
    if (_installed) {
        return;
    }
    _installed = true;

    struct sigaction sig_action;
    memset(&sig_action, 0, sizeof(sig_action));
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags |= SA_SIGINFO;
    sig_action.sa_sigaction = &SignalHandler;

    for (uint32_t i = 0; i < ARRAYSIZE(g_failure_signals); i++) {
        sigaction(g_failure_signals[i].number, &sig_action, &g_sigaction_bak[i]);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// log实现部分:

Log::Log() {
    m_device_type  = DEV_FILE;
    m_log_priority = LOG_PRIORITY_INFO;

    std::string self_name = GetSelfName();
    m_log_array[kLOG_LOG]   = new RollUtil("./log", self_name + ".log");
    m_log_array[kLOG_ERROR] = new RollUtil("./log", self_name + ".error");
    m_log_array[kLOG_STAT]  = new RollUtil("./log", self_name + ".stat");

    m_isset_time    = false;
    m_current_time  = TimeUtility::GetCurrentUS();
}

Log::Log(const Log& rhs) {
    m_device_type  = DEV_FILE;
    m_log_priority = LOG_PRIORITY_INFO;

    for (int i = 0; i < kLOG_BUTT; i++) {
        m_log_array[i] = NULL;
    }

    m_isset_time    = false;
    m_current_time  = 0;
}

Log::~Log() {
    for (int i = 0; i < kLOG_BUTT; i++) {
        delete m_log_array[i];
        m_log_array[i] = NULL;
    }
}

void Log::Write(LOG_PRIORITY pri, const char* file, uint32_t line,
    const char* function, const char* fmt, ...)
{
    // 优先级以PLOG为准，避免多做格式化
    if (pri < m_log_priority) {
        return;
    }

    static char buff[4096] = {0};

    // log前缀，接入其他log时不用组装
    int pre_len = 0;
    if (!m_log_write_func) {
        pre_len = snprintf(buff, ARRAYSIZE(buff), "[%s][%d][(%s:%d)(%s)][%s] ",
                TimeUtility::GetStringTimeDetail(),
                getpid(),
                file,
                line,
                function,
                g_priority_str[pri]
            );
        if (pre_len < 0) {
            pre_len = 0;
        }
    }

    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(buff + pre_len, ARRAYSIZE(buff) - pre_len, fmt, ap);
    va_end(ap);
    if (len < 0) {
        len = 0;
    }

    // 输出到其他log
    if (m_log_write_func) {
        m_log_write_func(pri, file, line, function, buff);
        return;
    }

    // 其他log以有'\n'，PLOG需要再补换行
    uint32_t tail = len + pre_len;
    if (tail > (ARRAYSIZE(buff) - 2)) {
        tail = ARRAYSIZE(buff) - 2;
    }

    buff[tail++] = '\n';
    buff[tail] = '\0';

    // 输出到stdout
    if (DEV_STDOUT == m_device_type) {
        fprintf(stdout, "%s", buff);
        return;
    }

    // 输出到ERROR文件
    if (pri >= LOG_PRIORITY_ERROR && m_log_array[kLOG_ERROR] != NULL) {
        FILE* error = m_log_array[kLOG_ERROR]->GetFile();
        if (error != NULL) {
            fwrite(buff, tail, 1, error);
        }
    }

    // 输出到log文件
    if (m_log_array[kLOG_LOG] != NULL) {
        FILE* log = m_log_array[kLOG_LOG]->GetFile();
        if (log != NULL) {
            fwrite(buff, tail, 1, log);
        }
    }
}

void Log::Write(const char* data)
{
    if (m_log_array[kLOG_STAT] == NULL) {
        return;
    }

    FILE* stat = m_log_array[kLOG_STAT]->GetFile();
    if (stat != NULL) {
        char buff[64] = {0};
        int len = snprintf(buff, ARRAYSIZE(buff), "[%s] ", TimeUtility::GetStringTimeDetail());
        fwrite(buff, len, 1, stat);
        fwrite(data, strlen(data), 1, stat);
    }
}

void Log::Close()
{
    for (int i = 0; i < kLOG_BUTT; i++) {
        if (m_log_array[i]) {
            m_log_array[i]->Close();
        }
    }
}

void Log::RegisterLogWriteFunc(const LogWriteFunc& log_write_func)
{
    m_log_write_func = log_write_func;
}

void Log::EnableCrashRecord()
{
    InstallSignalHandler();
}

void Log::Flush()
{
    for (int i = 0; i < kLOG_BUTT; i++) {
        if (m_log_array[i]) {
            m_log_array[i]->Flush();
        }
    }
}

int Log::SetOutputDevice(DEVICE_TYPE device)
{
    if (device < DEV_STDOUT || device > DEV_FILE) {
        return -1;
    }
    m_device_type = device;
    return 0;
}

int Log::SetOutputDevice(const std::string& device)
{
    std::string tmp(device);
    StringUtility::ToUpper(&tmp);
    for (uint32_t i = 0; i < ARRAYSIZE(g_device_str); i++) {
        if (g_device_str[i] == tmp) {
            m_device_type = static_cast<DEVICE_TYPE>(i);
            return 0;
        }
    }
    return -1;
}

int Log::SetLogPriority(LOG_PRIORITY priority)
{
    if (priority < LOG_PRIORITY_TRACE || priority > LOG_PRIORITY_FATAL) {
        return -1;
    }
    m_log_priority = priority;
    return 0;
}

int Log::SetLogPriority(const std::string& priority)
{
    std::string tmp(priority);
    StringUtility::ToUpper(&tmp);
    for (uint32_t i = 0; i < ARRAYSIZE(g_priority_str); i++) {
        if (g_priority_str[i] == tmp) {
            m_log_priority = static_cast<LOG_PRIORITY>(i);
            return 0;
        }
    }
    return -1;
}

void Log::SetMaxFileSize(uint32_t max_size_Mbytes)
{
    uint32_t file_size = max_size_Mbytes;
    if (max_size_Mbytes < 1) {
        file_size = 1;
    }
    file_size = file_size * 1024 * 1024;

    for (int i = 0; i < kLOG_BUTT; i++) {
        if (m_log_array[i]) {
            m_log_array[i]->SetFileSize(file_size);
        }
    }
}

void Log::SetMaxRollNum(uint32_t num)
{
    for (int i = 0; i < kLOG_BUTT; i++) {
        if (m_log_array[i]) {
            m_log_array[i]->SetRollNum(num);
        }
    }
}

void Log::SetFilePath(const std::string& file_path)
{
    for (int i = 0; i < kLOG_BUTT; i++) {
        if (m_log_array[i]) {
            m_log_array[i]->SetFilePath(file_path);
        }
    }

    // 更改路径后log立即输出在新的路径下面
    Close();
}

void Log::SetCurrentTime(int64_t timestamp) {
    m_current_time  = timestamp;
    m_isset_time    = true;
}

int64_t Log::GetCurrentTime() {
    if (m_isset_time) {
        return m_current_time;
    }
    return TimeUtility::GetCurrentUS();
}

} // namespace pebble

