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


#ifndef PEBBLE_LOG_LOG_H
#define PEBBLE_LOG_LOG_H

#include <stdint.h>

#include "source/common/pebble_common.h"

namespace pebble {
namespace log {

/// @brief 日志输出设备，支持标准输出、文件
/// @note 输出到网络通过tlog支持
typedef enum {
    DEV_STDOUT,
    DEV_FILE,
} DEVICE_TYPE;

/// @brief 日志级别，各级别的基本原则为\n
///     TRACE : 单步调试\n
///     DEBUG : 调试信息、服务处理关键路径\n
///     INFO  : 统计、系统操作信息（如配置重载、控制命令）\n
///     ERROR : 逻辑错误，服务处理失败\n
///     FATAL : 系统错误，无法提供服务\n
typedef enum {
    LOG_PRIORITY_TRACE,
    LOG_PRIORITY_DEBUG,
    LOG_PRIORITY_INFO,
    LOG_PRIORITY_ERROR,
    LOG_PRIORITY_FATAL,
} LOG_PRIORITY;

/// @brief 适配tlog记录日志接口
/// 使用方法示例:
///     // 初始化tlog，获取句柄
///     LPTLOGCTX log_ctx = tlog_init_from_file("log_test.xml");
///     LPTLOGCATEGORYINST log_handler = tlog_get_category(log_ctx, "test");
///
///     // 向log注册写日志函数即可
///     using namespace cxx::placeholders;
///     pebble::log::TLogFunc tlogfunc = cxx::bind(&pebble::log::TLogAdapter::Write, log_handler,
///         _1, _2, _3, _4, _5, _6, _7);
///     pebble::log::Log::RegisterTlogFunc(tlogfunc);
typedef cxx::function<void(int pri, const char* file, uint32_t line,
    const char* function, uint32_t id, uint32_t cls, const char* msg)> TLogFunc;

class Log {
public:
    /// @brief 设置输出设备类型
    /// @return 0成功，非0失败
    /// @note 默认为输出到文件
    static int SetOutputDevice(DEVICE_TYPE device);

    /// @brief 设置输出设备类型
    /// @para device 取值范围为 { "FILE", "STDOUT" }，大小写敏感
    /// @return 0成功，非0失败
    /// @note 默认为输出到文件
    static int SetOutputDevice(const std::string& device);

    /// @brief 设置打印级别，如设置ERROR级别后，只有>=ERROR的log会记录
    /// @return 0成功，非0失败
    /// @note 默认为DEBUG
    static int SetLogPriority(LOG_PRIORITY priority);

    /// @brief 设置打印级别，如设置ERROR级别后，只有>=ERROR的log会记录
    /// @para priority 取值范围为 { "TRACE", "DEBUG", "INFO", "ERROR", "FATAL" }，大小写敏感
    /// @return 0成功，非0失败
    /// @note 默认为DEBUG
    static int SetLogPriority(const std::string& priority);

    /// @brief 设置单个log文件的最大大小，单位为"M bytes"
    /// @return 实际设置的文件大小值
    /// @note max_size_bytes小于1时设置为1
    /// @note 单个log文件最大大小默认为10M
    static uint32_t SetMaxFileSize(uint32_t max_size_Mbytes);

    /// @brief 设置日志文件滚动个数
    /// @return 实际设置的滚动个数
    /// @note num小于1时设置为1
    /// @note 默认滚动个数为10
    static uint32_t SetMaxRollNum(uint32_t num);

    /// @brief 设置log文件路径
    /// @return 0成功，非0失败
    /// @note 默认写到当前目录下
    static int SetFilePath(const std::string& file_path);

    /// @brief 写log接口
    /// @para id,cls，兼容tlog(根据id和cls过滤用)
    static void Write(LOG_PRIORITY pri, const char* file, uint32_t line,
        const char* function, uint32_t id, uint32_t cls, const char* fmt, ...);

    /// @brief 仅在写文件时涉及
    static void Close();

    /// @brief 兼容tlog，注册tlog日志接口，日志将通过tlog输出
    /// @note 注册后log将通过tlog输出，上面Set接口将不再生效
    static void RegisterTlogFunc(const TLogFunc& tlog_func);

    /// @brief 打开程序crash记录调用栈功能
    static void EnableCrashRecord();

    /// @brief flush，由用户决定flush时机
    static void Flush();
};

} // namespace log
} // namespace pebble


#define PLOG_FATAL(fmt, ...) pebble::log::Log::Write(pebble::log::LOG_PRIORITY_FATAL, __FILE__, __LINE__, __FUNCTION__, 0, 0, fmt, ##__VA_ARGS__); // NOLINT
#define PLOG_ERROR(fmt, ...) pebble::log::Log::Write(pebble::log::LOG_PRIORITY_ERROR, __FILE__, __LINE__, __FUNCTION__, 0, 0, fmt, ##__VA_ARGS__); // NOLINT
#define PLOG_INFO(fmt, ...)  pebble::log::Log::Write(pebble::log::LOG_PRIORITY_INFO,  __FILE__, __LINE__, __FUNCTION__, 0, 0, fmt, ##__VA_ARGS__); // NOLINT
#define PLOG_DEBUG(fmt, ...) pebble::log::Log::Write(pebble::log::LOG_PRIORITY_DEBUG, __FILE__, __LINE__, __FUNCTION__, 0, 0, fmt, ##__VA_ARGS__); // NOLINT
#define PLOG_TRACE(fmt, ...) pebble::log::Log::Write(pebble::log::LOG_PRIORITY_TRACE, __FILE__, __LINE__, __FUNCTION__, 0, 0, fmt, ##__VA_ARGS__); // NOLINT

// 条件日志
#define PLOG_IF_FATAL(condition, fmt, ...) if (condition) { PLOG_FATAL(fmt, ##__VA_ARGS__); }
#define PLOG_IF_ERROR(condition, fmt, ...) if (condition) { PLOG_ERROR(fmt, ##__VA_ARGS__); }
#define PLOG_IF_INFO(condition, fmt, ...)  if (condition) { PLOG_INFO(fmt, ##__VA_ARGS__);  }
#define PLOG_IF_DEBUG(condition, fmt, ...) if (condition) { PLOG_DEBUG(fmt, ##__VA_ARGS__); }
#define PLOG_IF_TRACE(condition, fmt, ...) if (condition) { PLOG_TRACE(fmt, ##__VA_ARGS__); }

// 支持过滤，只适用于使用tlog
#define PLOG_FILTER_FATAL(id, cls, fmt, ...) pebble::log::Log::Write(pebble::log::LOG_PRIORITY_FATAL, __FILE__, __LINE__, __FUNCTION__, id, cls, fmt, ##__VA_ARGS__); // NOLINT
#define PLOG_FILTER_ERROR(id, cls, fmt, ...) pebble::log::Log::Write(pebble::log::LOG_PRIORITY_ERROR, __FILE__, __LINE__, __FUNCTION__, id, cls, fmt, ##__VA_ARGS__); // NOLINT
#define PLOG_FILTER_INFO(id, cls, fmt, ...)  pebble::log::Log::Write(pebble::log::LOG_PRIORITY_INFO,  __FILE__, __LINE__, __FUNCTION__, id, cls, fmt, ##__VA_ARGS__); // NOLINT
#define PLOG_FILTER_DEBUG(id, cls, fmt, ...) pebble::log::Log::Write(pebble::log::LOG_PRIORITY_DEBUG, __FILE__, __LINE__, __FUNCTION__, id, cls, fmt, ##__VA_ARGS__); // NOLINT
#define PLOG_FILTER_TRACE(id, cls, fmt, ...) pebble::log::Log::Write(pebble::log::LOG_PRIORITY_TRACE, __FILE__, __LINE__, __FUNCTION__, id, cls, fmt, ##__VA_ARGS__); // NOLINT


#endif // PEBBLE_LOG_LOG_H
