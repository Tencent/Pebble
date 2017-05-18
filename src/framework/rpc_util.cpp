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
#include "common/log.h"
#include "framework/rpc_util.inh"


namespace pebble {

RpcUtil::RpcUtil(IRpc* rpc, CoroutineSchedule* coroutine_schedule) {
    m_rpc = rpc;
    m_coroutine_schedule = coroutine_schedule;
}

RpcUtil::~RpcUtil() {
    m_rpc = NULL;
    m_coroutine_schedule = NULL;
}

int32_t RpcUtil::SendRequestSync(int64_t handle,
                    const RpcHead& rpc_head,
                    const uint8_t* buff,
                    uint32_t buff_len,
                    const OnRpcResponse& on_rsp,
                    int32_t timeout_ms) {
    if (!m_coroutine_schedule) {
        PLOG_ERROR("coroutine schedule not set");
        return kRPC_UTIL_CO_SCHEDULE_IS_NULL;
    }

    if (m_coroutine_schedule->CurrentTaskId() == INVALID_CO_ID) {
        PLOG_ERROR("not in coroutine");
        return kRPC_UTIL_NOT_IN_COROUTINE;
    }

    int32_t ret = kRPC_SUCCESS;

    SendRequestInCoroutine(handle, rpc_head, buff, buff_len, on_rsp, timeout_ms, &ret);

    return ret;
}

void RpcUtil::SendRequestInCoroutine(int64_t handle,
                    const RpcHead& rpc_head,
                    const uint8_t* buff,
                    uint32_t buff_len,
                    const OnRpcResponse& on_rsp,
                    uint32_t timeout_ms,
                    int32_t* ret) {
    int64_t co_id = m_coroutine_schedule->CurrentTaskId();
    OnRpcResponse rsp = cxx::bind(&RpcUtil::OnResponse, this,
        cxx::placeholders::_1, cxx::placeholders::_2, cxx::placeholders::_3, co_id);
    *ret = m_rpc->SendRequest(handle, rpc_head, buff, buff_len, rsp, timeout_ms);
    if (*ret != kRPC_SUCCESS) {
        return;
    }

    m_coroutine_schedule->Yield();

    *ret = on_rsp(m_result._ret, m_result._buff, m_result._buff_len);
}

void RpcUtil::SendRequestParallel(int64_t handle,
                                  const RpcHead& rpc_head,
                                  const uint8_t* buff,
                                  uint32_t buff_len,
                                  const OnRpcResponse& on_rsp,
                                  int32_t timeout_ms,
                                  int32_t* ret_code,
                                  uint32_t* num_called,
                                  uint32_t* num_parallel) {
    if (!m_coroutine_schedule) {
        PLOG_ERROR("coroutine schedule not set");
        *ret_code = kRPC_UTIL_CO_SCHEDULE_IS_NULL;
        --(*num_called);
        --(*num_parallel);
        return;
    }

    if (m_coroutine_schedule->CurrentTaskId() == INVALID_CO_ID) {
        PLOG_ERROR("not in coroutine");
        *ret_code = kRPC_UTIL_NOT_IN_COROUTINE;
        --(*num_called);
        --(*num_parallel);
        return;
    }

    if ( (*num_called) == 0 || (*num_parallel) == 0) {
        *ret_code = kRPC_UTIL_INVALID_NUM_PARALLEL_IN_COROUTINE;
        return;
    }

    SendRequestParallelInCoroutine(handle,
                                   rpc_head,
                                   buff,
                                   buff_len,
                                   on_rsp,
                                   timeout_ms,
                                   ret_code,
                                   num_called,
                                   num_parallel);
}

void RpcUtil::SendRequestParallelInCoroutine(int64_t handle,
                                             const RpcHead& rpc_head,
                                             const uint8_t* buff,
                                             uint32_t buff_len,
                                             const OnRpcResponse& on_rsp,
                                             uint32_t timeout_ms,
                                             int32_t* ret_code,
                                             uint32_t* num_called,
                                             uint32_t* num_parallel) {
    int64_t co_id = m_coroutine_schedule->CurrentTaskId();
    OnRpcResponse rsp = cxx::bind(&RpcUtil::OnResponseParallel, this,
                                  cxx::placeholders::_1,
                                  cxx::placeholders::_2,
                                  cxx::placeholders::_3,
                                  ret_code,
                                  on_rsp,
                                  co_id);
    *ret_code = m_rpc->SendRequest(handle, rpc_head, buff, buff_len, rsp, timeout_ms);
    if (*ret_code != kRPC_SUCCESS) {
        --(*num_called);
        --(*num_parallel);
        return;
    }

    if (--(*num_called) == 0) {
        m_coroutine_schedule->Yield();

        uint32_t num_yield = 0;
        while (++num_yield <= *num_parallel) {
            *(m_result._ret_code) = m_result._on_rsp(m_result._ret, m_result._buff, m_result._buff_len);

            if (num_yield < *num_parallel) {
                m_coroutine_schedule->Yield();
            }
        }
    }
}

int32_t RpcUtil::OnResponse(int32_t ret,
                            const uint8_t* buff,
                            uint32_t buff_len,
                            int64_t co_id) {
    m_result._ret      = ret;
    m_result._buff     = buff;
    m_result._buff_len = buff_len;

    m_coroutine_schedule->Resume(co_id);
    return kRPC_SUCCESS;
}

int32_t RpcUtil::OnResponseParallel(int32_t ret,
                                    const uint8_t* buff,
                                    uint32_t buff_len,
                                    int32_t* ret_code,
                                    const OnRpcResponse& on_rsp,
                                    int64_t co_id) {
    m_result._ret      = ret;
    m_result._buff     = buff;
    m_result._buff_len = buff_len;
    m_result._ret_code = ret_code;
    m_result._on_rsp   = on_rsp;

    m_coroutine_schedule->Resume(co_id);
    return kRPC_SUCCESS;
}

int32_t RpcUtil::ProcessRequest(int64_t handle, const RpcHead& rpc_head,
    const uint8_t* buff, uint32_t buff_len) {
    if (!m_coroutine_schedule) {
        return m_rpc->ProcessRequestImp(handle, rpc_head, buff, buff_len);
    }

    if (m_coroutine_schedule->CurrentTaskId() != INVALID_CO_ID) {
        return m_rpc->ProcessRequestImp(handle, rpc_head, buff, buff_len);
    }

    CommonCoroutineTask* task = m_coroutine_schedule->NewTask<CommonCoroutineTask>();
    cxx::function<void(void)> run = cxx::bind(&RpcUtil::ProcessRequestInCoroutine, this,
        handle, rpc_head, buff, buff_len);
    task->Init(run);
    task->Start();

    return kRPC_SUCCESS;
}

int32_t RpcUtil::ProcessRequestInCoroutine(int64_t handle, const RpcHead& rpc_head,
    const uint8_t* buff, uint32_t buff_len) {
    return m_rpc->ProcessRequestImp(handle, rpc_head, buff, buff_len);
}


} // namespace pebble

