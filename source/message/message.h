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


#ifndef PEBBLE_NET_MESSAGE_h
#define PEBBLE_NET_MESSAGE_h

#include "source/common/pebble_common.h"
#include "message_driver.h"
#include <map>
#include <queue>
#include <set>
#include "common.h"

namespace pebble { namespace net {
    /// @brief 基于消息的通讯接口类
    class Message {
        public:
            // -------------------network api begin-------------------------

            /// @brief 创建一个名柄
            /// @param 无
            /// @return 成功：非负整数
            /// @return 失败：负数
            static int NewHandle();

            /// @brief （服务端）把一个句柄绑定到指定url
            /// @param handle 由NewHandle返回的句柄
            /// @param url 指定url,形式类似"tbus://11.0.0.1:8080","pipe://11.0.0.1:80",
            /// "http://127.0.0.1:8880","tcp://127.0.0.1:8899"
            /// @return 0 表示成功
            /// @return 非0 表示失败
            static int Bind(int handle, const std::string &url);

            /// @brief (客户端)连接到指定url
            /// @param handle 由NewHandle返回的句柄
            /// @param url 指定url,形式类似"tbus://11.0.0.1:8080","pipe://11.0.0.1:80"
            /// "http://127.0.0.1:8880","tcp://127.0.0.1:8899"
            /// @return 0 表示成功
            /// @return 非0 表示失败
            static int Connect(int handle, const std::string &url);

            /// @brief 发送信息
            ///
            /// 如果在客户端使用，则会自动忽略参数peer_addr
            /// @param handle 由NewHandle返回的句柄
            /// @param msg 要发送的消息
            /// @param msg_len 消息的长度
            /// @param peer_addr 对端地址
            /// @param flag 可选参数，对应各传输协议Send时的flag,默认为0
            /// @return 0 表示成功
            /// @return 非0 表示失败
            static int SendTo(int handle, const char* msg,
                    size_t msg_len, uint64_t peer_addr, int flag = 0);

            /// @brief 客户端发送消息
            /// @param handle 由NewHandle返回的句柄
            /// @param msg 要发送的消息
            /// @param msg_len 消息的长度
            /// @param flag 可选参数，对应各传输协议Send时的flag，默认为0
            /// @return 0 表示成功
            /// @return 非0 表示失败
            static int Send(int handle, const char* msg,
                    size_t msg_len, int flag = 0); // Client only

            /// @brief 关闭句柄
            /// @param handle 由NewHandle返回
            /// @return 0 表示成功
            /// @return 非0 表示失败
            static int Close(int handle);

            /// @brief 服务器主动关闭对端
            /// @param handle 由NewHandle返回的句柄
            /// @param peer_addr 要关闭的对端
            /// @return 0 表示成功
            /// @return 非0 表示失败
            static int ClosePeer(int handle, uint64_t peer_addr);

            /// @brief 获取网络事件
            ///
            /// 目前有三种事件，分别是对端连接，对端消息，对端关闭连接,
            /// net_event_callbacks参数用于注册相应的网络事件，如果网络事件注
            /// 册则会在发生时被调用、
            /// @param net_event_callbacks 具体成员见结构体NetEventCallbacks的说明.
            /// @param max_event 最大处理事件个数
            /// @return 返回非负整数值，表示处理的事件数
            static int Poll(const NetEventCallbacks &net_event_callbacks,
                    int max_event);

            // error msg
            static char error_msg[256];

            static const char *p_error_msg;

            /// @brief 返回错误信息
            /// @param 无
            /// @return 返回错误描述
            static const char *LastErrorMessage();

            // -------------------network api end  -------------------------

        public:
            /// @brief 注册通信协议驱动
            /// @param driver 对MessageDriver接口实现的网络驱动
            /// @return 0 表示成功
            /// @return -1 表示失败
            static int RegisterDriver(MessageDriver * driver);

            /// @brief 查询通信协议驱动
            /// @param handle 句柄
            /// @return NULL 句柄未使用
            /// @return 非空 该句柄绑定的驱动的指针
            static MessageDriver * QueryDriver(int handle);

        private:
            // functions
            static std::map<int, SendToFunction> send_to_functions;

            static std::map<int, SendFunction> send_functions;

            static std::map<int, ClosePeerFunction> close_peer_functions;

            // handles
            static int32_t max_handle;

            static std::queue<int> free_handles;

            // drivers
            static std::map<std::string, MessageDriver*> prefix_to_driver;

            // handle to driver
            static std::map<int, MessageDriver*> handle_to_driver;

            // handles
            static std::set<int> all_handles;

        private:
            static int allocHandle();

            static void deallocHandle(int handle);

            static int sendToWrap(const SendFunction &send, const char* msg,
                    size_t msg_len, uint64_t peer_addr, int flag);
    };

} /* namespace pebble */ } /* namespace net */

#endif // PEBBLE_NET_MESSAGE_h
