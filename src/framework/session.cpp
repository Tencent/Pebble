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

#include "common/timer.h"
#include "framework/session.h"


namespace pebble {

SessionMgr::SessionMgr() {
    m_timer         = new SequenceTimer();
    m_last_error[0] = 0;
}

SessionMgr::~SessionMgr() {
    if (m_timer) {
        delete m_timer;
        m_timer = NULL;
    }
}

int32_t SessionMgr::AddSession(int64_t session_id, Session* session, uint32_t timeout_ms) {
    if (NULL == session || 0 == timeout_ms) {
        _LOG_LAST_ERROR("invalid param: session = %p, timeout_ms = %u", session, timeout_ms);
        return kSESSION_INVALID_PARAM;
    }

    std::pair<cxx::unordered_map<int64_t, SessionInfo>::iterator, bool> ret =
        m_sessions.insert({session_id, SessionInfo()});
    if (false == ret.second) {
        _LOG_LAST_ERROR("the session %ld is existed", session_id);
        return kSESSION_ALREADY_EXISTED;
    }

    TimeoutCallback cb = cxx::bind(&SessionMgr::OnTimeout, this, session_id, session);
    int64_t timerid = m_timer->StartTimer(timeout_ms, cb);
    if (timerid < 0) {
        m_sessions.erase(ret.first);
        _LOG_LAST_ERROR("start timer failed(%s)", m_timer->GetLastError());
        return kSESSION_START_TIMER_FAILED;
    }

    ret.first->second.m_session    = session;
    ret.first->second.m_timer_id   = timerid;
    ret.first->second.m_timeout_ms = timeout_ms;

    return 0;
}

Session* SessionMgr::GetSession(int64_t session_id) {
    cxx::unordered_map<int64_t, SessionInfo>::iterator it = m_sessions.find(session_id);
    if (m_sessions.end() == it) {
        _LOG_LAST_ERROR("the session %ld not exist", session_id);
        return NULL;
    }

    return it->second.m_session;
}

int32_t SessionMgr::RemoveSession(int64_t session_id) {
    cxx::unordered_map<int64_t, SessionInfo>::iterator it = m_sessions.find(session_id);
    if (m_sessions.end() == it) {
        _LOG_LAST_ERROR("the session %ld not exist", session_id);
        return kSESSION_UNEXISTED;
    }

    m_timer->StopTimer(it->second.m_timer_id);
    m_sessions.erase(it);

    return 0;
}

int32_t SessionMgr::CheckTimeout() {
    return m_timer->Update();
}

int32_t SessionMgr::RestartTimer(int64_t session_id, uint32_t new_timeout_ms) {
    cxx::unordered_map<int64_t, SessionInfo>::iterator it = m_sessions.find(session_id);
    if (m_sessions.end() == it) {
        _LOG_LAST_ERROR("the session %ld not exist", session_id);
        return kSESSION_UNEXISTED;
    }

    TimeoutCallback cb = cxx::bind(&SessionMgr::OnTimeout, this, session_id, it->second.m_session);
    int32_t timeout_ms = (0 == new_timeout_ms) ? it->second.m_timeout_ms : new_timeout_ms;
    int64_t timer_id   = m_timer->StartTimer(timeout_ms, cb);
    if (timer_id < 0) {
        _LOG_LAST_ERROR("start timer failed(%s)", m_timer->GetLastError());
        return kSESSION_START_TIMER_FAILED;
    }

    // 新的定时器启动成功后再停老的
    m_timer->StopTimer(it->second.m_timer_id);
    it->second.m_timer_id   = timer_id;
    it->second.m_timeout_ms = timeout_ms;

    return 0;
}

int32_t SessionMgr::OnTimeout(int64_t session_id, Session* session) {
    int32_t ret = session->OnTimeout(session_id);

    // ret <0 超时后session被remove
    // ret =0 超时后session仍然保持，重新计时
    // ret >0 超时后session仍然保持，使用返回值作为超时时间(ms)重新计时
    if (ret < 0) {
        m_sessions.erase(session_id);
    }

    return ret;
}

} // namespace pebble
