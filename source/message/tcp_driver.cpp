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


#include "tcp_driver.h"
#include <string.h>
#include "message.h"
#include <stdlib.h>
#include <limits>
#include <assert.h>

using namespace std;
using namespace pebble::net;

typedef union {
    struct {
        char peer_id[6];
        uint16_t peer_id_size;
    } address;
    uint64_t u64;
} ZMQAddressHelper;

int TcpDriver::Init() {
    m_z_ctx = zmq_init(1);
    return 0;
}

int TcpDriver::tcpSendTo(void* z_socket, const char* msg,
        size_t msg_len, uint64_t addr, int flag) {
    ZMQAddressHelper addr_helper;
    addr_helper.u64 = addr;

    if (addr_helper.address.peer_id_size >6) {
        NET_LIB_ERR_RETURN("Address Invalid");
    }

    int rc = zmq_send(z_socket, addr_helper.address.peer_id,
            addr_helper.address.peer_id_size, ZMQ_SNDMORE | ZMQ_DONTWAIT);
    if (rc < 0) {
        ERRNO_RETURN();
    }

    rc = zmq_send(z_socket, msg, msg_len, ZMQ_DONTWAIT);
    if (rc < 0) {
        ERRNO_RETURN();
    }

    return 0;
}


int TcpDriver::tcpSend(void* z_socket, const char* msg,
        size_t msg_len, int flag) {
    zmq_msg_t empty;
    int rc = zmq_msg_init(&empty);
    if (0 != rc) {
        ERRNO_RETURN();
    }

    rc = zmq_sendmsg(z_socket, &empty, ZMQ_SNDMORE | ZMQ_DONTWAIT);
    if (rc < 0) {
        ERRNO_RETURN();
    }

    rc = zmq_send(z_socket, msg, msg_len, ZMQ_DONTWAIT);
    if (rc < 0) {
        ERRNO_RETURN();
    }

    return 0;
}

int TcpDriver::Bind(int handle, const std::string &url,
        SendToFunction *send_to,
        ClosePeerFunction *close_peer) {
    int spos = url.find(":");
    if (spos <= 0) {
        NET_LIB_ERR_RETURN("URL Invalid");
    }

    int iport = atoi(url.substr(spos + 1).c_str());
    if (iport < 0 || iport > std::numeric_limits<uint16_t>::max()) {
        NET_LIB_ERR_RETURN("Port Invalid");
    }
    int rc;

    void *z_socket = zmq_socket(m_z_ctx, ZMQ_ROUTER);

    int high_water = 10000;
    zmq_setsockopt(z_socket, ZMQ_SNDHWM, &high_water, sizeof(high_water));
    zmq_setsockopt(z_socket, ZMQ_RCVHWM, &high_water, sizeof(high_water));

    rc = zmq_bind(z_socket, ("tcp://" + url).c_str());

    if (rc != 0) {
        zmq_close(z_socket);
        ERRNO_RETURN();
    }

    m_handle_to_socket[handle] = z_socket;
    m_socket_to_handle[z_socket] = handle;

    *send_to = cxx::bind(&TcpDriver::tcpSendTo, this, z_socket,
        cxx::placeholders::_1, cxx::placeholders::_2,
        cxx::placeholders::_3, cxx::placeholders::_4);

    zmq_pollitem_t poll_item;
    poll_item.socket = z_socket;
    poll_item.events = ZMQ_POLLIN;
    poll_item.fd = 0;
    m_poll_items.push_back(poll_item);

    return 0;
}


int TcpDriver::Connect(int handle, const std::string &url,
        SendFunction *send) {
    int spos = url.find(":");
    if (spos <= 0) {
        NET_LIB_ERR_RETURN("URL Invalid");
    }

    int iport = atoi(url.substr(spos + 1).c_str());
    if (iport <= 0 || iport > std::numeric_limits<uint16_t>::max()) {
        NET_LIB_ERR_RETURN("Port Invalid");
    }

    int rc;

    void *z_socket = zmq_socket(m_z_ctx, ZMQ_DEALER);

    int high_water = 10000;
    zmq_setsockopt(z_socket, ZMQ_SNDHWM, &high_water, sizeof(high_water));
    zmq_setsockopt(z_socket, ZMQ_RCVHWM, &high_water, sizeof(high_water));

    rc = zmq_connect(z_socket, ("tcp://" + url).c_str());

    if (rc != 0) {
        zmq_close(z_socket);
        ERRNO_RETURN();
    }

    m_handle_to_socket[handle] = z_socket;
    m_socket_to_handle[z_socket] = handle;

    *send = cxx::bind(&TcpDriver::tcpSend, this, z_socket,
            cxx::placeholders::_1, cxx::placeholders::_2,
            cxx::placeholders::_3);

    zmq_pollitem_t poll_item;
    poll_item.socket = z_socket;
    poll_item.events = ZMQ_POLLIN;
    poll_item.fd = 0;
    m_poll_items.push_back(poll_item);

    return 0;
}


int TcpDriver::Close(int handle) {
    void *z_socket = m_handle_to_socket[handle];
    m_handle_to_socket.erase(handle);
    if (z_socket == NULL) {
        return -1;
    } else {
        typedef std::vector<zmq_pollitem_t>::iterator Iterator;
        for (Iterator iter = m_poll_items.begin();
                iter != m_poll_items.end(); iter++) {
            if ((*iter).socket == z_socket) {
                m_poll_items.erase(iter);
                break;
            }
        }
        m_socket_to_handle.erase(z_socket);
        int rc = zmq_close(z_socket);
        if (rc != 0) {
            ERRNO_RETURN();
        } else {
            return 0;
        }
    }
}

int TcpDriver::Poll(const NetEventCallbacks &net_event_callbacks,
        int max_event) {
    int n = 0;

    int rc = zmq_poll(m_poll_items.data(), m_poll_items.size(), 0);

    if (rc <= 0) {
        return 0;
    }

    zmq_msg_t message;
    rc = zmq_msg_init(&message);
    if (rc != 0) {
        return 0;
    }

    for (uint32_t i = 0; i < m_poll_items.size(); i++) {
        if (m_poll_items[i].revents > 0) {
            void *z_socket = m_poll_items[i].socket;
            int handle = m_socket_to_handle[z_socket];
            while (true) {
                rc = zmq_msg_recv(&message, z_socket, ZMQ_DONTWAIT);
                if (rc < 0) {
                    break;
                }
                if (zmq_msg_more(&message)) { // server side
                    ZMQAddressHelper addr_helper;

                    // no our message
                    if (zmq_msg_size(&message) > sizeof(addr_helper.address.peer_id)) {
                        while (true) { // discard message
                            rc = zmq_msg_recv(&message, z_socket, ZMQ_DONTWAIT);
                            if (rc < 0 || !zmq_msg_more(&message)) {
                                break;
                            }
                        }

                        if (rc < 0) {
                            break;
                        } else {
                            continue;
                        }
                    }

                    addr_helper.address.peer_id_size = zmq_msg_size(&message);
                    memcpy(addr_helper.address.peer_id, zmq_msg_data(&message),
                            addr_helper.address.peer_id_size);

                    while (true) {
                        rc = zmq_msg_recv(&message, z_socket, ZMQ_DONTWAIT);
                        if (rc < 0) {
                            break;
                        }
                        if (!zmq_msg_more(&message)) {
                            break;
                        }
                    }
                    if (rc >= 0) {
                        if (net_event_callbacks.on_message) {
                            net_event_callbacks.on_message(handle,
                                    reinterpret_cast<char*>(zmq_msg_data(&message)),
                                    zmq_msg_size(&message), addr_helper.u64);
                        }
                        n++;
                    }
                } else { // client side
                    if (net_event_callbacks.on_message) {
                        net_event_callbacks.on_message(handle,
                                reinterpret_cast<char*>(zmq_msg_data(&message)),
                                zmq_msg_size(&message), 0);
                    }
                    n++;
                }

                if (n >= max_event) {
                    zmq_msg_close(&message);
                    return n;
                }
            }
        }
    }

    zmq_msg_close(&message);
    return n;
}

