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

#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

#include "common/net_util.h"
#include "common/string_utility.h"
#include "common/time_utility.h"
#include "framework/net_message.h"


namespace pebble {

/// @brief 封装一个连接，维护收发缓存及消息处理操作
class NetConnection {
public:
    NetConnection();
    ~NetConnection();
    /// @brief 初始化连接，主要是创建接收缓冲区
    /// @return 0 成功
    /// @return <0 失败
    int32_t Init(uint32_t buff_len, uint32_t msg_head_len);

    /// @brief cache新的发送数据，添加到发送队列尾部
    /// @return 0 成功
    /// @return <0 失败
    int32_t CacheSendData(const uint8_t* data, uint32_t data_len);

    /// @brief 在最后cache数据上append新的数据
    /// @return 0 成功
    /// @return <0 失败，失败时会清理掉当前数据
    int32_t AppendSendData(const uint8_t* data, uint32_t data_len);

    /// @brief 取出接收缓冲区的消息
    /// @return 0 成功
    /// @return <0 失败，可能是无数据或buff_len不够
    int32_t RecvMsg(uint8_t* buff, uint32_t* buff_len);

    /// @brief 窥探接收缓冲区的消息
    /// @return 0 成功
    /// @return <0 失败，可能是无数据
    int32_t PeekMsg(const uint8_t** msg, uint32_t* msg_len);

    /// @brief 清理接收缓存，和PeekMsg为配对操作
    /// @return 0 成功
    /// @return <0 失败，可能是无数据
    int32_t PopMsg();

    /// @brief 是否有新的消息
    bool HasNewMsg();

    // 每个连接维护一个接收缓冲区，每次只收取一个完整包并持有，直至上层用户消费掉，然后再开始接收新的数据
    uint8_t* _buff;         // 接收缓冲区
    uint32_t _buff_len;     // 接收缓冲区大小
    uint32_t _msg_head_len; // 消息头长度，用于TCP分包

    uint32_t _recv_len;     // 已经接收的数据长度
    uint32_t _cur_msg_len;  // 待接收消息的总长度，这个长度由上层用户解析消息头后给出，连接根据这个去收取一个完整包
    int64_t  _arrived_ms;   // 接收消息的时间戳
    uint64_t _peer_addr;    // 记录udp listen收到消息的远端地址

    // 每个连接维护发送消息列表，若一个消息未完全发送成功，需要缓存剩余数据，直至发送完毕
    struct Msg {
        Msg() : _msg_len(0), _msg(NULL) {}
        uint32_t _msg_len;
        uint8_t* _msg;  // 内存由外部维护
    };
    uint32_t _max_send_list_size;
    std::list<Msg> _send_msg_list; // 待发送的消息缓存，固定限制大小为1w
};

NetConnection::NetConnection() {
    _buff           = NULL;
    _buff_len       = 0;
    _msg_head_len   = 0;
    _recv_len       = 0;
    _cur_msg_len    = 0;
    _arrived_ms     = 0;
    _peer_addr      = INVAILD_HANDLE;
    _max_send_list_size = 10000;
}

NetConnection::~NetConnection() {
    free(_buff);
    for (std::list<Msg>::iterator it = _send_msg_list.begin();
        it != _send_msg_list.end(); ++it) {
        free((*it)._msg);
    }
}

int32_t NetConnection::Init(uint32_t buff_len, uint32_t msg_head_len) {
    if (_buff) {
        return -1;
    }
    if (buff_len == 0) {
        return -2;
    }
    _buff_len       = buff_len;
    _msg_head_len   = msg_head_len;
    _buff = (uint8_t*)malloc(_buff_len);
    if (_buff == NULL) {
        return -3;
    }
    return 0;
}

// 数据发送失败时，把未发送完成的数据cache起来，择机发送
int32_t NetConnection::CacheSendData(const uint8_t* data, uint32_t data_len) {
    if (data_len == 0 || data == NULL) {
        return -1;
    }
    if (_send_msg_list.size() > _max_send_list_size) {
        return -2;
    }

    Msg msg;
    msg._msg_len = data_len;
    msg._msg     = (uint8_t*)malloc(data_len);
    if (msg._msg == NULL) {
        return -3;
    }

    memcpy(msg._msg, data, data_len);
    _send_msg_list.push_back(msg);
    return 0;
}

int32_t NetConnection::AppendSendData(const uint8_t* data, uint32_t data_len) {
    if (_send_msg_list.empty()) {
        return -1;
    }
    Msg& msg = _send_msg_list.back();

    uint32_t new_len = msg._msg_len + data_len;
    uint8_t* new_msg = static_cast<uint8_t*>(realloc(msg._msg, new_len));
    if (NULL == new_msg) {
        // append数据失败，那么这个cache实际已经无效了，需要删除
        free(msg._msg);
        _send_msg_list.pop_back();
        return -2;
    }
    memcpy(new_msg + msg._msg_len, data, data_len);
    msg._msg     = new_msg;
    msg._msg_len = new_len;

    return 0;
}

int32_t NetConnection::RecvMsg(uint8_t* buff, uint32_t* buff_len) {
    // 有数据，且消息完整
    if (_recv_len > 0 && _recv_len == _cur_msg_len) {
        if (*buff_len < _cur_msg_len) {
            return -1;
        }
        memcpy(buff, _buff, _cur_msg_len);
        *buff_len = _cur_msg_len;

        // 清理接收数据
        _recv_len    = 0;
        _cur_msg_len = 0;
        return 0;
    }

    // 无数据或无完整消息
    return -2;
}

int32_t NetConnection::PeekMsg(const uint8_t** msg, uint32_t* msg_len) {
    // 有数据，且消息完整
    if (_recv_len > 0 && _recv_len == _cur_msg_len) {
        *msg = _buff;
        *msg_len = _cur_msg_len;
        return 0;
    }

    // 无数据或无完整消息
    return -1;
}

int32_t NetConnection::PopMsg() {
    // 有数据，且消息完整才清理
    if (_recv_len > 0 && _recv_len == _cur_msg_len) {
        _recv_len    = 0;
        _cur_msg_len = 0;
        return 0;
    }
    return -1;
}

bool NetConnection::HasNewMsg() {
    return (_recv_len > 0 && _recv_len == _cur_msg_len);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////

NetMessage::NetMessage() {
    m_last_error[0] = 0;
    m_epoll = NULL;
    m_netio = NULL;

    m_send_buff    = NULL;
    m_msg_head_len = 0;
    m_msg_buff_len = DEFAULT_MSG_BUFF_LEN;
    m_max_send_list_size = 10000;
}

NetMessage::~NetMessage() {
    CloseAllConnections();
    delete m_netio;
    delete m_epoll;
    free(m_send_buff);
}

int32_t NetMessage::Init(uint32_t msg_head_len, const GetMsgDataLen& get_msg_data_len_func,
    uint32_t msg_buff_len) {

    if (!get_msg_data_len_func) {
        _LOG_LAST_ERROR("get_msg_data_len_func is null");
        return kMESSAGE_INVAILD_PARAM;
    }

    m_msg_head_len = msg_head_len;
    m_get_msg_data_len_func = get_msg_data_len_func;
    if (msg_buff_len > 0) {
        m_msg_buff_len = msg_buff_len;
    }

    if (m_send_buff) {
        free(m_send_buff);
    }
    m_send_buff = (uint8_t*)malloc(m_msg_buff_len);

    int32_t ret = 0;
    if (!m_epoll) {
        m_epoll = new Epoll();
        ret = m_epoll->Init(1024);
        if (ret != 0) {
            _LOG_LAST_ERROR("%s", m_epoll->GetLastError());
            return kMESSAGE_EPOLL_INIT_FAILED;
        }
    }

    if (!m_netio) {
        m_netio = new NetIO();
        ret = m_netio->Init(m_epoll);
        if (ret != 0) {
            _LOG_LAST_ERROR("%s", m_netio->GetLastError());
            return kMESSAGE_NETIO_INIT_FAILED;
        }
    }

    return 0;
}

uint64_t NetMessage::Bind(const std::string& ip, uint16_t port) {
    NetAddr netaddr = m_netio->Listen(ip, port);
    if (INVAILD_NETADDR == netaddr) {
        _LOG_LAST_ERROR("listen %s:%d failed(%s)", ip.c_str(), port, m_netio->GetLastError());
        return kMESSAGE_BIND_ADDR_FAILED;
    }

    const SocketInfo* socket_info = m_netio->GetSocketInfo(netaddr);
    if (socket_info->_state & UDP_PROTOCOL) {
        // udp listen port需要创建connection对象
        if (CreateConnection(netaddr) == NULL) {
            m_netio->Close(netaddr);
            return INVAILD_HANDLE;
        }
    }

    return netaddr;
}

uint64_t NetMessage::Connect(const std::string& ip, uint16_t port) {
    NetAddr netaddr = m_netio->ConnectPeer(ip, port);
    if (INVAILD_NETADDR == netaddr) {
        _LOG_LAST_ERROR("connect %s:%d failed(%s)", ip.c_str(), port, m_netio->GetLastError());
        return kMESSAGE_CONNECT_ADDR_FAILED;
    }

    if (CreateConnection(netaddr) == NULL) {
        m_netio->Close(netaddr);
        return INVAILD_HANDLE;
    }

    return netaddr;
}

int32_t NetMessage::Send(uint64_t handle, const uint8_t* msg, uint32_t msg_len) {
    int32_t send_len = SendData(handle, msg, msg_len);

    if (send_len < 0) {
        return send_len;
    }

    if (send_len == (int32_t)msg_len) {
        return 0;
    }

    // 有数据未发完，需要缓存
    NetConnection* connection = GetConnection(handle);
    if (connection == NULL) {
        _LOG_LAST_ERROR("get connection %lu failed", handle);
        return kMESSAGE_CACHE_FAILED;
    }
    int32_t ret = connection->CacheSendData(msg + send_len, msg_len - send_len);
    if (ret != 0) {
        _LOG_LAST_ERROR("cache msg failed(%d), len = %d", ret, msg_len - send_len);
        return kMESSAGE_CACHE_FAILED;
    }

    return 0;
}

int32_t NetMessage::SendV(uint64_t handle, uint32_t msg_frag_num,
                          const uint8_t* msg_frag[], uint32_t msg_frag_len[]) {
    uint64_t send_handle = GetSendHandle(handle);
    uint32_t msg_len = 0;
    const SocketInfo* socket_info = m_netio->GetSocketInfo(send_handle);
    if (socket_info->_state & UDP_PROTOCOL) {
        // 分片的消息cp到一起
        for (uint32_t i = 0; i < msg_frag_num; i++) {
            if (msg_len + msg_frag_len[i] > m_msg_buff_len) {
                _LOG_LAST_ERROR("bufflen(%u) < msglen(%u) i=%d",
                    m_msg_buff_len, msg_len + msg_frag_len[i], i);
                return kMESSAGE_SEND_BUFF_NOT_ENOUGH;
            }
            memcpy(m_send_buff + msg_len, msg_frag[i], msg_frag_len[i]);
            msg_len += msg_frag_len[i];
        }

        return Send(handle, m_send_buff, msg_len);
    }

    // tcp
    int32_t send_len = m_netio->SendV(handle, msg_frag_num, (const char**)msg_frag, msg_frag_len);

    // 网络错误，关闭连接
    if (send_len < 0) {
        CloseConnection(handle);
        _LOG_LAST_ERROR("send to %lu failed(%s)", handle, m_netio->GetLastError());
        return kMESSAGE_SEND_FAILED;
    }

    // 计算总消息长度
    msg_len = 0;
    for (uint32_t i = 0; i < msg_frag_num; i++) {
        msg_len += msg_frag_len[i];
    }

    // 发送成功
    if (send_len == static_cast<int32_t>(msg_len)) {
        return 0;
    }

    // 有数据未发完，需要缓存
    int32_t ret = 0;
    bool cache = false;
    NetConnection* connection = GetConnection(handle);
    if (connection == NULL) {
        _LOG_LAST_ERROR("get connection %lu failed", handle);
        return kMESSAGE_CACHE_FAILED;
    }

    for (uint32_t i = 0; i < msg_frag_num; i++) {
        if (send_len >= static_cast<int32_t>(msg_frag_len[i])) {
            send_len -= msg_frag_len[i];
            continue;
        }

        if (!cache) {
            cache = true;
            ret = connection->CacheSendData(msg_frag[i] + send_len, msg_frag_len[i] - send_len);
        } else {
            ret = connection->AppendSendData(msg_frag[i] + send_len, msg_frag_len[i] - send_len);
        }

        if (ret != 0) {
            _LOG_LAST_ERROR("cache msg frags failed(%d) i=%d,len=%u.", ret, i, msg_frag_len[i]);
            return kMESSAGE_CACHE_FAILED;
        }
        send_len = 0;
    }

    return 0;
}

int32_t NetMessage::Recv(uint64_t handle, uint8_t* buff, uint32_t* buff_len,
                         MsgExternInfo* msg_info) {

    NetConnection* connection = GetConnection(handle);
    if (connection == NULL) {
        _LOG_LAST_ERROR("get connection %lu failed", handle);
        return kMESSAGE_UNKNOWN_CONNECTION;
    }

    int32_t ret = connection->RecvMsg(buff, buff_len);
    if (ret != 0) {
        _LOG_LAST_ERROR("uncompelte msg ret=%d", ret);
        return kMESSAGE_RECV_FAILED;
    }

    msg_info->_msg_arrived_ms = connection->_arrived_ms;
    msg_info->_remote_handle  = handle;
    msg_info->_self_handle    = handle;

    const SocketInfo* socket_info = m_netio->GetSocketInfo(handle);
    if (socket_info->_state & ACCEPT_ADDR) {
        msg_info->_self_handle = m_netio->GetLocalListenAddr(handle);
    }
    if ((socket_info->_state & UDP_PROTOCOL) && (socket_info->_state & LISTEN_ADDR)) {
        msg_info->_remote_handle = connection->_peer_addr;
    }

    return 0;
}

int32_t NetMessage::Peek(uint64_t handle,
    const uint8_t** msg, uint32_t* msg_len, MsgExternInfo* msg_info) {

    NetConnection* connection = GetConnection(handle);
    if (connection == NULL) {
        _LOG_LAST_ERROR("get connection %lu failed", handle);
        return kMESSAGE_UNKNOWN_CONNECTION;
    }

    int32_t ret = connection->PeekMsg(msg, msg_len);
    if (ret != 0) {
        _LOG_LAST_ERROR("uncompelte msg ret=%d", ret);
        return kMESSAGE_RECV_FAILED;
    }

    msg_info->_msg_arrived_ms = connection->_arrived_ms;
    msg_info->_remote_handle  = handle;
    msg_info->_self_handle    = handle;

    const SocketInfo* socket_info = m_netio->GetSocketInfo(handle);
    if (socket_info->_state & ACCEPT_ADDR) {
        msg_info->_self_handle = m_netio->GetLocalListenAddr(handle);
    }
    if ((socket_info->_state & UDP_PROTOCOL) && (socket_info->_state & LISTEN_ADDR)) {
        msg_info->_remote_handle = connection->_peer_addr;
    }

    return 0;
}

int32_t NetMessage::Pop(uint64_t handle) {
    NetConnection* connection = GetConnection(handle);
    if (connection == NULL) {
        _LOG_LAST_ERROR("get connection %lu failed", handle);
        return kMESSAGE_UNKNOWN_CONNECTION;
    }

    int32_t ret = connection->PopMsg();
    if (ret != 0) {
        _LOG_LAST_ERROR("uncompelte msg ret=%d", ret);
        return kMESSAGE_RECV_EMPTY;
    }

    return 0;
}

int32_t NetMessage::Close(uint64_t handle) {
    cxx::unordered_map<uint64_t, NetConnection*>::iterator it = m_connections.find(handle);
    if (it != m_connections.end()) {
        delete it->second;
        m_connections.erase(it);
    }
    m_netio->Close(handle);
    return 0;
}

int32_t NetMessage::PollConnectionBuffer(uint64_t* handle) {
    cxx::unordered_map<uint64_t, NetConnection*>::iterator it = m_connections.begin();
    for (; it != m_connections.end(); ++it) {
        if (it->second->HasNewMsg()) {
            *handle = it->first;
            return 1;
        }
    }
    return 0;
}

// handle一定是数据连接句柄，通过数据关系找到本地监听句柄
int32_t NetMessage::Poll(uint64_t* handle, int32_t* event, int32_t timeout_ms) {
    // 优先消费缓存消息
    int32_t num = PollConnectionBuffer(handle);
    if (num > 0) {
        return 0;
    }

    int32_t ret = m_epoll->Wait(timeout_ms);
    if (ret <= 0) {
        return -1;
    }

    uint32_t events  = 0;
    uint64_t netaddr = 0;
    ret = m_epoll->GetEvent(&events, &netaddr);
    if (ret != 0) {
        return -1;
    }

    if (events & EPOLLERR) {
        _LOG_LAST_ERROR("EPOLLERR get, %lu", netaddr);
        CloseConnection(netaddr);
        return kMESSAGE_GET_ERR_EVENT;
    }

    ret = -1;
    if (events & EPOLLOUT) {
        // 发送缓存数据
        if (SendCacheData(netaddr) > 0) {
            // ret = 0;
        }
    }

    const SocketInfo* socket_info = m_netio->GetSocketInfo(netaddr);
    if (events & EPOLLIN) {
        // 收包处理，区分TCP和UDP的收包逻辑
        if (socket_info->_state & TCP_PROTOCOL) {
            if (socket_info->_state & LISTEN_ADDR) {
                uint64_t peer_handle = m_netio->Accept(netaddr);
                if (peer_handle != INVAILD_NETADDR) {
                    CreateConnection(peer_handle);
                }
                return ret;
            }
            do {
                ret = RecvTcpData(netaddr);
            } while (ret == RECV_CONTINUE);
        } else {
            ret = RecvUdpData(netaddr);
        }

        if (ret < 0) {
            CloseConnection(netaddr);
            return kMESSAGE_RECV_FAILED;
        }

        ret = 0;
        *handle = netaddr;
    }

    return ret;
}

NetConnection* NetMessage::CreateConnection(uint64_t netaddr) {
    NetConnection* connection = new NetConnection();
    int32_t ret = connection->Init(m_msg_buff_len, m_msg_head_len);
    if (ret < 0) {
        delete connection;
        _LOG_LAST_ERROR("connection init failed %d", ret);
        return NULL;
    }

    connection->_max_send_list_size = m_max_send_list_size;

    if (m_connections.insert({netaddr, connection}).second == false) {
        _LOG_LAST_ERROR("connection insert %lu failed", netaddr);
        return NULL;
    }

    return connection;
}

NetConnection* NetMessage::GetConnection(uint64_t netaddr) {
    cxx::unordered_map<uint64_t, NetConnection*>::iterator it = m_connections.find(netaddr);
    if (it == m_connections.end()) {
        return NULL;
    }
    return it->second;
}

void NetMessage::CloseConnection(uint64_t netaddr) {
    // NetMessage只close accept连接，connect不关闭，尝试重连
    const SocketInfo* socket_info = m_netio->GetSocketInfo(netaddr);
    if (socket_info->_state & TCP_PROTOCOL && socket_info->_state & ACCEPT_ADDR) {
        Close(netaddr);
    }
}

void NetMessage::CloseAllConnections() {
    cxx::unordered_map<uint64_t, NetConnection*>::iterator it = m_connections.begin();
    for (; it != m_connections.end(); ++it) {
        delete it->second;
    }
    m_connections.clear();
    m_netio->CloseAll();
}

int32_t NetMessage::SendData(uint64_t handle, const uint8_t* msg, uint32_t msg_len) {
    int32_t send_len = 0;
    uint64_t send_handle = GetSendHandle(handle);
    const SocketInfo* socket_info = m_netio->GetSocketInfo(send_handle);
    if (socket_info->_state & TCP_PROTOCOL) {
        send_len = m_netio->Send(handle, (char*)msg, msg_len);
    } else {
        if (socket_info->_state & CONNECT_ADDR) { // udp protocol connect
            send_len = m_netio->Send(handle, (char*)msg, msg_len);
        } else { // udp protocol listen
            send_len = m_netio->SendTo(send_handle, handle, (char*)msg, msg_len);
        }
    }

    // 网络错误，关闭连接
    if (send_len < 0) {
        CloseConnection(handle);
        _LOG_LAST_ERROR("send to %lu failed(%s)", handle, m_netio->GetLastError());
        return kMESSAGE_SEND_FAILED;
    }

    // 发送成功
    if (send_len == (int32_t)msg_len) {
        return send_len;
    }

    // udp包未发完整，返回发送失败
    if ((socket_info->_state & UDP_PROTOCOL) && send_len > 0) {
        _LOG_LAST_ERROR("send udp pkg to %lu uncomplete(%s) sendlen=%u,msglen=%u",
            handle, m_netio->GetLastError(), send_len, msg_len);
        return kMESSAGE_SEND_FAILED;
    }

    return send_len;
}

int32_t NetMessage::SendCacheData(uint64_t netaddr) {
    NetConnection* connection = GetConnection(netaddr);
    if (connection == NULL) {
        _LOG_LAST_ERROR("get connection %lu failed", netaddr);
        return kMESSAGE_UNKNOWN_CONNECTION;
    }

    if (connection->_send_msg_list.empty()) {
        return 0;
    }

    // 每次暂时发一包
    NetConnection::Msg& msg = connection->_send_msg_list.front();
    int32_t send_len = SendData(netaddr, msg._msg, msg._msg_len);
    if (send_len < 0) {
        // 返回<0连接已经关闭，缓存已经清理
        return send_len;
    }

    if (send_len == (int32_t)msg._msg_len) {
        // 发送完整数据，清掉缓存
        connection->_send_msg_list.pop_front();
    } else if (send_len > 0) {
        // 发送部分数据，重新缓存
        uint8_t* new_buff = (uint8_t*)malloc(msg._msg_len - send_len);
        memcpy(new_buff, msg._msg + send_len, msg._msg_len - send_len);
        free(msg._msg);
        msg._msg = new_buff;
        msg._msg_len -= send_len;
    } // 发送0字节，缓存不变

    return send_len;
}

int32_t NetMessage::RecvTcpData(uint64_t netaddr) {
    // 每次收取1包，等待用户消费完后再收取下一包
    NetConnection* connection = GetConnection(netaddr);
    if (connection == NULL) {
        _LOG_LAST_ERROR("get connection %lu failed", netaddr);
        return kMESSAGE_UNKNOWN_CONNECTION;
    }

    if (connection->HasNewMsg()) {
        return RECV_END;
    }

    uint32_t old_len = connection->_recv_len; // 已经收取的数据长度
    uint32_t need_read = 0;     // 还需要读取的长度
    bool get_msg_len = false;   // 是否需要解析头获取数据部分长度
    if (old_len < m_msg_head_len) {
        need_read = m_msg_head_len - old_len;
        get_msg_len = true;
    } else {
        need_read = connection->_cur_msg_len - old_len;
    }

    int32_t recv_len = m_netio->Recv(netaddr, (char*)connection->_buff + old_len, need_read);
    if (recv_len < 0) {
        _LOG_LAST_ERROR("recv failed(%d:%s), netaddr=%lu", recv_len, m_netio->GetLastError(), netaddr);
        return -1;
    }
    connection->_recv_len += recv_len;
    if (recv_len < (int32_t)need_read) {
        // 未收完期望的数据，等待下次再收
        return RECV_END;
    }

    if (get_msg_len) {
        int32_t msg_data_len = m_get_msg_data_len_func(connection->_buff, m_msg_head_len);
        if (msg_data_len < 0) {
            _LOG_LAST_ERROR("para msg head failed(%d)", msg_data_len);
            return -1;
        }
        connection->_cur_msg_len = m_msg_head_len + msg_data_len;
        connection->_arrived_ms  = TimeUtility::GetCurrentMS();
    }

    if (connection->HasNewMsg()) {
        return RECV_END;
    }

    return RECV_CONTINUE;
}

int32_t NetMessage::RecvUdpData(uint64_t netaddr) {
    NetConnection* connection = GetConnection(netaddr);
    if (connection == NULL) {
        _LOG_LAST_ERROR("get connection %lu failed", netaddr);
        return kMESSAGE_UNKNOWN_CONNECTION;
    }

    uint64_t peer_addr = INVAILD_NETADDR;
    int32_t recv_len = 0;

    const SocketInfo* socket_info = m_netio->GetSocketInfo(netaddr);
    if (socket_info->_state & LISTEN_ADDR) {
        recv_len = m_netio->RecvFrom(netaddr, &peer_addr, (char*)connection->_buff, connection->_buff_len);
    } else {
        recv_len = m_netio->Recv(netaddr, (char*)connection->_buff, connection->_buff_len);
    }
    if (recv_len < 0) {
        _LOG_LAST_ERROR("recv failed(%d:%s), netaddr=%lu", recv_len, m_netio->GetLastError(), netaddr);
        return -1;
    }
    if (recv_len == 0) {
        return 0;
    }

    connection->_cur_msg_len = recv_len;
    connection->_recv_len    = recv_len;
    connection->_arrived_ms  = TimeUtility::GetCurrentMS();
    connection->_peer_addr   = peer_addr;

    if (peer_addr != INVAILD_NETADDR) {
        m_peer_handle_to_local[peer_addr] = netaddr;
    }

    return 1;
}

bool NetMessage::IsTcpTransport(uint64_t handle) {
    const SocketInfo* socket_info = m_netio->GetSocketInfo(handle);
    return socket_info->_state & TCP_PROTOCOL;
}

void NetMessage::SetMaxSendListSize(uint32_t max_send_list_size) {
    m_max_send_list_size = max_send_list_size;
}

uint64_t NetMessage::GetSendHandle(uint64_t netaddr) {
    cxx::unordered_map<uint64_t, uint64_t>::iterator it = m_peer_handle_to_local.find(netaddr);
    if (it != m_peer_handle_to_local.end()) {
        return it->second;
    }
    return netaddr;
}


} // namespace pebble
