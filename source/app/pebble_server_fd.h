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


#ifndef SOURCE_APP_PEBBLE_SERVER_FD_INC_
#define SOURCE_APP_PEBBLE_SERVER_FD_INC_

/// 此文件是用于pebble_server.h的前置声明
class INIReader;
class CoroutineSchedule;

namespace pebble {
class TaskTimer;
class ControlCommandHandler;
class TimerTaskInterface;
class UpdaterInterface;
class ClientInetAddr;
class BroadcastMgr;


namespace rpc {
class ConnectionObj;
class Rpc;

namespace processor {
class TAsyncProcessor;
}
}

}  // namespace pebble



#endif  // SOURCE_APP_PEBBLE_SERVER_FD_INC_
