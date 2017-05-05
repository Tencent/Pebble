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


#ifndef _PEBBLE_COMMON_RAW_MESSAGE_DRIVER_H_
#define _PEBBLE_COMMON_RAW_MESSAGE_DRIVER_H_

#include <list>
#include "framework/message.h"


namespace pebble {

class NetMessage;

#define TCP_HEAD_MAGIC 0xA5A5A5A5
#pragma pack(1)
/// @brief 对tcp传输数据增加消息头
struct TcpMsgHead {
    TcpMsgHead() : _magic(TCP_HEAD_MAGIC), _version(1), _data_len(0) {}
    uint32_t _magic;
    uint32_t _version;
    uint32_t _data_len;
};
#pragma pack()


/// @brief RAW TCP/UDP网络驱动接口
class RawMessageDriver : public MessageDriver {
protected:
    RawMessageDriver();
    RawMessageDriver(const RawMessageDriver& rhs) {}
public:
    // 默认接收缓冲区为8M
    static const int32_t DEFAULT_MSG_BUFF_LEN = 1024 * 1024 * 2;
    virtual ~RawMessageDriver();

    static RawMessageDriver* Instance() {
        static RawMessageDriver s_raw_message;
        return &s_raw_message;
    }

    int32_t Init(uint32_t msg_buff_len = DEFAULT_MSG_BUFF_LEN);

    virtual int64_t Bind(const std::string& url);

    virtual int64_t Connect(const std::string& url);

    virtual int32_t Send(int64_t handle, const uint8_t* msg, uint32_t msg_len, int32_t flag);

    virtual int32_t SendV(int64_t handle, uint32_t msg_frag_num,
                          const uint8_t* msg_frag[], uint32_t msg_frag_len[], int32_t flag);

    virtual int32_t Recv(int64_t handle, uint8_t* msg_buff, uint32_t* buff_len,
                         MsgExternInfo* msg_info);

    virtual int32_t Peek(int64_t handle, const uint8_t** msg, uint32_t* msg_len,
                         MsgExternInfo* msg_info);

    virtual int32_t Pop(int64_t handle);

    virtual int32_t Close(int64_t handle);

    virtual int32_t Poll(int64_t* handle, int32_t* event, int32_t timeout_ms);

    virtual int32_t ReportHandleResult(int64_t handle, int32_t result, int64_t time_cost)
    { return kMESSAGE_UNINSTALL_DRIVER; }

    virtual int32_t GetUsedSize(int64_t handle, uint32_t* remain_size, uint32_t* max_size);

    virtual const char* GetLastError();

private:
    int32_t ParseHead(const uint8_t* head, uint32_t head_len);

private:
    NetMessage* m_net_message;
};


} // namespace pebble

#endif // _PEBBLE_COMMON_RAW_MESSAGE_DRIVER_H_


