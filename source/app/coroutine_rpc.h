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


#ifndef SOURCE_APP_COROUTINE_RPC_H_
#define SOURCE_APP_COROUTINE_RPC_H_

#include "source/rpc/rpc.h"

namespace pebble {
class PebbleServer;

namespace rpc {

class CoroutineRpc : public Rpc {
protected:
    CoroutineRpc();

public:
    static Rpc* Instance() {
        if (NULL == m_rpc) {
            m_rpc = new CoroutineRpc();
        }
        return m_rpc;
    }

    virtual ~CoroutineRpc();

    int Init(PebbleServer* server, const std::string& app_id,
        int64_t game_id, const std::string& game_key) {
        m_pebble_server = server;

        return Rpc::Init(app_id, game_id, game_key);
    }

    void Resume(int64_t coroutine_id);

    virtual int Block(protocol::TProtocol** prot, int timeout_ms);

    virtual int ProcessRequest(const std::string& name, int64_t seqid,
                            cxx::shared_ptr<protocol::TProtocol> protocol);

    int ProcessRequestImp(const std::string& name, int64_t seqid,
                            cxx::shared_ptr<protocol::TProtocol> protocol);

    /// @brief 获取当前连接对象
    /// @param connection_obj 输出参数，将当前的连接信息写到connection_obj中
    /// @return 0 获取成功
    /// @return 其他 获取失败
    virtual int GetCurConnectionObj(ConnectionObj* connection_obj);

    /// @brief server间直接广播的消息，不能立即处理，需要先缓存
    int PendingMsg(int handle, uint64_t peer_addr, int encode_type,
        const std::string& message);

    /// @brief 处理pending的请求消息
    int ProcessPendingMsg(int max_event = 100);

private:
    void Cob(int ret, protocol::TProtocol* prot, cxx::function<void()> cob) {
        m_result = ret;
        m_prot = prot;
        cob();
    }

    void AddCurConnectionObj();
    void FreeCurConnectionObj();

    class PendingMessage {
    public:
        PendingMessage(int handle, uint64_t addr, int encode, const std::string& msg)
            : m_handle(handle), m_peer_addr(addr), m_encode_type(encode), m_message(msg) {}
        ~PendingMessage() {}
        int m_handle;
        uint64_t m_peer_addr;
        int m_encode_type;
        std::string m_message;
    };

private:
    pebble::PebbleServer* m_pebble_server;
    int m_result;
    protocol::TProtocol* m_prot;
    std::map<int64_t, ConnectionObj*> m_conn_objs;

    std::map<int, cxx::shared_ptr<protocol::TProtocol> > m_pending_handler;
    std::vector< cxx::shared_ptr<PendingMessage> > m_pending_msgs;
};

}  // namespace rpc
}  // namespace pebble

#endif  // SOURCE_APP_COROUTINE_RPC_H_

