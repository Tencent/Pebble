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


#include "message.h"
#include <limits>
#include <stdio.h>

using namespace std;
using namespace pebble::net;

map<int, SendToFunction> Message::send_to_functions;

map<int, SendFunction> Message::send_functions;

map<int, ClosePeerFunction> Message::close_peer_functions;

int32_t Message::max_handle = 0;

queue<int> Message::free_handles;

map<std::string, MessageDriver*> Message::prefix_to_driver;

map<int, MessageDriver*> Message::handle_to_driver;

std::set<int> Message::all_handles;

char Message::error_msg[256] = "";

const char *Message::p_error_msg = error_msg;

int Message::allocHandle() {
    if (!free_handles.empty()) {
        int handle = free_handles.front();
        free_handles.pop();
        all_handles.insert(handle);
        return handle;
    }

    if (max_handle == numeric_limits<int>::max()) {
        return -1;
    }

    const int ALLOC_BAT_NUM = 10;
    int n = min(ALLOC_BAT_NUM, numeric_limits<int>::max() - max_handle);
    for (int i = 0; i < n; i++) {
        free_handles.push(max_handle + i);
    }
    max_handle += n;
    return allocHandle();
}

void Message::deallocHandle(int handle) {
    free_handles.push(handle);
}

const char *Message::LastErrorMessage() {
    return p_error_msg;
}

int Message::RegisterDriver(MessageDriver * driver) {
    if (driver == NULL) {
        NET_LIB_ERR_RETURN("NULL Pointer");
    }
    if (prefix_to_driver.find(driver->Name()) != prefix_to_driver.end()) {
        NET_LIB_ERR_RETURN("Aready Registered");
    }
    prefix_to_driver[driver->Name()] = driver;
    return 0;
}

MessageDriver * Message::QueryDriver(int handle) {
    std::map<int, MessageDriver*>::iterator it = handle_to_driver.find(handle);
    return it == handle_to_driver.end() ? NULL : it->second;
}

int Message::NewHandle() {
    return allocHandle();
}


int Message::Bind(int handle, const std::string &url) {
    if (all_handles.find(handle) == all_handles.end()) { // invalid handle
        NET_LIB_ERR_RETURN("Handle Invalid");
    }
    string prefix = url.substr(0, url.find(":"));
    map<string, MessageDriver*>::iterator iter = prefix_to_driver.find(prefix);
    if (iter == prefix_to_driver.end()) {
        NET_LIB_ERR_RETURN("Driver Not Registered");
    }

    if (handle_to_driver.find(handle) != handle_to_driver.end()) { // used handle
        NET_LIB_ERR_RETURN("Handle Used");
    }

    SendToFunction send_to;
    ClosePeerFunction close_peer;
    int ret = iter->second->Bind(handle, url.substr(url.find("://") + 3), &send_to,
            &close_peer);

    if (0 != ret) {
        return ret;
    }

    handle_to_driver[handle] = iter->second;

    if (send_to) {
        send_to_functions[handle] = send_to;
    }

    if (close_peer) {
        close_peer_functions[handle] = close_peer;
    }

    return 0;
}

int Message::Connect(int handle, const std::string &url) {
    if (all_handles.find(handle) == all_handles.end()) { // invalid handle
        NET_LIB_ERR_RETURN("Handle Invalid");
    }
    string prefix = url.substr(0, url.find(":"));
    map<string, MessageDriver*>::iterator iter = prefix_to_driver.find(prefix);
    if (iter == prefix_to_driver.end()) {
        NET_LIB_ERR_RETURN("Driver Not Registered");
    }

    if (handle_to_driver.find(handle) != handle_to_driver.end()) { // used handle
        NET_LIB_ERR_RETURN("Handle Used");
    }

    SendFunction send;
    int ret = iter->second->Connect(handle, url.substr(url.find("://") + 3),
            &send);

    if (0 != ret) {
        return ret;
    }

    handle_to_driver[handle] = iter->second;

    if (send) {
        send_functions[handle] = send;
        send_to_functions[handle] = cxx::bind(sendToWrap, send,
                cxx::placeholders::_1, cxx::placeholders::_2,
                cxx::placeholders::_3, cxx::placeholders::_4);
    }

    return 0;
}

int Message::SendTo(int handle, const char* msg, size_t msg_len, uint64_t peer_addr, int flag) {
    if (UNLIKELY(msg == NULL)) {
        NET_LIB_ERR_RETURN("NULL Pointer");
    }
    map<int, SendToFunction>::iterator iter = send_to_functions.find(handle);

    if (send_to_functions.end() == iter) {
        NET_LIB_ERR_RETURN("Handle Unbind Or Invalid");
    }

    try {
        return iter->second(msg, msg_len, peer_addr, flag);
    } catch (tr1::bad_function_call ) {
        NET_LIB_ERR_RETURN("Bad Function Call");
    }
}

int Message::sendToWrap(const SendFunction &send, const char* msg,
        size_t msg_len, uint64_t peer_addr, int flag) {
    return send(msg, msg_len, flag);
}

int Message::Send(int handle, const char* msg, size_t msg_len, int flag) {
    if (UNLIKELY(msg == NULL)) {
        NET_LIB_ERR_RETURN("NULL Pointer");
    }
    map<int, SendFunction>::iterator iter = send_functions.find(handle);

    if (send_functions.end() == iter) {
        NET_LIB_ERR_RETURN("Handle Unbind Or Invalid");
    }

    try {
        return iter->second(msg, msg_len, flag);
    } catch (tr1::bad_function_call ) {
        NET_LIB_ERR_RETURN("Bad Function Call");
    }
}

int Message::Close(int handle) {
    if (all_handles.find(handle) == all_handles.end()) { // invalid handle
        NET_LIB_ERR_RETURN("Handle Invalid");
    }

    map<int, MessageDriver*>::iterator iter = handle_to_driver.find(handle);

    if (iter == handle_to_driver.end()) { // unused handle
        all_handles.erase(handle);
        deallocHandle(handle);
        return 0;
    }

    int ret = iter->second->Close(handle);

    if (0 != ret) {
        return ret;
    }

    send_to_functions.erase(handle);
    send_functions.erase(handle);
    handle_to_driver.erase(handle);
    all_handles.erase(handle);
    deallocHandle(handle);

    return 0;
}

int Message::ClosePeer(int handle, uint64_t peer_addr) {
    map<int, ClosePeerFunction>::iterator iter = close_peer_functions.find(handle);

    if (iter == close_peer_functions.end()) { // unused handle
        NET_LIB_ERR_RETURN("Not Support Op");
    }

    try {
        return iter->second(peer_addr);
    } catch (tr1::bad_function_call ) {
        NET_LIB_ERR_RETURN("Bad Function Call");
    }
}

int Message::Poll(const NetEventCallbacks &net_event_callbacks,
        int max_event) {
    int n = 0;

    if (UNLIKELY(max_event <= 0)) {
        max_event = numeric_limits<int>::max();
    }

    for (map<string, MessageDriver*>::iterator iter = prefix_to_driver.begin();
            iter != prefix_to_driver.end(); iter++) {
        int ret = iter->second->Poll(net_event_callbacks, max_event - n);
        if (ret > 0) {
            n += ret;
            if (n >= max_event) {
                return n;
            }
        }
    }

    return n;
}
