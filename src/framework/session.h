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


#ifndef _PEBBLE_COMMON_SESSION_H_
#define _PEBBLE_COMMON_SESSION_H_

#include "common/error.h"
#include "common/platform.h"


namespace pebble {

// 前置声明
class SequenceTimer;

/// @brief Session模块错误码定义
typedef enum {
    kSESSION_ERROR_BASE         = SESSION_ERROR_CODE_BASE,
    kSESSION_INVALID_PARAM      = kSESSION_ERROR_BASE - 1, // 参数非法
    kSESSION_ALREADY_EXISTED    = kSESSION_ERROR_BASE - 2, // session已存在
    kSESSION_UNEXISTED          = kSESSION_ERROR_BASE - 3, // session不存在
    kSESSION_START_TIMER_FAILED = kSESSION_ERROR_BASE - 4, // 启定时器失败
} SessionErrorCode;

class SessionErrorStringRegister {
public:
    static void RegisterErrorString() {
        SetErrorString(kSESSION_INVALID_PARAM, "invalid paramater");
        SetErrorString(kSESSION_ALREADY_EXISTED, "session already exist");
        SetErrorString(kSESSION_UNEXISTED, "session unexist");
        SetErrorString(kSESSION_START_TIMER_FAILED, "start timer failed");
    }
};


/// @brief Session超时返回码定义
typedef enum {
    kSESSION_BE_REMOVED   = -1,   // 超时后session被remove
    kSESSION_BE_CONTINUED = 0,    // 超时后session仍然保持，重新计时
    kSESSION_BE_RESETED           // 超时后session仍然保持，使用返回值作为超时时间(ms)重新计时
} OnSessionTimeoutRetCode;

/// @brief session接口定义
class Session {
public:
    Session() {}
    virtual ~Session() {}

    /// @brief session超时处理接口
    /// @param session_id 会话ID
    /// @return kSESSION_BE_REMOVED 超时后session被remove
    /// @return kSESSION_BE_CONTINUED 超时后session仍然保持，重新计时
    /// @return >0 超时后session仍然保持，使用返回值作为超时时间(ms)重新计时
    /// @see OnSessionTimeoutRetCode
    virtual int32_t OnTimeout(int64_t session_id) = 0;
};


class SessionMgr {
public:
    SessionMgr();
    ~SessionMgr();

    /// @brief 添加一个session
    /// @param session_id 会话ID
    /// @param session session对象
    /// @param timeout_ms 超时时间(>0)，单位ms
    /// @return 0 成功
    /// @return <0 失败 @see SessionErrorCode
    int32_t AddSession(int64_t session_id, Session* session, uint32_t timeout_ms);

    /// @brief 查询一个session
    /// @param session_id 会话ID
    /// @return 非NULL 成功
    /// @return NULL 失败
    Session* GetSession(int64_t session_id);

    /// @brief 从session列表中移除session_id对应的session(并未删除对象，session对象本身由调用者维护)
    /// @param session_id 会话ID
    /// @return 0 成功
    /// @return <0 失败 @see SessionErrorCode
    int32_t RemoveSession(int64_t session_id);

    /// @brief 超时检测，应该在主循环中调用
    /// @return 超时的session数目
    int32_t CheckTimeout();

    /// @brief 重启会话的计时，若new_timeout_ms>0，使用new_timeout_ms作为超时时间重新计时\n
    ///     否则使用原超时时间重新计时
    /// @param session_id 会话ID
    /// @param new_timeout_ms 新的超时时间，单位ms，=0时使用原超时时间
    /// @return 0 成功
    /// @return <0 失败 @see SessionErrorCode
    int32_t RestartTimer(int64_t session_id, uint32_t new_timeout_ms = 0);

    /// @brief 返回最后错误信息
    const char* GetLastError() const {
        return m_last_error;
    }

    /// @brief 返回session数量
    int64_t GetSessionNum() {
        return m_sessions.size();
    }

private:
    int32_t OnTimeout(int64_t session_id, Session* session);

    struct SessionInfo {
        SessionInfo() {
            m_session    = NULL;
            m_timer_id   = -1;
            m_timeout_ms = 0;
        }

        Session* m_session;
        int64_t  m_timer_id;
        uint32_t m_timeout_ms;
    };

private:
    SequenceTimer* m_timer;
    cxx::unordered_map<int64_t, SessionInfo> m_sessions;
    char m_last_error[256];
};


} // namespace pebble

#endif // _PEBBLE_COMMON_SESSION_H_
