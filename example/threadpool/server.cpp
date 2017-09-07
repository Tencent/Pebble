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

#include <iostream>

#include "common/thread_pool.h"

void mytask(int id) {
    std::cout << "my task " << id << std::endl;
}


int main(int argc, const char** argv) {

    pebble::ThreadPool pool;
    pool.Init(5);

    for (int i = 0; i < 10; i++) {
        cxx::function<void()> task = cxx::bind(mytask, i);
        pool.AddTask(task);
    }

    return 0;
}



