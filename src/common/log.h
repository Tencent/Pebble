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


#ifndef _PEBBLE_COMMON_LOG_H_
#define _PEBBLE_COMMON_LOG_H_

#include "common/platform.h"

namespace pebble {

/// @brief 日志输出设备，支持标准输出、文件
/// @note 输出到网络通过tlog支持
typedef enum {
    DEV_STDOUT = 0,
    DEV_FILE,
} DEVICE_TYPE;

/// @brief 日志级别，各级别的基本原则为\n
///     TRACE : 单步调试\n
///     DEBUG : 调试信息、服务处理关键路径\n
///     INFO  : 统计、系统操作信息（如配置重载、控制命令）\n
///     ERROR : 逻辑错误，服务处理失败\n
///     FATAL : 系统错误，无法提供服务\n
typedef enum {
    LOG_PRIORITY_TRACE = 0,
    LOG_PRIORITY_DEBUG,
    LOG_PRIORITY_INFO,
    LOG_PRIORITY_ERROR,
    LOG_PRIORITY_FATAL,
} LOG_PRIORITY;

/// @brief 适配其他日志接口，PLOG信息写到其他log
typedef cxx::function<void(int priority, const char* file, uint32_t line,
    const char* function, const char* msg)> LogWriteFunc;

class RollUtil;

class Log {
protected:
    Log();
    Log(const Log& rhs);

public:
    static Log& Instance() {
        static Log s_log_instance;
        return s_log_instance;
    }

    ~Log();

    /// @brief 设置输出设备类型
    /// @return 0成功，非0失败
    /// @note 默认为输出到文件
    int SetOutputDevice(DEVICE_TYPE device);

    /// @brief 设置输出设备类型
    /// @para device 取值范围为 { "FILE", "STDOUT" }，大小写敏感
    /// @return 0成功，非0失败
    /// @note 默认为输出到文件
    int SetOutputDevice(const std::string& device);

    /// @brief 设置打印级别，如设置ERROR级别后，只有>=ERROR的log会记录
    /// @return 0成功，非0失败
    /// @note 默认为DEBUG
    int SetLogPriority(LOG_PRIORITY priority);

    /// @brief 设置打印级别，如设置ERROR级别后，只有>=ERROR的log会记录
    /// @para priority 取值范围为 { "TRACE", "DEBUG", "INFO", "ERROR", "FATAL" }，大小写敏感
    /// @return 0成功，非0失败
    /// @note 默认为DEBUG
    int SetLogPriority(const std::string& priority);

    /// @brief 设置单个log文件的最大大小，单位为"M bytes"
    /// @note max_size_bytes小于1时设置为1
    /// @note 单个log文件最大大小默认为10M
    void SetMaxFileSize(uint32_t max_size_Mbytes);

    /// @brief 设置日志文件滚动个数
    /// @note 默认滚动个数为10
    void SetMaxRollNum(uint32_t num);

    /// @brief 设置log文件路径
    /// @note 默认写到当前目录下
    void SetFilePath(const std::string& file_path);

    /// @brief 写log接口
    /// @para id,cls，兼容tlog(根据id和cls过滤用)
    void Write(LOG_PRIORITY pri, const char* file, uint32_t line,
        const char* function, const char* fmt, ...);

    /// @brief 一般用于写统计数据
    void Write(const char* data);

    /// @brief 仅在写文件时涉及
    void Close();

    /// @brief 兼容其他log，注册其他log写日志接口
    /// @note 注册后log将通过其他log输出，上面Set接口将不再生效
    void RegisterLogWriteFunc(const LogWriteFunc& tlog_func);

    /// @brief 打开程序crash记录调用栈功能
    void EnableCrashRecord();

    /// @brief flush，由用户决定flush时机
    void Flush();

public:
    /// @brief 设置当前时间，应该在上层框架主循环中不断的调用，以及时刷新时间
    void SetCurrentTime(int64_t timestamp);

    /// @brief 返回当前时间戳
    int64_t GetCurrentTime();

    /// @brief 返回当前设置的打印级别
    int32_t GetPriority() {
        return m_log_priority;
    }

private:
    enum LogType {
        kLOG_LOG = 0,
        kLOG_ERROR,
        kLOG_STAT,
        kLOG_BUTT
    };
    DEVICE_TYPE     m_device_type;
    LOG_PRIORITY    m_log_priority;
    RollUtil*       m_log_array[kLOG_BUTT];
    LogWriteFunc    m_log_write_func;

    bool            m_isset_time;
    int64_t         m_current_time;
};

} // namespace pebble


#define PLOG_FATAL(fmt, ...) do { if (pebble::LOG_PRIORITY_FATAL >= pebble::Log::Instance().GetPriority()) { pebble::Log::Instance().Write(pebble::LOG_PRIORITY_FATAL, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__); } } while (0)
#define PLOG_ERROR(fmt, ...) do { if (pebble::LOG_PRIORITY_ERROR >= pebble::Log::Instance().GetPriority()) { pebble::Log::Instance().Write(pebble::LOG_PRIORITY_ERROR, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__); } } while (0)
#define PLOG_INFO(fmt,  ...) do { if (pebble::LOG_PRIORITY_INFO  >= pebble::Log::Instance().GetPriority()) { pebble::Log::Instance().Write(pebble::LOG_PRIORITY_INFO,  __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__); } } while (0)
#define PLOG_DEBUG(fmt, ...) do { if (pebble::LOG_PRIORITY_DEBUG >= pebble::Log::Instance().GetPriority()) { pebble::Log::Instance().Write(pebble::LOG_PRIORITY_DEBUG, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__); } } while (0)
#define PLOG_TRACE(fmt, ...) do { if (pebble::LOG_PRIORITY_TRACE >= pebble::Log::Instance().GetPriority()) { pebble::Log::Instance().Write(pebble::LOG_PRIORITY_TRACE, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__); } } while (0)

#define PLOG_STAT(data) pebble::Log::Instance().Write(data);

// 条件日志
#define PLOG_IF_FATAL(condition, fmt, ...) if (condition) { PLOG_FATAL(fmt, ##__VA_ARGS__); }
#define PLOG_IF_ERROR(condition, fmt, ...) if (condition) { PLOG_ERROR(fmt, ##__VA_ARGS__); }
#define PLOG_IF_INFO(condition, fmt,  ...) if (condition) { PLOG_INFO(fmt,  ##__VA_ARGS__); }
#define PLOG_IF_DEBUG(condition, fmt, ...) if (condition) { PLOG_DEBUG(fmt, ##__VA_ARGS__); }
#define PLOG_IF_TRACE(condition, fmt, ...) if (condition) { PLOG_TRACE(fmt, ##__VA_ARGS__); }

// 记录日志并返回
#define RETURN_IF_FATAL(condition, ret, fmt, ...) if (condition) { PLOG_FATAL(fmt, ##__VA_ARGS__); return (ret); }
#define RETURN_IF_ERROR(condition, ret, fmt, ...) if (condition) { PLOG_ERROR(fmt, ##__VA_ARGS__); return (ret); }
#define RETURN_IF_INFO(condition, ret, fmt,  ...) if (condition) { PLOG_INFO(fmt,  ##__VA_ARGS__); return (ret); }
#define RETURN_IF_DEBUG(condition, ret, fmt, ...) if (condition) { PLOG_DEBUG(fmt, ##__VA_ARGS__); return (ret); }
#define RETURN_IF_TRACE(condition, ret, fmt, ...) if (condition) { PLOG_TRACE(fmt, ##__VA_ARGS__); return (ret); }

// 频率控制
#define LOG_VARNAME(prefix, line) LOG_VARNAME_CONCAT(prefix, line)
#define LOG_VARNAME_CONCAT(prefix, line) prefix ## line

#define LOG_CNT_VAR     LOG_VARNAME(s_log_cnt_, __LINE__)
#define START_TIME_VAR  LOG_VARNAME(s_start_time_ ,__LINE__)
#define NOW_TIME_VAR    LOG_VARNAME(s_now_time_ ,__LINE__)

#define PLOG_N_EVERY_SECOND(num, pri, fmt, args...) \
    do { \
        if (pri >= pebble::Log::Instance().GetPriority()) { \
            static int32_t LOG_CNT_VAR = 0; static int64_t START_TIME_VAR = 0; \
            if (START_TIME_VAR == 0) { START_TIME_VAR = pebble::Log::Instance().GetCurrentTime(); } \
            int64_t NOW_TIME_VAR = pebble::Log::Instance().GetCurrentTime(); \
            if (START_TIME_VAR + 1000000 < NOW_TIME_VAR) { \
                START_TIME_VAR = NOW_TIME_VAR; \
                if (LOG_CNT_VAR > num) { \
                    pebble::Log::Instance().Write(pri, __FILE__, __LINE__, __FUNCTION__, \
                        "discard %d logs last second", LOG_CNT_VAR - num); \
                } \
                LOG_CNT_VAR = 0; \
            } \
            if (++LOG_CNT_VAR <= num) { \
                pebble::Log::Instance().Write(pri, __FILE__, __LINE__, __FUNCTION__, fmt, ##args); \
            } \
        } \
    } while (0)

#define PLOG_FATAL_N_EVERY_SECOND(num, fmt, args...) PLOG_N_EVERY_SECOND(num, pebble::LOG_PRIORITY_FATAL, fmt, ##args)
#define PLOG_ERROR_N_EVERY_SECOND(num, fmt, args...) PLOG_N_EVERY_SECOND(num, pebble::LOG_PRIORITY_ERROR, fmt, ##args)
#define PLOG_INFO_N_EVERY_SECOND(num, fmt,  args...) PLOG_N_EVERY_SECOND(num, pebble::LOG_PRIORITY_INFO, fmt, ##args)
#define PLOG_DEBUG_N_EVERY_SECOND(num, fmt, args...) PLOG_N_EVERY_SECOND(num, pebble::LOG_PRIORITY_DEBUG, fmt, ##args)
#define PLOG_TRACE_N_EVERY_SECOND(num, fmt, args...) PLOG_N_EVERY_SECOND(num, pebble::LOG_PRIORITY_TRACE, fmt, ##args)

#endif // _PEBBLE_COMMON_LOG_H_

