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


#include <source/app/easy_coroutine.h>
#include <iostream>
#include "source/common/coroutine.h"

namespace pebble {

EasyCoroutine::EasyCoroutine()
        : my_server_(NULL) {
    // DO NOTHING
}

EasyCoroutine::~EasyCoroutine() {
    // DO NOTHING
}

int EasyCoroutine::Update(PebbleServer* pebble_server) {
    int ret = -1;
    if (pebble_server == NULL)
        return ret;

    // 处理新增协程
    if (new_yield_ids_.empty() == false) {
        yield_ids_.insert(yield_ids_.end(), new_yield_ids_.begin(),
                          new_yield_ids_.end());
        new_yield_ids_.clear();
    }

    // 处理Resume()
    CoroutineSchedule* schedule = pebble_server->GetCoroutineSchedule();
    std::list<YieldItem>::iterator it = yield_ids_.begin();
    while (it != yield_ids_.end()) {
        if (it->check_resume != NULL) {

            // 处理YieldUntil()
            if (it->check_resume()) {
                ret = 0;
                schedule->Resume(it->id);
                it = yield_ids_.erase(it);
            } else {
                it++;
            }

        } else {

            // 处理YieldReturn()
            if (pebble_server->is_idle()
                    || it->skip_times > it->max_skip_times) {
                it->skip_times = 0;
                ret = 0;
                schedule->Resume(it->id);
                it = yield_ids_.erase(it);
            } else {
                it->skip_times++;
                it++;
            }
        }
    }

    return ret;
}

int EasyCoroutine::Init(PebbleServer* server) {
    if (server == NULL)
        return -1;
    my_server_ = server;
    server->AddUpdater(this);
    return 0;
}

void EasyCoroutine::YieldReturn(int max_skip_times) {
    if (my_server_ == NULL)
        return;
    Yield(max_skip_times, NULL);
}

void EasyCoroutine::YieldUntil(CheckResumeCallback check_resume_cb,
                               int cookie) {
    cxx::function<bool()> check_resume = cxx::bind(check_resume_cb, cookie);
    YieldUntil(check_resume);
}

void EasyCoroutine::YieldUntil(cxx::function<bool()> check_resume_cb) {
    if (my_server_ == NULL)
        return;
    Yield(0, check_resume_cb);
}

void EasyCoroutine::Yield(int max_skip_times,
                          cxx::function<bool()> check_resume) {
    CoroutineSchedule* schedule = my_server_->GetCoroutineSchedule();
    int64_t cor_id = schedule->CurrentTaskId();
    pebble::YieldItem yield_item;
    yield_item.id = cor_id;
    yield_item.skip_times = 0;
    yield_item.max_skip_times = max_skip_times;
    yield_item.check_resume = check_resume;
    new_yield_ids_.push_back(yield_item);
    schedule->Yield();
}

}  // namespace pebble

