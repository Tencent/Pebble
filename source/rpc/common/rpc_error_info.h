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


#ifndef PEBBLE_RPC_ERROR_INFO_H
#define PEBBLE_RPC_ERROR_INFO_H

#include <iostream>
#include <string>
#include <map>

namespace pebble {
namespace rpc {

class ErrorInfo
{
public:
    static const int kRpcNoRrror      = 0;
    static const int kInitZKFailed    = -1;
    static const int kRpcTimeoutError = -2;
    static const int kResolveServiceAddressError  = -3;
    static const int kSendRequestFailed           = -4;
    static const int kCreateServiceInstanceFailed = -5;
    static const int kInvalidPara    = -6;
    static const int kRecvException  = -7;  // 收到exception消息
    static const int kRecvFailed     = -8;  // 接收处理失败
    static const int kMsgTypeError   = -9;  // msgtype不符合预期
    static const int kOtherException = -10; // 系统处理异常
    static const int kMissingResult  = -11; // 收到的响应无期望的结果


    static const int kSystemError = -100;


    static void Init() {
        m_error_code = kRpcNoRrror;

        m_error_info[kRpcNoRrror] = "no error";
        m_error_info[kInitZKFailed] = "init zookeeper failed";
        m_error_info[kRpcTimeoutError] = "rpc timeout";
        m_error_info[kResolveServiceAddressError] = "reslove service address error";
        m_error_info[kSendRequestFailed] = "send request failed";
        m_error_info[kCreateServiceInstanceFailed] = "create service instance on ZK failed";
        m_error_info[kSystemError] = "system error, mabye bug or other unknown error";
        m_error_info[kInvalidPara] = "invalid para";
        m_error_info[kRecvException] = "receive exception msg";
        m_error_info[kRecvFailed] = "receive process failed";
        m_error_info[kMsgTypeError] = "message type error";
        m_error_info[kOtherException] = "rpc internal exception";
        m_error_info[kMissingResult] = "rpc response missing result";
    }

    static int ErrorCode() {return m_error_code;}

    static std::string ErrorString(int error_code) {
        std::string error = "no error.";
        std::map<int, std::string>::iterator it = m_error_info.find(error_code);
        if (it != m_error_info.end()) {
            return it->second;
        }

        return error;
    }

    static std::string ErrorString() {

        std::string error = "no error.";
        std::map<int, std::string>::iterator it = m_error_info.find(m_error_code);
        if (it != m_error_info.end()) {
            return it->second;
        }

        return error;
    }

    static bool HasError() {return m_error_code != kRpcNoRrror;}
    static void SetErrorCode(int error_code) {m_error_code = error_code;}

private:
    static std::map<int, std::string> m_error_info;
    static int m_error_code;
};

} // namespace rpc
} // namespace pebble


#endif
