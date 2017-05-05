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


#ifndef _PEBBLE_COMMON_NET_MESSAGE_H_
#define _PEBBLE_COMMON_NET_MESSAGE_H_

#include <list>
#include "framework/message.h"


namespace pebble {

class Epoll;
class NetIO;
class NetConnection;

#define INVAILD_HANDLE UINT64_MAX

/// @brief获取消息的数据部分长度，NetMessage从TCP接收到消息头部分后，
///     向上层用户询问此消息的数据长度，用于接收完整消息
/// @param head 消息头缓存
/// @param head_len 消息头长度
/// @return uint32_t 消息数据部分的长度(不包括消息头长度)，<0解码出错，应该关闭连接
typedef cxx::function<int32_t(const uint8_t* head, uint32_t head_len)> GetMsgDataLen;


/// @brief 封装tcp/udp收发消息功能，向上层用户提供基于消息的收发能力
class NetMessage {
public:
    NetMessage();
    ~NetMessage();

    /// @brief 每个连接默认的收发缓冲区大小，默认为2M
    static const int32_t DEFAULT_MSG_BUFF_LEN = 1024 * 1024 * 2;

    /// @param msg_head_len 由上层用户指定TCP发送时消息头的长度
    /// @param get_msg_data_len_func 当接收完消息头部分后，回调此函数得到消息数据部分的长度
    /// @param msg_buff_len TCP接收缓冲区大小，默认为2M
    int32_t Init(uint32_t msg_head_len, const GetMsgDataLen& get_msg_data_len_func,
        uint32_t msg_buff_len = DEFAULT_MSG_BUFF_LEN);

    /// @return 0 成功
    /// @return INVALID_HANDLE 失败
    uint64_t Bind(const std::string& ip, uint16_t port);

    /// @return 0 成功
    /// @return INVALID_HANDLE 失败
    uint64_t Connect(const std::string& ip, uint16_t port);

    /// @return 0 成功
    /// @return <0 失败
    int32_t Send(uint64_t handle, const uint8_t* msg, uint32_t msg_len);

    /// @return 0 成功
    /// @return <0 失败
    int32_t SendV(uint64_t handle, uint32_t msg_frag_num,
                          const uint8_t* msg_frag[], uint32_t msg_frag_len[]);

    /// @return 0 成功
    /// @return <0 失败
    int32_t Recv(uint64_t handle, uint8_t* buff, uint32_t* buff_len, MsgExternInfo* msg_info);

    /// @return 0 成功
    /// @return <0 失败
    int32_t Peek(uint64_t handle, const uint8_t** msg, uint32_t* msg_len, MsgExternInfo* msg_info);

    /// @return 0 成功
    /// @return <0 失败
    int32_t Pop(uint64_t handle);

    /// @return 0 成功
    /// @return <0 失败
    int32_t Close(uint64_t handle);

    /// @return 0 成功，有事件
    /// @return <0 失败，无事件或网络故障
    int32_t Poll(uint64_t* handle, int32_t* event, int32_t timeout_ms);

    /// @brief 是否TCP连接
    bool IsTcpTransport(uint64_t handle);

    const char* GetLastError() {
        return m_last_error;
    }

    /// @brief 设置发送缓冲区列表最大长度
    void SetMaxSendListSize(uint32_t max_send_list_size);

private:
    int32_t PollConnectionBuffer(uint64_t* handle);

    NetConnection* CreateConnection(uint64_t netaddr);

    NetConnection* GetConnection(uint64_t netaddr);

    void CloseConnection(uint64_t netaddr);

    void CloseAllConnections();

    /// @return >0 成功 发送消息数
    /// @return <0 失败 连接被关闭
    int32_t SendData(uint64_t handle, const uint8_t* msg, uint32_t msg_len);

    int32_t SendCacheData(uint64_t netaddr);

    int32_t RecvTcpData(uint64_t netaddr);

    int32_t RecvUdpData(uint64_t netaddr);

    uint64_t GetSendHandle(uint64_t netaddr);

private:
    enum {
        RECV_CONTINUE = 0,
        RECV_END,
    };

private:
    char   m_last_error[256];
    Epoll* m_epoll;
    NetIO* m_netio;

    uint8_t* m_send_buff; // 对使用udp发送多片消息时使用buff组装完整包
    uint32_t m_msg_buff_len;

    uint32_t m_max_send_list_size;

    uint32_t m_msg_head_len;
    GetMsgDataLen m_get_msg_data_len_func;

    // 连接数据
    cxx::unordered_map<uint64_t, NetConnection*> m_connections;

    // udp <peer handle, local listen handle> map
    cxx::unordered_map<uint64_t, uint64_t> m_peer_handle_to_local;
};


} // namespace pebble

#endif // _PEBBLE_COMMON_NET_MESSAGE_H_
