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


#include "control_handler.h"
#include "pebble_server.h"

namespace pebble {

void ControlCommandHandler::RunControlCommand(const control_service::ReqRunControlCommand& req,
    cxx::function<void(control_service::ResRunControlCommand const& _return)> cob) {

    if (pebble_server_ == NULL) {
        return;
    }
    control_service::ResRunControlCommand res;
    res.seq = req.seq;
    res.command = req.command;

    res.result = 0;
    pebble_server_->OnControlCommand(req.command, &res.result, &res.description);

    cob(res);
}

} // namespace pebble

