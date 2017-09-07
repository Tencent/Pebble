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


#ifndef _PEBBLE_COMMON_NET_UTIL_H_
#define _PEBBLE_COMMON_NET_UTIL_H_

#include <list>
#include <string>
#include <sys/epoll.h>

namespace pebble {

int GetIpByIf(const char* if_name, std::string* ip);

//////////////////////////////////////////////////////////////////////////////////////

typedef uint64_t NetAddr;
static const NetAddr INVAILD_NETADDR = UINT64_MAX;
class NetIO;

#define NETADDR_IP_PRINT_FMT   "%u.%u.%u.%u:%u"
#define NETADDR_IP_PRINT_CTX(socket_info) \
    (socket_info->_ip & 0xFFU), ((socket_info->_ip >> 8) & 0xFFU), \
    ((socket_info->_ip >> 16) & 0xFFU), ((socket_info->_ip >> 24) & 0xFFU), \
    (((socket_info->_port & 0xFF) << 8) | ((socket_info->_port >> 8) & 0xFF))

uint64_t NetAddressToUIN(const std::string& ip, uint16_t port);

void UINToNetAddress(uint64_t net_uin, std::string* ip, uint16_t* port);

void SetLogOut(FILE* log_fp);

#define NET_LOG_DEBUG_LEVEL 0
#define NET_LOG_INFO_LEVEL 1
#define NET_LOG_ERROR_LEVEL 2
void SetLogLevel(int32_t log_level);

static const uint8_t TCP_PROTOCOL = 0x80;   ///< Protocol: tcp协议
static const uint8_t UDP_PROTOCOL = 0x40;   ///< Protocol: udp协议
static const uint8_t IN_BLOCKED = 0x10;     ///< 阻塞中
static const uint8_t ADDR_TYPE = 0x07;      ///< AddrType: 地址类型
static const uint8_t CONNECT_ADDR = 0x04;   ///<           主动连接
static const uint8_t LISTEN_ADDR = 0x02;    ///<           监听
static const uint8_t ACCEPT_ADDR = 0x01;    ///<           被动连接

struct SocketInfo
{
    void Reset();

    uint8_t GetProtocol() const { return (_state & 0xC0); }
    uint8_t GetAddrType() const { return (_state & 0x7); }

    int32_t _socket_fd;
    uint32_t _addr_info;                    ///< _addr_info accept时本地监听的地址信息，TCP的主动连接时为可自动重连次数
    uint32_t _ip;
    uint16_t _port;
    uint8_t _state;
    uint8_t _uin;                           ///< 复用时的标记
};

class Epoll
{
    friend class NetIO;
public:
    Epoll();
    ~Epoll();

    int32_t Init(uint32_t max_event);

    /// @brief 等待事件触发
    /// @return >=0 触发的事件数
    /// @return -1 错误，原因见errno
    int32_t Wait(int32_t timeout_ms);

    int32_t AddFd(int32_t fd, uint32_t events, uint64_t data);

    int32_t DelFd(int32_t fd);

    int32_t ModFd(int32_t fd, uint32_t events, uint64_t data);

    /// @brief 获取事件
    /// @return 0 获取成功
    /// @return -1 获取失败，没有事件信息
    int32_t GetEvent(uint32_t *events, uint64_t *data);

    const char* GetLastError() const {
        return m_last_error;
    }

private:
    char    m_last_error[256];
    int32_t m_epoll_fd;
    int32_t m_max_event;
    int32_t m_event_num;
    struct epoll_event* m_events;
    NetIO*  m_bind_net_io;
};

/// @brief 网络IO处理类，管理socket、封装网络操作
/// @note 使用地址忽略socket，上层不用考虑重连
class NetIO
{
    friend class Epoll;
public:
    NetIO();
    ~NetIO();

    /// @brief 初始化
    int32_t Init(Epoll* epoll);

    /// @brief 打开监听
    /// @note 主动创建的连接异常时会自动尝试恢复
    NetAddr Listen(const std::string& ip, uint16_t port);

    /// @brief 接受服务连接
    /// @note 被动打开的连接在连接异常时会自动释放
    NetAddr Accept(NetAddr listen_addr);

    /// @brief 连接地址
    /// @note 主动创建的连接异常时会自动尝试恢复
    NetAddr ConnectPeer(const std::string& ip, uint16_t port);

    /// @brief 发送数据
    /// @note 只负责发送，不保存任何发送残渣数据
    int32_t Send(NetAddr dst_addr, const char* data, uint32_t data_len);

    /// @brief 发送数据
    /// @note 只负责发送，不保存任何发送残渣数据
    int32_t SendV(NetAddr dst_addr, uint32_t data_num,
                  const char* data[], uint32_t data_len[], uint32_t offset = 0);

    /// @brief 发送数据
    /// @note only for udp server send rsp
    int32_t SendTo(NetAddr local_addr, NetAddr remote_addr, const char* data, uint32_t data_len);

    /// @brief 接收数据
    /// @return -1 读取失败或连接关闭了，错误见errno
    /// @return >= 0 读取的内容长度，可能为空
    /// @note 只负责发送，不保存任何接收的残渣数据
    int32_t Recv(NetAddr dst_addr, char* buff, uint32_t buff_len);

    /// @brief 接收数据
    /// @note only for udp listen
    int32_t RecvFrom(NetAddr local_addr, NetAddr* remote_addr, char* buff, uint32_t buff_len);

    /// @brief 关闭连接
    /// @return -1 关闭后返回错误，错误原因见errno
    /// @return 0  连接关闭了
    /// @note 失败的意义同close
    int32_t Close(NetAddr dst_addr);

    /// @brief 重置连接(先关后连，暂时只针对CONNECT_ADDR类型)
    /// @return -1 关闭后返回错误，错误原因见errno
    /// @return 0  重置成功
    int32_t Reset(NetAddr dst_addr); // TODO: -> ReConnect

    /// @brief 关闭所有连接
    void CloseAll();

    /// @brief 获取地址的socket相关信息
    /// @return 非NULL对象指针
    /// @note 地址不存在时返回的Info中数据全为0
    const SocketInfo* GetSocketInfo(NetAddr dst_addr) const;

    /// @brief 获取监听地址的socket相关信息
    /// @return 非NULL对象指针
    /// @note 地址不存在或地址不是被动连接的地址时，返回的Info中数据全为0
    /// @note 用于被动连接的地址获取关联的本地的监听地址信息
    const SocketInfo* GetLocalListenSocketInfo(NetAddr dst_addr) const;

    /// @brief 获取监听地址的句柄信息
    NetAddr GetLocalListenAddr(NetAddr dst_addr);

    const char* GetLastError() const {
        return m_last_error;
    }

    // 配置项
    static bool NON_BLOCK;              ///< NON_BLOCK 是否为非阻塞读写，默认为true
    static bool ADDR_REUSE;             ///< ADDR_REUSE 是否打开地址复用，默认为true
    static bool KEEP_ALIVE;             ///< KEEP_ALIVE 是否打开连接定时活性检测，默认为true
    static bool USE_NAGLE;              ///< USE_NAGLE 是否使用nagle算法合并小包，默认为false
    static bool USE_LINGER;             ///< USE_LINGER 是否使用linger延时关闭连接，默认为false
    static int32_t LINGER_TIME;         ///< LINGER_TIME 使用linget时延时时间设置，默认为0
    static int32_t LISTEN_BACKLOG;      ///< LISTEN_BACKLOG 监听的backlog队列长度，默认为10240
    static uint32_t MAX_SOCKET_NUM;     ///< MAX_SOCKET_NUM 最大的连接数，默认为1000000
    static uint8_t AUTO_RECONNECT;      ///< AUTO_RECONNECT 对TCP的主动连接自动重连，默认值为3

    // 常数
    static const uint32_t MAX_SENDV_DATA_NUM = 32;   ///< SendV接口最大发送的数据段数量

private:
    NetAddr AllocNetAddr();

    void FreeNetAddr(NetAddr net_addr);

    SocketInfo* RawGetSocketInfo(NetAddr net_addr);

    int32_t InitSocketInfo(const std::string& ip, uint16_t port, SocketInfo* socket_info);

    int32_t OnEvent(NetAddr net_addr, uint32_t events);

    int32_t RawListen(NetAddr net_addr, SocketInfo* socket_info);

    int32_t RawConnect(NetAddr net_addr, SocketInfo* socket_info);

    int32_t RawClose(SocketInfo* socket_info);

    char            m_last_error[256];

    Epoll           *m_epoll;

    NetAddr         m_used_id;
    SocketInfo      *m_sockets;
    std::list<NetAddr>  m_free_sockets;
};

} // namespace pebble

#endif // _PEBBLE_COMMON_NET_UTIL_H_
