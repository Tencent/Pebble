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


#include "common/log.h"
#include "extension/redis/redis_co.h"


namespace pebble {

struct RedisCoroutineCtx {
    RedisCoroutineCtx() : _redis_coroutine(NULL), _co_id(-1) {}
    RedisCoroutineCtx(RedisCoroutine* rc, int64_t coid)
        : _redis_coroutine(rc), _co_id(coid) {}

    RedisCoroutine* _redis_coroutine;
    int64_t         _co_id;
};

void RedisCallbackFn (redisAsyncContext* ac, void* reply, void* privdata) {
    RedisCoroutineCtx* ctx = (RedisCoroutineCtx*)privdata;
    ctx->_redis_coroutine->OnRedisCallback(ctx->_co_id, (redisReply*)reply);
    delete ctx;
}

RedisCoroutine::RedisCoroutine() {
    m_co_sche    = NULL;
    m_timeout_ms = DEFAULT_REDIS_TIMEOUT_MS;
    m_reply      = NULL;
}

RedisCoroutine::~RedisCoroutine() {
    m_co_sche    = NULL;
}

int RedisCoroutine::Init(CoroutineSchedule* co_sche, int timeout_ms) {
    if (co_sche == NULL || timeout_ms <= 0) {
        PLOG_ERROR("param invalid co_sche=%p, timeout_ms=%d", co_sche, timeout_ms);
        return -1;
    }

    m_co_sche    = co_sche;
    m_timeout_ms = timeout_ms;

    return 0;
}

redisReply* RedisCoroutine::RedisvAsyncCommand(redisAsyncContext *ac, const char *format, va_list ap) {
    RedisCoroutineCtx* ctx = CreateRedisCoroutineCtx();
    if (ctx == NULL) {
        return NULL;
    }
    int ret = redisvAsyncCommand(ac, RedisCallbackFn, (void*)ctx, format, ap);
    if (ret != REDIS_OK) {
        PLOG_ERROR("redisvAsyncCommand failed(%d)", ret);
        delete ctx;
        return NULL;
    }

    if (!Yield()) {
        return NULL;
    }

    return m_reply;
}

redisReply* RedisCoroutine::RedisAsyncCommand(redisAsyncContext *ac, const char *format, ...) {
    redisReply* reply = NULL;
    va_list ap;
    va_start(ap, format);
    reply = RedisvAsyncCommand(ac, format, ap);
    va_end(ap);

    return reply;
}

redisReply* RedisCoroutine::RedisAsyncCommandArgv(redisAsyncContext *ac, int argc, const char **argv, const size_t *argvlen) {
    RedisCoroutineCtx* ctx = CreateRedisCoroutineCtx();
    if (ctx == NULL) {
        return NULL;
    }
    int ret = redisAsyncCommandArgv(ac, RedisCallbackFn, (void*)ctx, argc, argv, argvlen);
    if (ret != REDIS_OK) {
        PLOG_ERROR("redisAsyncCommandArgv failed(%d)", ret);
        delete ctx;
        return NULL;
    }

    if (!Yield()) {
        return NULL;
    }

    return m_reply;
}

redisReply* RedisCoroutine::RedisAsyncFormattedCommand(redisAsyncContext *ac, const char *cmd, size_t len) {
    RedisCoroutineCtx* ctx = CreateRedisCoroutineCtx();
    if (ctx == NULL) {
        return NULL;
    }
    int ret = redisAsyncFormattedCommand(ac, RedisCallbackFn, (void*)ctx, cmd, len);
    if (ret != REDIS_OK) {
        PLOG_ERROR("redisAsyncFormattedCommand failed(%d)", ret);
        delete ctx;
        return NULL;
    }

    if (!Yield()) {
        return NULL;
    }

    return m_reply;
}

void RedisCoroutine::OnRedisCallback(int64_t co_id, redisReply* reply) {
    m_reply = reply;
    int ret = m_co_sche->Resume(co_id);
    PLOG_IF_ERROR(ret != 0, "resume failed(%d)", ret);
}

bool RedisCoroutine::Yield() {
    int ret = m_co_sche->Yield(m_timeout_ms);
    if (ret != 0) {
        PLOG_ERROR("yield return failed(%d)", ret);
        return false;
    }
    return true;
}

RedisCoroutineCtx* RedisCoroutine::CreateRedisCoroutineCtx() {
    if (!m_co_sche) {
        PLOG_ERROR("coroutine schedule is null");
        return NULL;
    }

    int64_t co_id = m_co_sche->CurrentTaskId();
    if (co_id == INVALID_CO_ID) {
        PLOG_ERROR("not in coroutine");
        return NULL;
    }

    RedisCoroutineCtx* ctx = new RedisCoroutineCtx(this, co_id);
    return ctx;
}


} // namespace pebble


