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


#ifndef PEBBLE_NET_MESSAGEDRIVER_h
#define PEBBLE_NET_MESSAGEDRIVER_h

#include "source/common/pebble_common.h"
#include <stdint.h>
#include <string.h>
#include <string>

namespace pebble { namespace net {

    typedef cxx::function<int (const char* msg, size_t msg_len,
            uint64_t addr, int flag)> SendToFunction;

    typedef cxx::function<int (const char* msg, size_t msg_len,
            int flag)> SendFunction;

    typedef cxx::function<int (uint64_t peer_addr)> ClosePeerFunction;

    typedef cxx::function<int (int, const char*, size_t, uint64_t)> OnMessage;
    typedef cxx::function<int (int, uint64_t)> OnPeerConnected;
    typedef cxx::function<int (int, uint64_t)> OnPeerClosed;

    ///@brief 网络事件回调
    struct NetEventCallbacks {
        OnMessage on_message; ///< 对端消息 原型: int on_message(int handle, const char* msg,
                              ///< size_t msg_len, uint64_t peer_addr);注意，msg仅在callback内有效
        OnPeerConnected on_peer_connected;   ///< 对端连接 原型：int on_peer_connected(int handle,
                                             ///< uint64_t peer_addr)
        OnPeerClosed on_peer_closed;         ///< 对端关闭 原型：int on_peer_closed(int handle,
                                             ///< uint64_t peer_addr)
        OnMessage on_invalid_message;        ///< 无效消息 原型： int on_invalid_message
                                             ///< (int handle, const char *msg,
                                             ///<  size_t msg_len, uint64_t peer_addr)
    };

    /// @brief 网络驱动统一接口
    class MessageDriver {
        public:
            virtual ~MessageDriver() {}

            /// @brief 网络驱动统一绑定接口，绑定url，并设置具体的发送函数与关闭对端函数
            /// @param handle 由NewHandle函数返回的句柄
            /// @param url 指定的url
            /// @param send_to 发送函数指针，用于设置相应发送函数
            /// @param close_peer 关闭对端函数指针，用于设置相应关闭对端函数
            /// @return 0 表示成功
            /// @return 非0 表示失败
            virtual int Bind(int handle, const std::string &url,
                    SendToFunction *send_to,
                    ClosePeerFunction *close_peer) = 0;

            /// @brief 网络驱动统一连接接口，并设置具体发送函数
            /// @param handle 由NewHandle函数返回的句柄
            /// @param url 用于连接的url
            /// @param send 发送函数指针，用于设置相应发送函数
            /// @return 0 表示成功
            /// @return 非0 表示失败
            virtual int Connect(int handle, const std::string &url,
                    SendFunction *send) = 0;

            /// @brief 关闭句柄
            /// @param handle 由NewHandle函数返回的句柄
            /// @return 0 表示成功
            /// @return 非0 表示失败
            virtual int Close(int handle) = 0;

            /// @brief 获取网络事件，
            /// @param net_event_callbacks 发生网络事件时的相应处理函数
            /// @param max_event 最大处理事件个数
            /// @return 返回非负整数值，表示处理的事件数
            virtual int Poll(const NetEventCallbacks &net_event_callbacks,
                    int max_event) {
                return 0;
            }

            /// @brief 获取网络驱动类型
            /// @param 无
            /// @return 返回网络驱动类型名称
            virtual std::string Name() = 0;
    };

} /* namespace pebble */ } /* namespace net */

#endif // PEBBLE_NET_MESSAGEDRIVER_h
