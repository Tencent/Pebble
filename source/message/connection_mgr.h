// Copyright (c) 2015, Tencent Inc. All rights reserved.
#ifndef PEBBLE_NET_CONNECTION_MGR_H
#define PEBBLE_NET_CONNECTION_MGR_H

#include <list>
#include <map>
#include <stdint.h>
#include <utility>
#include "source/common/pebble_common.h"

namespace pebble { namespace net {

class Connection;

// 返回0表示处理，ConnectionMgr删除，非0表示不处理，重新置为run
typedef cxx::function<int(int handle, // NOLINT
    uint64_t peer_addr, int64_t session_id)> ConnectionIdle; // NOLINT

typedef struct tag_ConnectionEvent {
    ConnectionIdle onIdle;
} ConnectionEvent;

class ConnectionMgr {
public:
    ConnectionMgr();

    ~ConnectionMgr();

    // 连接建立等控制消息也算
    void OnMessage(int handle, uint64_t addr, int64_t session_id);

    // 剔除，不再监控
    void OnClose(int handle, uint64_t addr);

    void RegisterEventCb(const ConnectionEvent& events);

    int Update(int64_t cur_time_ms = -1);

    int SetIdleTimeout(int idle_timeout_s);

private:
    int m_idle_timeout_s; // idle超时时长，默认60s
    int64_t m_cur_time_ms;
    ConnectionEvent m_event_cbs;
    Connection* m_head;
    std::map<std::pair<int, uint64_t>, Connection*> m_conn_map;
};

} /* namespace pebble */ } /* namespace net */

#endif // PEBBLE_NET_CONNECTION_MGR_H
