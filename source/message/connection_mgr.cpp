// Copyright (c) 2015, Tencent Inc. All rights reserved.
#include <stdio.h>
#include "connection_mgr.h"
#include "source/common/time_utility.h"
#include "source/message/message.h"


namespace pebble { namespace net {

class Connection {
public:
    Connection* m_prev;
    Connection* m_next;

    int m_handle;
    uint64_t m_peer_addr;
    int64_t m_session_id;
    int64_t m_active_time_ms;

    Connection() : m_prev(NULL), m_next(NULL), m_handle(0),
        m_peer_addr(0), m_session_id(-1), m_active_time_ms(0) {}
};

int list_push_back(Connection* head, Connection* item)
{
    if (NULL == head->m_next) {
        // 空表
        item->m_prev = head;
        item->m_next = NULL;

        head->m_next = item;
        head->m_prev = item;
        return 0;
    }

    item->m_prev = head->m_prev;
    item->m_next = NULL;

    head->m_prev->m_next = item;
    head->m_prev = item;
    return 0;
}

int list_remove(Connection* head, Connection* item)
{
    Connection* p_item = item->m_prev;
    Connection* n_item = item->m_next;

    p_item->m_next = n_item;
    if (n_item) {
        n_item->m_prev = p_item;
    } else {
        if (p_item == head) {
            head->m_prev = NULL;
        } else {
            head->m_prev = p_item;
        }
    }

    return 0;
}

int list_move_back(Connection* head, Connection* item)
{
    list_remove(head, item);
    list_push_back(head, item);
    return 0;
}

ConnectionMgr::ConnectionMgr()
    : m_idle_timeout_s(100), m_cur_time_ms(0), m_head(new Connection())
{
}

ConnectionMgr::~ConnectionMgr()
{
    delete m_head;
    m_head = NULL;
}

void ConnectionMgr::OnMessage(int handle, uint64_t addr, int64_t session_id)
{
    if (!m_event_cbs.onIdle) {
        return;
    }

    std::pair<int, uint64_t> key = std::make_pair(handle, addr);
    std::map<std::pair<int, uint64_t>, Connection*>::iterator it = m_conn_map.find(key);
    Connection* conn = NULL;
    if (m_conn_map.end() == it) {
        conn = new Connection();
        conn->m_handle = handle;
        conn->m_peer_addr = addr;
        conn->m_session_id = session_id;
        conn->m_active_time_ms = m_cur_time_ms;

        if (list_push_back(m_head, conn) != 0) {
            delete conn;
            return;
        }

        m_conn_map[key] = conn;
        return;
    }

    conn = it->second;
    if (!conn) {
        return;
    }

    if (conn->m_handle != handle || conn->m_peer_addr != addr) {
        fprintf(stderr, "error, %d != %d || %lu != %lu\n",
            conn->m_handle, handle, conn->m_peer_addr, addr); // PLOG in 0.1.6
    }

    conn->m_active_time_ms = m_cur_time_ms;
    conn->m_session_id = session_id;
    list_move_back(m_head, conn);
}

void ConnectionMgr::OnClose(int handle, uint64_t addr)
{
    if (!m_event_cbs.onIdle) {
        return;
    }

    std::pair<int, uint64_t> key = std::make_pair(handle, addr);
    std::map<std::pair<int, uint64_t>, Connection*>::iterator it = m_conn_map.find(key);
    if (m_conn_map.end() == it) {
        return;
    }
    Connection* conn = it->second;
    list_remove(m_head, conn);
    m_conn_map.erase(it);
    delete conn;
}

void ConnectionMgr::RegisterEventCb(const ConnectionEvent& events)
{
    m_event_cbs = events;
}

int ConnectionMgr::Update(int64_t cur_time_ms)
{
    if (!m_event_cbs.onIdle) {
        return -1;
    }

    if (-1 == cur_time_ms) {
        m_cur_time_ms = TimeUtility::GetCurremtMs();
    } else {
        m_cur_time_ms = cur_time_ms;
    }

    Connection* conn = m_head->m_next;
    Connection* tmp  = NULL;
    std::map<std::pair<int, uint64_t>, Connection*>::iterator it;
    while (conn) {
        if (conn->m_active_time_ms + m_idle_timeout_s * 1000 > m_cur_time_ms) {
            break;
        }

        tmp = conn->m_next;

        if (m_event_cbs.onIdle(conn->m_handle, conn->m_peer_addr, conn->m_session_id) == 0) {
            // 用户处理成功
            list_remove(m_head, conn);

            Message::ClosePeer(conn->m_handle, conn->m_peer_addr);

            it = m_conn_map.find(std::make_pair(conn->m_handle, conn->m_peer_addr));
            if (it != m_conn_map.end()) {
                m_conn_map.erase(it);
            }

            delete conn;
            conn = NULL;
        } else {
            // 用户不释放，重启
            conn->m_active_time_ms = m_cur_time_ms;
            list_move_back(m_head, conn);
        }

        conn = tmp;
    }

    return 0;
}

int ConnectionMgr::SetIdleTimeout(int idle_timeout_s)
{
    if (idle_timeout_s <= 0) {
        return -1;
    }

    m_idle_timeout_s = idle_timeout_s;
    return 0;
}



} /* namespace pebble */ } /* namespace net */

