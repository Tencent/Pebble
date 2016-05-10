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


#include <assert.h>
#include <iostream>
#include <stdio.h>

#include "source/common/coroutine.h"

class TutorialTask : public CoroutineTask {
public:
    // 协程需要执行任务的主体
    virtual void Run() {
        int64_t c_id = id();

        // 1. 执行函数的第一部分
        printf("coroutine ID[%ld]:...do part A job...\n", c_id);
        printf("coroutine ID[%ld]:blocked, wait for I/O, signal or other\n",
               c_id);

        // 2. 让出cpu, 该协程Yield
        this->Yield();
        // 得到程序可以继续执行信号后调用resume, 程序从下面一行开始执行

        // 3. 等协程resume后二次进入run函数, 协程会记录上一次运行结束的位置
        //    即从Yield()函数之后开始执行, 而非普通二次调用函数那样从头开始
        printf("coroutine ID[%ld]:second step into Run() function\n", c_id);
        printf("coroutine ID[%ld]:...do part B job...\n", c_id);
    }
};

int main() {
    // 1. 定义协程调度器 schedule
    CoroutineSchedule schedule;
    // 2. 使用Init函数初始化该调度器,初始化成功返回0
    int ret = schedule.Init();
    assert(ret == 0);

    // 3. 创建协程任务
    // NewTask为模版函数, 创建的task会自行删除
    TutorialTask* task1 = schedule.NewTask<TutorialTask>();
    TutorialTask* task2 = schedule.NewTask<TutorialTask>();
    TutorialTask* task3 = schedule.NewTask<TutorialTask>();

    int i_size = schedule.Size();
    assert(i_size == 3);

    // 4. 执行
    printf("main: run task1\n");
    task1->Start();
    // 在Run()中调用Yield后,task1暂停执行的同时让出cpu,程序可以往下执行
    printf("main: task1 have not finished but yield, so can run here\n");

    printf("main: run task2\n");
    task2->Start();
    printf("main: task2 have not finished but yield, so can run here\n");

    printf("main: run task3\n");
    task3->Start();
    printf("main: task3 have not finished but yield, so can run here\n");

    // 5. 通过调度器恢复Yield的协程
    printf("main: task3 is ready, so Resume it\n");
    schedule.Resume(task3->id());  // 按需要resume,无需按序进行
    i_size = schedule.Size();
    assert(i_size == 2);

    printf("main: task1 is ready, so Resume it\n");
    schedule.Resume(task1->id());
    i_size = schedule.Size();
    assert(i_size == 1);

    printf("main: task2 is ready, so Resume it\n");
    schedule.Resume(task2->id());

    // 6. 删除schedule,删除没有resume的协程,返回值为还未结束的协程数
    ret = schedule.Close();
    assert(ret == 0);  // 协程都已经resume了
}
