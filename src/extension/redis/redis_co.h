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


#ifndef PEBBLE_EXTENSION_REDIS_CO_H
#define PEBBLE_EXTENSION_REDIS_CO_H

#include "async.h"
#include "common/coroutine.h"

// pebble 发布包本身不提供redis的库

namespace pebble {

struct RedisCoroutineCtx;

/// @brief redis协程接口封装，仅提供协程化同步接口(实际异步调用)，不涉及redis本身的管理
class RedisCoroutine {
public:
    static const int DEFAULT_REDIS_TIMEOUT_MS = 2000;

    RedisCoroutine();
    ~RedisCoroutine();

    /// @param co_sche pebble提供的协程调度器
    /// @param timeout_ms 命令执行的超时时间，单位ms，默认2s
    /// @return 0成功，非0失败
    int Init(CoroutineSchedule* co_sche, int timeout_ms = DEFAULT_REDIS_TIMEOUT_MS);

    /// @brief 下面一组接口对应redis的异步调用，必须要在协程中执行
    /// @return NULL失败，非空成功
    redisReply* RedisvAsyncCommand(redisAsyncContext *ac, const char *format, va_list ap);
    redisReply* RedisAsyncCommand(redisAsyncContext *ac, const char *format, ...);
    redisReply* RedisAsyncCommandArgv(redisAsyncContext *ac, int argc, const char **argv, const size_t *argvlen);
    redisReply* RedisAsyncFormattedCommand(redisAsyncContext *ac, const char *cmd, size_t len);

public:
    /// @note 内部使用
    void OnRedisCallback(int64_t co_id, redisReply* reply);

private:
    bool Yield();
    RedisCoroutineCtx* CreateRedisCoroutineCtx();

private:
    CoroutineSchedule* m_co_sche;
    int m_timeout_ms;
    redisReply * m_reply;
};

} // namespace pebble

#endif // PEBBLE_EXTENSION_REDIS_CO_H

