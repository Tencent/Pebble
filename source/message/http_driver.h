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


#ifndef PEBBLE_NET_HTTP_DRIVER_h_
#define PEBBLE_NET_HTTP_DRIVER_h_

#include "message_driver.h"
#include "source/common/pebble_common.h"
#include <netinet/in.h>
#include <map>
#include <queue>

namespace pebble { namespace net {
    // stream <===> message
    // message is fix size buffer
    class StreamMessageTranslator {
        public:
            virtual ~StreamMessageTranslator() {}

            virtual int InStream(const char *buf, size_t buf_len) = 0;
            virtual int OutStreamPeek(char **buf, size_t *buf_len) = 0;
            virtual int OutStreamConsume(size_t buf_len) = 0;

            virtual int SendMessage(const uint8_t *buf, uint32_t buf_len) = 0;
            virtual int RecvMessage(uint8_t **buf, uint32_t *buf_len) = 0;

            virtual bool IsServerSide() = 0;
    };

    class HttpDriver : public MessageDriver {
        public:
            HttpDriver();

            int Init();

            virtual int Bind(int handle, const std::string &url,
                    SendToFunction *send_to,
                    ClosePeerFunction *close_peer);

            virtual int Connect(int handle, const std::string &url,
                    SendFunction *send);

            virtual int Close(int handle);

            virtual int Poll(const NetEventCallbacks &net_event_callbacks,
                    int max_event);

            virtual std::string Name() {
                return "http";
            }

        private:
            int m_epoll_fd;

            std::map<int, int> m_server_socket_to_handle;
            std::map<int, int> m_client_socket_to_handle;

            std::map<uint64_t, int> m_ownership;

            std::map<int, StreamMessageTranslator*> m_socket_to_translator;

        private:
            int httpSendTo(const char* msg,
                    size_t msg_len, uint64_t addr, int flag);

            int httpSend(int handle, std::string host,
                    struct sockaddr_in socket_addr, const char* msg,
                    size_t msg_len, int flag);

            int acceptClient(int listenfd, int handle,
                    const NetEventCallbacks &net_event_callbacks);

            int clientEvent(int clientfd, int events, const NetEventCallbacks &net_event_callbacks);

            static int setNonblocking(int fd);

            static uint64_t makeOwnershipKey(int first, int second);

            int openConnection(int handle, bool server_side, int clientfd, const std::string &host);

            void closeConnection(int clientfd);

            void closeConnection(int clientfd, const NetEventCallbacks &net_event_callbacks);

            int closePeer(uint64_t peer_addr);
    };

} /* namespace pebble */ } /* namespace net */

#endif // PEBBLE_NET_HTTP_DRIVER_h_
