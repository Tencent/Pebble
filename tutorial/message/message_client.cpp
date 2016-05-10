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

#include <stdio.h>
#include <stdlib.h>
#include "source/message/http_driver.h"
#include "source/message/message.h"
#include "source/message/tcp_driver.h"

using namespace std;
using namespace pebble::net;

int on_message(int handle, const char *msg, size_t msg_len, uint64_t addr) {
    string str(msg, msg_len);
    printf("client handle:%d, msg:%s, len:%d\n", handle,
            str.c_str(), static_cast<int>(msg_len));
    return 0;
}

#define CHECK(e) \
    if ((e) != 0) {\
        printf("file:%s, line:%d, error:%s\n", __FILE__, __LINE__, Message::LastErrorMessage());\
        exit(1);\
    }

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("usage:%s tbusd_address(eg.:udp://127.0.0.1:11599)\n", argv[0]);
        exit(1);
    }

    HttpDriver driver2;
    driver2.Init();
    Message::RegisterDriver(&driver2);

    TcpDriver driver3;
    driver3.Init();
    Message::RegisterDriver(&driver3);

    int handle3 = Message::NewHandle();
    CHECK(Message::Connect(handle3, "http://127.0.0.1:8250"));

    int handle4 = Message::NewHandle();
    CHECK(Message::Connect(handle4, "tcp://127.0.0.1:8260"));

    int handle5 = Message::NewHandle();
    CHECK(Message::Connect(handle5, "tcp://127.0.0.1:8260"));

    printf("handle3:%d, handle4:%d, handle5:%d\n",
            handle3, handle4, handle5);

    const char * msg3 = "hello, i am h3";

    const char * msg4 = "tcp 1";
    const char * msg5 = "tcp 2";

    CHECK(Message::Send(handle3, msg3, strlen(msg3)));
    CHECK(Message::Send(handle3, msg3, strlen(msg3)));

    CHECK(Message::Send(handle4, msg4, strlen(msg4)));
    CHECK(Message::Send(handle5, msg5, strlen(msg5)));
    CHECK(Message::Send(handle4, msg4, strlen(msg4)));
    CHECK(Message::Send(handle5, msg5, strlen(msg5)));

    NetEventCallbacks net_event_callbacks;
    net_event_callbacks.on_message = on_message;

    while (true) {
        Message::Poll(net_event_callbacks, 100);
        usleep(100);
    }

    return 0;
}

