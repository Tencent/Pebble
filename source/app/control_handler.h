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


#ifndef _SOURCE_APP_CONTROL_HANDLER_H_
#define _SOURCE_APP_CONTROL_HANDLER_H_

#include "source/app/idl/control_service_ControlCommand_if.h"

namespace pebble {
    class PebbleServer;
}

namespace pebble {

class ControlCommandHandler : public control_service::ControlCommandCobSvIf {
public:
    explicit ControlCommandHandler(pebble::PebbleServer* pebble_server) :
        pebble_server_(pebble_server) {}
    virtual ~ControlCommandHandler() {}

    void RunControlCommand(const control_service::ReqRunControlCommand& req,
        cxx::function<void(control_service::ResRunControlCommand const& _return)> cob);

private:
    ControlCommandHandler();
    pebble::PebbleServer* pebble_server_;
};



}  // namespace pebble




#endif // _SOURCE_APP_CONTROL_HANDLER_H_

