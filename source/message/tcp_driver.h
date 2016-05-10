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


#ifndef PEBBLE_NET_TCP_DRIVER_h
#define PEBBLE_NET_TCP_DRIVER_h

#include <zmq.h>
#include "message_driver.h"
#include "source/common/pebble_common.h"
#include <map>
#include <queue>
#include <vector>

namespace pebble { namespace net {
    class TcpDriver : public MessageDriver {
        public:
            TcpDriver(): m_z_ctx(NULL) {
            }

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
                return "tcp";
            }

        private:
            void *m_z_ctx;

            std::map<int, void*> m_handle_to_socket;

            std::map<void*, int> m_socket_to_handle;

            std::vector<zmq_pollitem_t> m_poll_items;

        private:
            int tcpSendTo(void* z_socket, const char* msg,
                    size_t msg_len, uint64_t addr, int flag);

            int tcpSend(void* z_socket, const char* msg,
                    size_t msg_len, int flag);
    };

} /* namespace pebble */ } /* namespace net */

#endif // PEBBLE_NET_TCP_DRIVER_h
