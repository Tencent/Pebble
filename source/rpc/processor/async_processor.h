/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef PEBBLE_RPC_PROCESSOR_ASYNCPROCESSOR_H
#define PEBBLE_RPC_PROCESSOR_ASYNCPROCESSOR_H

#include <tr1/memory>
#include <tr1/functional>
#include "source/rpc/protocol/protocol.h"
#include "source/rpc/processor/processor.h"

namespace pebble { namespace rpc { namespace processor {

/**
 * Async version of a TProcessor.  It is not expected to complete by the time
 * the call to process returns.  Instead, it calls a cob to signal completion.
 */

// class TEventServer; // forward declaration
// 0.92版本中暂时无TEventServer的实现，参考实现
// https://github.com/facebook/fbthrift

class TAsyncProcessor {
public:
    virtual ~TAsyncProcessor() {}

    virtual std::string getServiceName() { return std::string(""); }

    virtual void process(cxx::function<void(bool success)> _return,
                            cxx::shared_ptr<protocol::TProtocol> in,
                            cxx::shared_ptr<protocol::TProtocol> out,
                            const std::string& fname, int64_t seqid) = 0;

    void process(cxx::function<void(bool success)> _return,
                    cxx::shared_ptr<pebble::rpc::protocol::TProtocol> io,
                    const std::string& fname, int64_t seqid) {
        process(_return, io, io, fname, seqid);
    }

    cxx::shared_ptr<TProcessorEventHandler> getEventHandler() {
        return eventHandler_;
    }

    void setEventHandler(cxx::shared_ptr<TProcessorEventHandler> eventHandler) {
        eventHandler_ = eventHandler;
    }

    // const TEventServer* getAsyncServer() {
    //     return asyncServer_;
    // }
protected:
    TAsyncProcessor() {}

    cxx::shared_ptr<TProcessorEventHandler> eventHandler_;
    // const TEventServer* asyncServer_;
private:
    // friend class TEventServer;
    // void setAsyncServer(const TEventServer* server) {
    //     asyncServer_ = server;
    // }
};

class TAsyncProcessorFactory {
public:
    virtual ~TAsyncProcessorFactory() {}

    /**
     * Get the TAsyncProcessor to use for a particular connection.
     *
     * This method is always invoked in the same thread that the connection was
     * accepted on.  This generally means that this call does not need to be
     * thread safe, as it will always be invoked from a single thread.
     */
    virtual cxx::shared_ptr<TAsyncProcessor> getProcessor(
        const TConnectionInfo& connInfo) = 0;
};

} // namespace processor
} // namespace rpc
} // namespace pebble


#if 0
// XXX I'm lazy for now
namespace apache { namespace thrift {
using pebble::rpc::async::TAsyncProcessor;
}}
#endif

#endif // PEBBLE_RPC_PROCESSOR_ASYNCPROCESSOR_H
