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

#include <stdlib.h>
#include "source/message/http_driver.h"
#include "source/message/message.h"
#include "source/message/tcp_driver.h"

using namespace std;
using namespace pebble::net;

#define CHECK(e) \
    if ((e) != 0) {\
        printf("file:%s, line:%d, error:%s\n", __FILE__, __LINE__, Message::LastErrorMessage());\
        exit(1);\
    }

int on_message(int handle, const char *msg, size_t msg_len, uint64_t addr) {
    string str(msg, msg_len);
    printf("server handle:%d, msg:%s, len:%d\n", handle,
            str.c_str(), static_cast<int>(msg_len));

    char ret_msg[512];
    int ret_n = snprintf(ret_msg, sizeof(ret_msg), "your msg=%s", str.c_str());
    CHECK(Message::SendTo(handle, ret_msg, ret_n, addr));
    return 0;
}


int main(int argc, char **argv) {
    if (argc != 2) {
        printf("usage:%s tbusd_address(eg.:udp://127.0.0.1:11599)\n", argv[0]);
        exit(1);
    }

    HttpDriver driver3;
    CHECK(driver3.Init());
    Message::RegisterDriver(&driver3);

    TcpDriver driver4;
    CHECK(driver4.Init());
    Message::RegisterDriver(&driver4);

    int handle4 = Message::NewHandle();
    CHECK(Message::Bind(handle4, "http://0.0.0.0:8250"));
    int handle5 = Message::NewHandle();
    CHECK(Message::Bind(handle5, "tcp://0.0.0.0:8260"));

    printf("handle4:%d, handle5:%d\n",
            handle4, handle5);

    NetEventCallbacks net_event_callbacks;
    net_event_callbacks.on_message = on_message;

    while (true) {
        Message::Poll(net_event_callbacks, 100);
        usleep(100);
    }

    return 0;
}

