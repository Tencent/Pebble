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


#ifndef SOURCE_APP_SERVANT_TASK_H_
#define SOURCE_APP_SERVANT_TASK_H_

#include "source/common/coroutine.h"
#include "source/rpc/rpc.h"

namespace pebble {
namespace rpc {

class ServantTask : public CoroutineTask {
public:
    ServantTask() {}

    virtual ~ServantTask() {}

    virtual void Run() {
        m_run();
    }

    void Init(cxx::function<void(void)> run) {
        m_run = run;
    }

private:
    cxx::function<void(void)> m_run;
};
}  // namespace rpc
}  // namespace pebble

#endif  // SOURCE_APP_SERVANT_TASK_H_
