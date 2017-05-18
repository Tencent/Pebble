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

#include "common/coroutine.h"
#include "common/timer.h"
#include "framework/channel_mgr.h"
#include "framework/message.h"
#include "framework/naming.h"
#include "framework/pebble_rpc.h"
#include "framework/processor.h"
#include "framework/router.h"
#include "framework/rpc.h"
#include "framework/rpc_util.inh"
#include "framework/session.h"


namespace pebble {

void RegisterErrorString() {
    // common
    TimerErrorStringRegister::RegisterErrorString();
    CoroutineErrorStringRegister::RegisterErrorString();

    // framework
    ChannelErrorStringRegister::RegisterErrorString();
    MessageErrorStringRegister::RegisterErrorString();
    NamingErrorStringRegister::RegisterErrorString();
    ProcessorErrorStringRegister::RegisterErrorString();
    RouterErrorStringRegister::RegisterErrorString();
    RpcErrorStringRegister::RegisterErrorString();
    SessionErrorStringRegister::RegisterErrorString();
    RpcUtilErrorStringRegister::RegisterErrorString();
    PebbleRpcErrorStringRegister::RegisterErrorString();
}

} // namespace pebble
