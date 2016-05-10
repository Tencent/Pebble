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


#include "rpc_error_info.h"

int pebble::rpc::ErrorInfo::m_error_code = pebble::rpc::ErrorInfo::kRpcNoRrror;
std::map<int, std::string> pebble::rpc::ErrorInfo::m_error_info;

const int pebble::rpc::ErrorInfo::kRpcNoRrror;
const int pebble::rpc::ErrorInfo::kInitZKFailed;
const int pebble::rpc::ErrorInfo::kRpcTimeoutError;
const int pebble::rpc::ErrorInfo::kResolveServiceAddressError;
const int pebble::rpc::ErrorInfo::kSendRequestFailed;
const int pebble::rpc::ErrorInfo::kCreateServiceInstanceFailed;
const int pebble::rpc::ErrorInfo::kInvalidPara;
const int pebble::rpc::ErrorInfo::kRecvException;
const int pebble::rpc::ErrorInfo::kRecvFailed;
const int pebble::rpc::ErrorInfo::kMsgTypeError;
const int pebble::rpc::ErrorInfo::kOtherException;
const int pebble::rpc::ErrorInfo::kSystemError;
const int pebble::rpc::ErrorInfo::kMissingResult;

