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


#ifndef PEBBLE_RPC_TRANSPORT_MSG_BUFFER_H
#define PEBBLE_RPC_TRANSPORT_MSG_BUFFER_H

#include "source/rpc/router/router.h"
#include "source/rpc/transport/buffer_transport.h"


namespace pebble {

namespace broadcast {
class ChannelMgr;
}

namespace rpc { namespace transport {

/**
 * MsgBuffer实现了transport接口，实际并不是一个真正的transport，而是为了适配thrift框架
 * 抽象出来的，承接protocol层和传输层，MsgBuffer和一个地址一一对应。
 *
 * 在server端，若有新地址接入，就会创建一个MsgBuffer (MsgBuffer和远端地址对应，管理1个handle)
 * 在client端，连接一个server地址，创建一个MsgBuffer
 *
 */
class MsgBuffer : public TVirtualTransport<MsgBuffer> {
    public:
        static const int DEFAULT_BUFFER_SIZE = 4096;

        MsgBuffer() :
            m_input(0),
            m_output(DEFAULT_BUFFER_SIZE),
            m_handle(-1) {
        }

        explicit MsgBuffer(int buf_size) :
            m_input(0),
            m_output(buf_size),
            m_handle(-1) {
        }

        ~MsgBuffer() {
            close();
        }

        virtual bool isOpen();

        virtual bool peek();

        virtual void open();

        virtual void close();

        uint32_t read(uint8_t* buf, uint32_t len);

        uint32_t readAll(uint8_t* buf, uint32_t len);

        void write(const uint8_t* buf, uint32_t len);

        virtual uint32_t readEnd();

        uint32_t writeEnd();

        /**
          * \brief 绑定传输层handle到此对象，在服务地址刷新时需要先close再bind
          * \param handle 传输层handle
          * \param url handle对应的url
          * \param addr 远端地址，只有server端新接入连接才需要填写，客户端使用默认值0
          */
        void bind(int handle, const std::string& url, uint64_t addr = 0);

        void bind(pebble::broadcast::ChannelMgr* channel_mgr,
            const std::string& channel_name, int encode_type);

        void setMessage(const uint8_t* msgBuf, uint32_t msgBufLen);

        const std::string& getPeerUrl() {
            return m_peer_url;
        }

        int getHandle() {
            return m_handle;
        }

    private:
        TMemoryBuffer m_input;

        TMemoryBuffer m_output;

        int m_handle;

        cxx::function<int(char*, size_t)> m_sendFunc; // NOLINT

        std::string m_peer_url;
};

} // namespace transport
} // namespace rpc
} // namespace pebble

#endif // PEBBLE_RPC_TRANSPORT_MSG_BUFFER_H

