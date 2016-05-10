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


#include "http_driver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include "http_parser.h"
#include <source/rpc/transport/buffer_transport.h>
#include "message.h"

namespace pebble { namespace net {

static const char POST_REQUEST_HEADER[] = // HOST and Content-length to be added
    "POST / HTTP/1.1\r\n"
    "Content-Type: application/x-thrift\r\n"
    "Accept: application/x-thrift\r\n"
    "User-Agent: Thrift/0.9.2-pebble (C++/THttpClient)\r\n"
    "Host: ";

static const char POST_RESPONSE_HEADER[] = // Content-length and Date to be added
    "HTTP/1.1 200 OK\r\n"
    "Server: Thrift/0.9.2-pebble\r\n"
    "Access-Control-Allow-Origin: *\r\n"
    "Content-Type: application/x-thrift\r\n"
    "Connection: Keep-Alive\r\n"
    "Date: ";

static const char OPTIONS_RESPONSE_HEADER[] = // Date to be added
    "HTTP/1.1 200 OK\r\n"
    "Access-Control-Allow-Origin: *\r\n"
    "Access-Control-Allow-Methods: POST, OPTIONS\r\n"
    "Access-Control-Allow-Headers: Content-Type\r\n";

static const char CONTENT_LENGTH[] =
    "\r\nContent-Length: ";

class HTTPMessageTranslator : public StreamMessageTranslator {
    public:
        explicit HTTPMessageTranslator() :
            m_parser_type(HTTP_REQUEST),
            m_message_completed(false) {
            memset(&m_parser, 0, sizeof(m_parser));
        }

        int InitServer() {
            m_parser_type = HTTP_REQUEST;
            m_parser.data = this;
            http_parser_init(&m_parser, m_parser_type);
            return 0;
        }

        int InitClient(const std::string &host) {
            m_parser_type = HTTP_RESPONSE;
            m_host = host;
            m_parser.data = this;
            http_parser_init(&m_parser, m_parser_type);
            return 0;
        }

        virtual int InStream(const char *buf, size_t buf_len) {
            if (m_message_completed) {
                return -1;
            }
            return http_parser_execute(&m_parser, &settings, buf, buf_len);
        }

        virtual int OutStreamPeek(char **buf, size_t *buf_len) {
            uint8_t * ubuf;
            uint32_t ulen;
            m_out_stream.getBuffer(&ubuf, &ulen);
            ubuf = const_cast<uint8_t*>(m_out_stream.borrow(NULL, &ulen));
            *buf = reinterpret_cast<char *>(ubuf);
            *buf_len = static_cast<size_t>(ulen);
            return 0;
        }

        virtual int OutStreamConsume(size_t buf_len) {
            m_out_stream.consume(static_cast<uint32_t>(buf_len));
            if (m_out_stream.available_read() == 0) {
                m_out_stream.resetBuffer();
            }
            return 0;
        }

        virtual int SendMessage(const uint8_t *buf, uint32_t buf_len) {
            if (HTTP_REQUEST == m_parser_type) { // parse http request mean this is a server
                m_out_stream.write(reinterpret_cast<const uint8_t*>(POST_RESPONSE_HEADER),
                        sizeof(POST_RESPONSE_HEADER) - 1);
                int ret = writeTimeRFC1123();
                if (ret != 0) {
                    return ret;
                }
            } else {
                m_out_stream.write(reinterpret_cast<const uint8_t*>(POST_REQUEST_HEADER),
                        sizeof(POST_REQUEST_HEADER) - 1);
                m_out_stream.write(reinterpret_cast<const uint8_t*>(m_host.c_str()), m_host.size());
            }
            m_out_stream.write(reinterpret_cast<const uint8_t*>(CONTENT_LENGTH),
                    sizeof(CONTENT_LENGTH) - 1);
            char cl_buff[64];
            const int cl_len = snprintf(cl_buff, sizeof(cl_buff), "%u\r\n\r\n", buf_len);
            if (cl_len < 0) {
                return cl_len;
            }
            m_out_stream.write(reinterpret_cast<const uint8_t*>(cl_buff), cl_len);
            m_out_stream.write(buf, buf_len);
            return 0;
        }

        virtual int RecvMessage(uint8_t **buf, uint32_t *buf_len) {
            if (m_message_completed) {
                m_in_http_body.getBuffer(buf, buf_len);
                m_in_http_body.resetBuffer();
                m_message_completed = false;
                http_parser_init(&m_parser, m_parser_type);
                return 0;
            } else {
                return -1;
            }
        }

        virtual bool IsServerSide() {
            return HTTP_REQUEST == m_parser_type;
        }

    public:
        int OnBody(const char *at, size_t length) {
            m_in_http_body.write(reinterpret_cast<const uint8_t*>(at),
                    static_cast<uint32_t>(length));
            return 0;
        }

        int OnComplete() {
            m_message_completed = true;
            return 0;
        }

        static int on_body(http_parser* p, const char *at, size_t length) {
            HTTPMessageTranslator *translator = reinterpret_cast<HTTPMessageTranslator *>(p->data);
            return translator->OnBody(at, length);
        }

        static int on_complete(http_parser *p) {
            HTTPMessageTranslator *translator = reinterpret_cast<HTTPMessageTranslator *>(p->data);
            return translator->OnComplete();
        }


    private:
        http_parser_type m_parser_type;

        struct http_parser m_parser;

        pebble::rpc::transport::TMemoryBuffer m_in_http_body;

        pebble::rpc::transport::TMemoryBuffer m_out_stream;

        bool m_message_completed;

        std::string m_host;

    private:
        int writeTimeRFC1123() {
            static const char* Days[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
            static const char* Months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
                "Aug", "Sep", "Oct", "Nov", "Dec"};
            char buff[128];
            time_t t = time(NULL);
            tm broken_t;
            gmtime_r(&t, &broken_t);

            int len = snprintf(buff, sizeof(buff), "%s, %d %s %d %d:%d:%d GMT",
                    Days[broken_t.tm_wday], broken_t.tm_mday, Months[broken_t.tm_mon],
                    broken_t.tm_year + 1900,
                    broken_t.tm_hour, broken_t.tm_min, broken_t.tm_sec);

            if (len < 0) {
                return len;
            }

            m_out_stream.write(reinterpret_cast<const uint8_t*>(buff), len);
            return 0;
        }

        static http_parser_settings settings;
};

http_parser_settings HTTPMessageTranslator::settings = {
    NULL, NULL, NULL, NULL, NULL, NULL,
    HTTPMessageTranslator::on_body,
    HTTPMessageTranslator::on_complete,
    NULL, NULL
};


HttpDriver::HttpDriver(): m_epoll_fd(0) {
}

int HttpDriver::Init() {
    m_epoll_fd = epoll_create(256);
    if (m_epoll_fd < 0) {
        ERRNO_RETURN();
    }
    return 0;
}

int HttpDriver::httpSendTo(const char* msg,
        size_t msg_len, uint64_t addr, int flag) {
    int clientfd = static_cast<int>(addr);

    std::map<int, StreamMessageTranslator*>::iterator tran_iter =
        m_socket_to_translator.find(clientfd);
    if (tran_iter == m_socket_to_translator.end()) {
        NET_LIB_ERR_RETURN("Fatal");
    }

    return tran_iter->second->SendMessage(reinterpret_cast<const uint8_t*>(msg),
            static_cast<uint32_t>(msg_len));
}


int HttpDriver::httpSend(int handle, std::string host,
        struct sockaddr_in socket_addr, const char* msg,
        size_t msg_len, int flag) {
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd < 0) {
        ERRNO_RETURN();
    }


    if (connect(clientfd, (struct sockaddr *)&socket_addr, sizeof(socket_addr)) < 0) {
        close(clientfd);
        ERRNO_RETURN();
    }

    if (0 != openConnection(handle, false, clientfd, host)) {
        close(clientfd);
        return -1;
    }

    StreamMessageTranslator *translator = m_socket_to_translator[clientfd];

    return translator->SendMessage(reinterpret_cast<const uint8_t*>(msg),
            static_cast<uint32_t>(msg_len));
}

int HttpDriver::setNonblocking(int fd) {
    int flags;
    int r;

    do {
        r = fcntl(fd, F_GETFL);
    } while (r == -1 && errno == EINTR);

    if (r == -1)
        return -errno;

    flags = r | O_NONBLOCK;

    do {
        r = fcntl(fd, F_SETFL, flags);
    } while (r == -1 && errno == EINTR);

    if (r) {
        return -errno;
    }

    return 0;
}

int HttpDriver::Bind(int handle, const std::string &url,
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

    struct sockaddr_in socket_addr;
    bzero(&socket_addr, sizeof(socket_addr));
    socket_addr.sin_family = AF_INET;
    std::string host = url.substr(0, spos);
    int rc = inet_aton(host.c_str(), &(socket_addr.sin_addr));
    if (rc == 0) {
        NET_LIB_ERR_RETURN("URL Invalid");
    }
    socket_addr.sin_port = htons(iport);

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        ERRNO_RETURN();
    }

    if (0 != bind(listenfd, (struct sockaddr *)&socket_addr, sizeof(socket_addr))) {
        close(listenfd);
        ERRNO_RETURN();
    }

    if (0 != listen(listenfd, 128)) {
        close(listenfd);
        ERRNO_RETURN();
    }

    if (0 != setNonblocking(listenfd)) {
        close(listenfd);
        ERRNO_RETURN();
    }

    int flag = 1;
    if (0 != setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag))) {
        close(listenfd);
        ERRNO_RETURN();
    }

    struct epoll_event event;
    event.data.fd = listenfd;
    event.events = EPOLLIN| EPOLLERR;
    if (-1 == epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, listenfd, &event)) {
        close(listenfd);
        ERRNO_RETURN();
    }

    m_server_socket_to_handle[listenfd] = handle;
    m_ownership[makeOwnershipKey(handle, listenfd)] = listenfd;

    *send_to = cxx::bind(&HttpDriver::httpSendTo, this,
        cxx::placeholders::_1, cxx::placeholders::_2,
        cxx::placeholders::_3, cxx::placeholders::_4);

    *close_peer = cxx::bind(&HttpDriver::closePeer, this,
            cxx::placeholders::_1);

    return 0;
}


int HttpDriver::Connect(int handle, const std::string &url,
        SendFunction *send) {
    int spos = url.find(":");
    if (spos <= 0) {
        NET_LIB_ERR_RETURN("URL Invalid");
    }

    int iport = atoi(url.substr(spos + 1).c_str());
    if (iport <= 0 || iport > std::numeric_limits<uint16_t>::max()) {
        NET_LIB_ERR_RETURN("Port Invalid");
    }

    struct sockaddr_in socket_addr;
    bzero(&socket_addr, sizeof(socket_addr));
    socket_addr.sin_family = AF_INET;
    std::string host = url.substr(0, spos);
    int rc = inet_aton(host.c_str(), &(socket_addr.sin_addr));
    if (rc == 0) {
        NET_LIB_ERR_RETURN("URL Invalid");
    }
    socket_addr.sin_port = htons(iport);

    *send = cxx::bind(&HttpDriver::httpSend, this,
            handle, url, socket_addr,
            cxx::placeholders::_1, cxx::placeholders::_2,
            cxx::placeholders::_3);

    return 0;
}


int HttpDriver::Close(int handle) {
    typedef std::map<uint64_t, int>::iterator Iterator;
    Iterator begin = m_ownership.lower_bound(makeOwnershipKey(handle, 0));
    Iterator end = m_ownership.lower_bound(makeOwnershipKey(handle + 1, 0));
    for (Iterator iter = begin; iter != end; iter++) {
        int fd = iter->second;
        StreamMessageTranslator *translator = m_socket_to_translator[fd];
        if (translator != NULL) {
            char *buf;
            size_t buf_len;
            int ret = translator->OutStreamPeek(&buf, &buf_len);
            if (ret == 0 && buf_len > 0) {
                if (::send(fd, buf, buf_len, MSG_NOSIGNAL) <= 0) {
                    Message::p_error_msg = strerror(errno);
                }
            }
            delete translator;
        }

        epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
        close(fd);

        m_server_socket_to_handle.erase(fd);
        m_client_socket_to_handle.erase(fd);
        m_socket_to_translator.erase(fd);
    }
    m_ownership.erase(begin, end);
    return 0;
}

int HttpDriver::closePeer(uint64_t peer_addr) {
    closeConnection(static_cast<int>(peer_addr));
    return 0;
}

uint64_t HttpDriver::makeOwnershipKey(int first, int second) {
    uint64_t high = first;
    high <<= 32;
    return high | second;
}

int HttpDriver::openConnection(int handle, bool server_side,
        int clientfd, const std::string &host) {
    if (-1 == setNonblocking(clientfd)) {
        ERRNO_RETURN();
    }

    struct epoll_event event;
    event.data.fd = clientfd;
    event.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP;
    if (-1 == epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, clientfd, &event)) {
        ERRNO_RETURN();
    }

    m_ownership[makeOwnershipKey(handle, clientfd)] = clientfd;

    HTTPMessageTranslator *translator = new HTTPMessageTranslator;
    if (server_side) {
        translator->InitServer();
    } else {
        translator->InitClient(host);
    }
    m_socket_to_translator[clientfd] = translator;
    m_client_socket_to_handle[clientfd] = handle;

    return 0;
}

void HttpDriver::closeConnection(int clientfd) {
    int handle = m_client_socket_to_handle[clientfd];
    StreamMessageTranslator *translator = m_socket_to_translator[clientfd];
    if (translator != NULL) {
        char *buf;
        size_t buf_len;
        int ret = translator->OutStreamPeek(&buf, &buf_len);
        if (ret == 0 && buf_len > 0) {
            if (::send(clientfd, buf, buf_len, MSG_NOSIGNAL) <= 0) {
                Message::p_error_msg = strerror(errno);
            }
        }
        delete translator;
    }

    epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, clientfd, NULL);
    close(clientfd);

    m_client_socket_to_handle.erase(clientfd);
    m_socket_to_translator.erase(clientfd);
    m_ownership.erase(makeOwnershipKey(handle, clientfd));
}

void HttpDriver::closeConnection(int clientfd, const NetEventCallbacks &net_event_callbacks) {
    std::map<int, int>::iterator iter = m_client_socket_to_handle.find(clientfd);
    if (iter != m_client_socket_to_handle.end()) {
        int handle = iter->second;

        closeConnection(clientfd);

        if (net_event_callbacks.on_peer_closed) {
            net_event_callbacks.on_peer_closed(handle, clientfd);
        }
    }
}

int HttpDriver::acceptClient(int listenfd, int handle,
        const NetEventCallbacks &net_event_callbacks) {
    int clientfd = accept(listenfd, NULL, NULL);
    if (clientfd < 0) {
        if (errno == EAGAIN) {
            return -1;
        } else {
            ERRNO_RETURN();
        }
    }

    if (openConnection(handle, true, clientfd, "") != 0) {
        close(clientfd);
        return -1;
    }

    if (net_event_callbacks.on_peer_connected) {
        net_event_callbacks.on_peer_connected(handle, clientfd);
    }

    return 0;
}

int HttpDriver::clientEvent(int clientfd, int events,
        const NetEventCallbacks &net_event_callbacks) {
    if ((events & EPOLLERR) || (events & EPOLLHUP)) {
        closeConnection(clientfd, net_event_callbacks);
        return 0;
    }

    std::map<int, StreamMessageTranslator*>::iterator tran_iter =
        m_socket_to_translator.find(clientfd);


    if (tran_iter == m_socket_to_translator.end()) {
        closeConnection(clientfd, net_event_callbacks);
        return 0;
    }

    StreamMessageTranslator *translator = tran_iter->second;

    if (events & EPOLLOUT) {
        char *buf;
        size_t buf_len;
        int ret = translator->OutStreamPeek(&buf, &buf_len);
        if (ret == 0 && buf_len > 0) {
            int nsend = ::send(clientfd, buf, buf_len, MSG_NOSIGNAL);
            if (nsend > 0) {
                translator->OutStreamConsume(nsend);
            }
        }
    }

    if (events & EPOLLIN) {
        char buf[256];
        size_t buf_len = sizeof(buf);
        int nrecv = 0;
        do {
            nrecv = ::recv(clientfd, buf, buf_len, MSG_NOSIGNAL);
            if (nrecv < 0) {
                if (errno != EAGAIN) {
                    closeConnection(clientfd, net_event_callbacks);
                    return 0;
                }
            } else {
                int nparsed = translator->InStream(buf, nrecv);
                int handle = m_client_socket_to_handle[clientfd];
                if (nparsed != nrecv) { // ERR
                    if (net_event_callbacks.on_invalid_message) {
                        net_event_callbacks.on_invalid_message(
                                handle, buf, nrecv, clientfd);
                    }
                    closeConnection(clientfd, net_event_callbacks);
                    return 0;
                }

                uint8_t *msg = NULL;
                uint32_t msg_len = 0;

                if (translator->RecvMessage(&msg, &msg_len) == 0) { // message completed
                    bool is_server_side = translator->IsServerSide();

                    try {
                        if (net_event_callbacks.on_message) {
                            net_event_callbacks.on_message(handle,
                                    reinterpret_cast<char*>(msg), msg_len, clientfd);
                        }
                    } catch (...) {
                        if (!is_server_side) {
                            closeConnection(clientfd);
                        } else {
                            closeConnection(clientfd, net_event_callbacks);
                        }
                        throw;
                    }

                    if (!is_server_side) {
                        closeConnection(clientfd);
                        return 0;
                    }
                }

                if (nrecv == 0) { // EOF
                    closeConnection(clientfd, net_event_callbacks);
                    return 0;
                }
            }
        } while (nrecv > 0);
    }

    return -1;
}

int HttpDriver::Poll(const NetEventCallbacks &net_event_callbacks,
        int max_event) {
    const int EVENT_ARRAY_SIZE = 256;
    struct epoll_event events[EVENT_ARRAY_SIZE];

    max_event = std::min(EVENT_ARRAY_SIZE, max_event);

    int num = epoll_wait(m_epoll_fd, events, max_event, 0);

    if (num < 0) {
        return num;
    }

    int ret = 0;

    for (int i = 0; i < num; i++) {
        std::map<int, int>::iterator iter_ssth = m_server_socket_to_handle.find(events[i].data.fd);
        if (iter_ssth != m_server_socket_to_handle.end()) {
            int handle = iter_ssth->second;
            if (events[i].events & EPOLLIN) {
                while (acceptClient(events[i].data.fd, handle, net_event_callbacks) == 0) {
                    ret++;
                    if (ret >= max_event) {
                        break;
                    }
                }
            } else if (events[i].events & EPOLLERR) {
                int listenfd = events[i].data.fd;
                epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, listenfd, NULL);
                close(listenfd);
                int handle = m_server_socket_to_handle[listenfd];
                m_ownership.erase(makeOwnershipKey(handle, listenfd));
                m_server_socket_to_handle.erase(listenfd);
            }
        } else {
            if (clientEvent(events[i].data.fd, events[i].events, net_event_callbacks) == 0) {
                ret++;
                if (ret >= max_event) {
                    break;
                }
            }
        }
    }

    return ret;
}

} /* namespace pebble */ } /* namespace net */

