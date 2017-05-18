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


#ifndef _PEBBLE_COMMON_MESSAGE_H_
#define _PEBBLE_COMMON_MESSAGE_H_

#include <string>
#include "common/platform.h"
#include "common/error.h"

namespace pebble {

typedef enum {
    kMESSAGE_UNINSTALL_DRIVER       =   MESSAGE_ERROR_CODE_BASE,        ///< 未安装通信驱动
    kMESSAGE_INVAILD_PARAM          =   MESSAGE_ERROR_CODE_BASE - 1,    ///< 参数非法
    kMESSAGE_ADDRESS_NOT_EXIST      =   MESSAGE_ERROR_CODE_BASE - 2,    ///< 地址不存在
    kMESSAGE_BIND_ADDR_FAILED       =   MESSAGE_ERROR_CODE_BASE - 3,    ///< 绑定地址失败
    kMESSAGE_CONNECT_ADDR_FAILED    =   MESSAGE_ERROR_CODE_BASE - 4,    ///< 连接地址失败
    kMESSAGE_ON_DISCONNECTED        =   MESSAGE_ERROR_CODE_BASE - 5,    ///< 连接断开
    kMESSAGE_RECV_INVAILD_MSG       =   MESSAGE_ERROR_CODE_BASE - 6,    ///< 收到非法消息
    kMESSAGE_RECV_BUFF_NOT_ENOUGH   =   MESSAGE_ERROR_CODE_BASE - 7,    ///< 接收缓存不够
    kMESSAGE_RECV_EMPTY             =   MESSAGE_ERROR_CODE_BASE - 8,    ///< 收到空消息
    kMESSAGE_EPOLL_INIT_FAILED      =   MESSAGE_ERROR_CODE_BASE - 9,    ///< epoll init失败
    kMESSAGE_NETIO_INIT_FAILED      =   MESSAGE_ERROR_CODE_BASE - 10,   ///< netio init失败
    kMESSAGE_SEND_FAILED            =   MESSAGE_ERROR_CODE_BASE - 11,   ///< send失败
    kMESSAGE_RECV_FAILED            =   MESSAGE_ERROR_CODE_BASE - 12,   ///< recv失败
    kMESSAGE_UNSUPPORT              =   MESSAGE_ERROR_CODE_BASE - 13,   ///< 不支持接口
    kMESSAGE_GET_EVENT_FAILED       =   MESSAGE_ERROR_CODE_BASE - 14,   ///< 获取事件失败
    kMESSAGE_GET_ERR_EVENT          =   MESSAGE_ERROR_CODE_BASE - 15,   ///< 获取到ERR事件
    kMESSAGE_CACHE_FAILED           =   MESSAGE_ERROR_CODE_BASE - 16,   ///< cache失败
    kMESSAGE_SEND_BUFF_NOT_ENOUGH   =   MESSAGE_ERROR_CODE_BASE - 17,   ///< 发送缓存区不够
    kMESSAGE_RECV_INVALID_DATA      =   MESSAGE_ERROR_CODE_BASE - 18,   ///< 收到非法消息
    kMESSAGE_UNKNOWN_CONNECTION     =   MESSAGE_ERROR_CODE_BASE - 19,   ///< 未知的连接
}MessageErrorCode;

class MessageErrorStringRegister {
public:
    static void RegisterErrorString() {
        SetErrorString(kMESSAGE_UNINSTALL_DRIVER, "no message dirver installed");
        SetErrorString(kMESSAGE_INVAILD_PARAM, "invalid paramater");
        SetErrorString(kMESSAGE_ADDRESS_NOT_EXIST, "address not exist");
        SetErrorString(kMESSAGE_BIND_ADDR_FAILED, "bind failed");
        SetErrorString(kMESSAGE_CONNECT_ADDR_FAILED, "connect failed");
        SetErrorString(kMESSAGE_ON_DISCONNECTED, "connection disconnected");
        SetErrorString(kMESSAGE_RECV_INVAILD_MSG, "receive invalid message");
        SetErrorString(kMESSAGE_RECV_BUFF_NOT_ENOUGH, "receive buffer not enough");
        SetErrorString(kMESSAGE_RECV_EMPTY, "receive empty message");
        SetErrorString(kMESSAGE_EPOLL_INIT_FAILED, "epoll init failed");
        SetErrorString(kMESSAGE_NETIO_INIT_FAILED, "netio init failed");
        SetErrorString(kMESSAGE_SEND_FAILED, "send failed");
        SetErrorString(kMESSAGE_RECV_FAILED, "receive failed");
        SetErrorString(kMESSAGE_UNSUPPORT, "unsupport interface");
        SetErrorString(kMESSAGE_GET_EVENT_FAILED, "get net event failed");
        SetErrorString(kMESSAGE_GET_ERR_EVENT, "geted ERR event");
        SetErrorString(kMESSAGE_CACHE_FAILED, "cache message failed");
        SetErrorString(kMESSAGE_SEND_BUFF_NOT_ENOUGH, "send buffer not enough");
        SetErrorString(kMESSAGE_RECV_INVALID_DATA, "receive invalid message");
        SetErrorString(kMESSAGE_UNKNOWN_CONNECTION, "unknown connection");
    }
};


/// @brief 消息相关信息，message收到消息后除了递交消息本身到业务模块外，还需提供消息附属信息
///     这个附属信息在上层各业务模块间流动，业务模块按需使用这些信息
struct MsgExternInfo {
    MsgExternInfo() {
        _self_handle    = -1;
        _remote_handle  = -1;
        _msg_arrived_ms = 0;
        _src            = NULL;
    }
    MsgExternInfo(const MsgExternInfo& rhs) {
        _self_handle    = rhs._self_handle;
        _remote_handle  = rhs._remote_handle;
        _msg_arrived_ms = rhs._msg_arrived_ms;
        _src            = rhs._src;
    }

    int64_t         _self_handle;       // bind或connect获得的handle
    int64_t         _remote_handle;     // 远端handle
    // int64_t         _channel;
    int64_t         _msg_arrived_ms;    // 消息到达时间

    void*           _src;               // 消息源，由消息分发模块填写，方便消息在各模块间往返
};

/// @brief 网络驱动接口
class MessageDriver {
public:
    virtual ~MessageDriver() {}

    virtual int64_t Bind(const std::string& url) = 0;

    virtual int64_t Connect(const std::string& url) = 0;

    virtual int32_t Send(int64_t handle, const uint8_t* msg, uint32_t msg_len, int32_t flag) = 0;

    virtual int32_t SendV(int64_t handle, uint32_t msg_frag_num,
                          const uint8_t* msg_frag[], uint32_t msg_frag_len[], int32_t flag) = 0;

    virtual int32_t Recv(int64_t handle, uint8_t* msg_buff, uint32_t* buff_len,
                         MsgExternInfo* msg_info) = 0;

    virtual int32_t Peek(int64_t handle, const uint8_t** msg, uint32_t* msg_len,
                         MsgExternInfo* msg_info) = 0;

    virtual int32_t Pop(int64_t handle) = 0;

    virtual int32_t ReportHandleResult(int64_t handle, int32_t result, int64_t time_cost) = 0;

    virtual int32_t Close(int64_t handle) = 0;

    virtual int32_t Poll(int64_t* handle, int32_t* event, int32_t timeout) = 0;

    virtual int32_t GetUsedSize(int64_t handle, uint32_t* remain_size, uint32_t* max_size) = 0;

    virtual const char* GetLastError() = 0;
};

/// @brief 基于消息的通讯接口类
class Message {
public:
    // -------------------message api begin-------------------------

    /// @brief 初始化
    static int32_t Init();

    // TODO: url规范需要统一
    /// @brief （服务端）把一个句柄绑定到指定url
    /// @param url 指定url，形式类似：
    ///     "tbuspp://1000.unit_wx.query/inst0",
    ///     "tbus://11.0.0.1",
    ///     "http://127.0.0.1:8880[/service]"
    /// @return >=0 表示成功
    /// @return <0 表示失败，错误码@see MessageErrorCode
    static int64_t Bind(const std::string &url);

    /// @brief (客户端)连接到指定url
    /// @param url 指定url，形式类似：
    ///     "tbuspp://1000.unit_wx.query[/inst0]",
    ///     "tbus://11.0.0.1",
    ///     "http://127.0.0.1:8880[/service]"
    /// @return >=0 表示成功
    /// @return <0 表示失败，错误码@see MessageErrorCode
    static int64_t Connect(const std::string &url);

    /// @brief 发送消息
    /// @param handle 由Bind或Connect或Recv返回的句柄
    /// @param msg 要发送的消息
    /// @param msg_len 消息的长度
    /// @param flag 可选参数，默认为0
    /// @return 0 发送成功
    /// @return <0 表示失败，错误码@see MessageErrorCode
    static int32_t Send(int64_t handle, const uint8_t* msg, uint32_t msg_len, int32_t flag = 0);

    /// @brief 发送消息
    /// @param handle 由Bind或Connect或Recv返回的句柄
    /// @param msg_frag_num 要发送的消息段数量
    /// @param msg_frag 要发送的消息段
    /// @param msg_frag_len 消息段的长度
    /// @param flag 可选参数，默认为0
    /// @return 0 发送成功
    /// @return <0 表示失败，错误码@see MessageErrorCode
    static int32_t SendV(int64_t handle, uint32_t msg_frag_num,
                         const uint8_t* msg_frag[], uint32_t msg_frag_len[], int flag = 0);

    /// @brief 接收消息
    /// @param handle 接收消息的句柄
    /// @param msg_buff 接收消息的BUFF
    /// @param buff_len 接收消息的BUFF长度，并返回实际的消息长度
    /// @param remote 接收的消息来源
    /// @return 0 发送成功
    /// @return <0 表示失败，错误码@see MessageErrorCode
    static int32_t Recv(int64_t handle, uint8_t* msg_buff, uint32_t* buff_len,
                        MsgExternInfo* msg_info = NULL);

    /// @brief 查看第一条消息但不取出
    /// @param handle 接收消息的句柄
    /// @param msg 返回的消息指针
    /// @param msg_len 返回的消息长度
    /// @param remote 接收的消息来源
    /// @return 0 发送成功
    /// @return <0 表示失败，错误码@see MessageErrorCode
    static int32_t Peek(int64_t handle, const uint8_t* *msg, uint32_t* msg_len,
                        MsgExternInfo* msg_info = NULL);

    /// @brief 将第一条消息取出
    /// @param handle 取出的消息句柄
    /// @return 0 发送成功
    /// @return <0 表示失败，错误码@see MessageErrorCode
    static int32_t Pop(int64_t handle);

    /// @brief 关闭句柄
    /// @param handle 由Bind或Connect或Recv返回的句柄
    /// @return 0 表示成功
    /// @return <0 表示失败，错误码@see MessageErrorCode
    static int32_t Close(int64_t handle);

    /// @brief 获取网络事件
    /// @param handle 触发事件的句柄
    /// @param event 触发的事件
    /// @param timeout poll等待的最大时间，单位ms
    /// @return 0 等到事件
    /// @return -1 等待超时
    static int32_t Poll(int64_t* handle, int32_t* event, int32_t timeout);

    /// @brief 上报网络质量
    /// @param handle 句柄
    /// @param result 上报返回值
    /// @param time_cost 上报时间消耗
    /// @return 0 成功
    /// @return <0 表示失败，错误码@see MessageErrorCode
    static int32_t ReportHandleResult(int64_t handle, int32_t result, int64_t time_cost);

    /// @brief 获取句柄对应的通道的使用情况
    /// @param handle 句柄
    /// @param remain_size 返回剩余可用的大小
    /// @param total_size 返回配置可用的大小
    /// @return 0 查询返回成功
    /// @return <0 表示失败，错误码@see MessageErrorCode
    static int32_t GetUsedSize(int64_t handle, uint32_t* remain_size, uint32_t* max_size);

    /// @brief 返回错误信息
    /// @param 无
    /// @return 返回错误描述
    static const char* GetLastError();

    // -------------------network api end-------------------------
public:
    /// @brief 设置通信驱动(通信库)，运行时只支持一种通信驱动，如rawudp，tbuspp或第3方网络库
    /// @param driver 对MessageDriver接口实现的网络库
    /// @return 0 表示成功
    /// @return -1 表示失败
    static void SetMessageDriver(MessageDriver* driver);

private:
    static MessageDriver* m_driver;
};

} // namespace pebble

#endif // _PEBBLE_COMMON_MESSAGE_H_
