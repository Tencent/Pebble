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


#include "framework/message.h"
#include "framework/raw_message_driver.h"

namespace pebble {


MessageDriver* Message::m_driver = NULL;

int32_t Message::Init()
{
    RawMessageDriver* driver = RawMessageDriver::Instance();
    if (driver->Init() == 0) {
        Message::SetMessageDriver(driver);
        return 0;
    }
    return kMESSAGE_UNINSTALL_DRIVER;
}

int64_t Message::Bind(const std::string &url)
{
    if (m_driver) {
        return m_driver->Bind(url);
    }
    return kMESSAGE_UNINSTALL_DRIVER;
}

int64_t Message::Connect(const std::string &url)
{
    if (m_driver) {
        return m_driver->Connect(url);
    }
    return kMESSAGE_UNINSTALL_DRIVER;
}

int32_t Message::Send(int64_t handle, const uint8_t* msg, uint32_t msg_len, int32_t flag)
{
    if (m_driver) {
        return m_driver->Send(handle, msg, msg_len, flag);
    }
    return kMESSAGE_UNINSTALL_DRIVER;
}

int32_t Message::SendV(int64_t handle, uint32_t msg_frag_num,
                       const uint8_t* msg_frag[], uint32_t msg_frag_len[], int flag)
{
    if (m_driver) {
        return m_driver->SendV(handle, msg_frag_num, msg_frag, msg_frag_len, flag);
    }
    return kMESSAGE_UNINSTALL_DRIVER;
}

int32_t Message::Recv(int64_t handle, uint8_t* msg_buff, uint32_t* buff_len,
                      MsgExternInfo* msg_info)
{
    if (m_driver) {
        return m_driver->Recv(handle, msg_buff, buff_len, msg_info);
    }
    return kMESSAGE_UNINSTALL_DRIVER;
}

int32_t Message::Peek(int64_t handle, const uint8_t* *msg, uint32_t* msg_len,
                      MsgExternInfo* msg_info)
{
    if (m_driver) {
        return m_driver->Peek(handle, msg, msg_len, msg_info);
    }
    return kMESSAGE_UNINSTALL_DRIVER;
}

int32_t Message::Pop(int64_t handle)
{
    if (m_driver) {
        return m_driver->Pop(handle);
    }
    return kMESSAGE_UNINSTALL_DRIVER;
}

int32_t Message::Close(int64_t handle)
{
    if (m_driver) {
        return m_driver->Close(handle);
    }
    return kMESSAGE_UNINSTALL_DRIVER;
}

int32_t Message::Poll(int64_t* handle, int32_t* event, int32_t timeout)
{
    if (m_driver) {
        return m_driver->Poll(handle, event, timeout);
    }
    return kMESSAGE_UNINSTALL_DRIVER;
}

int32_t Message::ReportHandleResult(int64_t handle, int32_t result, int64_t time_cost)
{
    if (m_driver) {
        return m_driver->ReportHandleResult(handle, result, time_cost);
    }
    return kMESSAGE_UNINSTALL_DRIVER;
}

int32_t Message::GetUsedSize(int64_t handle, uint32_t* remain_size, uint32_t* max_size)
{
    if (m_driver) {
        return m_driver->GetUsedSize(handle, remain_size, max_size);
    }
    return kMESSAGE_UNINSTALL_DRIVER;
}

const char* Message::GetLastError()
{
    if (m_driver) {
        return m_driver->GetLastError();
    }
    return "no driver installed.";
}

void Message::SetMessageDriver(MessageDriver* driver)
{
    m_driver = driver;
}


} // namespace pebble
