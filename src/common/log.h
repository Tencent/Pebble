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
};

} // namespace pebble


#define PLOG_FATAL(fmt, ...) pebble::Log::Instance().Write(pebble::LOG_PRIORITY_FATAL, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__); // NOLINT
#define PLOG_ERROR(fmt, ...) pebble::Log::Instance().Write(pebble::LOG_PRIORITY_ERROR, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__); // NOLINT
#define PLOG_INFO(fmt,  ...) pebble::Log::Instance().Write(pebble::LOG_PRIORITY_INFO,  __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__); // NOLINT
#define PLOG_DEBUG(fmt, ...) pebble::Log::Instance().Write(pebble::LOG_PRIORITY_DEBUG, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__); // NOLINT
#define PLOG_TRACE(fmt, ...) pebble::Log::Instance().Write(pebble::LOG_PRIORITY_TRACE, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__); // NOLINT

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

#endif // _PEBBLE_COMMON_LOG_H_

