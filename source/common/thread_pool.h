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


#ifndef __PEBBLE_SOURCE_COMMON_THREAD_POOL_H__
#define __PEBBLE_SOURCE_COMMON_THREAD_POOL_H__

/*
    线程池：
    1、固定线程个数。
    2、线程关系对等。如果有不对等的场景，可以使用不同的线程池。
    3、添加一个任务后，先放到队列里，由多个线程同时去抢，由抢到者负责执行。
*/

#include <pthread.h>
#include <stdint.h>
#include <tr1/functional>
#include <vector>

#include "source/common/blocking_queue.h"
#include "source/common/thread.h"

namespace pebble {
namespace common {

class ThreadPool {
public:
    // 线程池的运行状态，放到结构体里，便于后面扩展
    struct Stats
    {
        size_t pending_task_num;    // 等待被执行任务数
    };

    ThreadPool();
    ~ThreadPool();

    /// @brief 线程池中线程个数
    ///
    /// @param[in] thread_num 线程个数，默认为4
    /// @return 0: 成功 其他: 失败
    int Init(size_t thread_num = 4);

    /// @brief 向线程池中增加一个待执行的任务
    ///
    /// @param[in] task 待执行任务
    /// @return 0: 成功 其他: 失败
    int AddTask(std::tr1::function<void()>& fun, int64_t task_id);

    /// @brief 获得线程池的运行状态
    ///
    /// @param[out] stat 线程池运行状态
    /// @return void
    void GetStatus(Stats* stat);

    /// @brief 停止接受新的任务，并终止掉线程池中所有线程的运行
    ///
    /// @param[in] waiting true:  会等待所有pending的task执行完才会结束
    ///                    false: 马上结束，pending task丢失
    /// @return void
    void Terminate(bool waiting = true);

    bool GetFinishedTaskID(int64_t* task_id);

private:
    struct Task {
    public:
        std::tr1::function<void()> fun;
        int64_t task_id;
    };

    class InnerThread : public Thread {
    public:
        InnerThread(BlockingQueue<Task>* pending_queue,
            BlockingQueue<int64_t>* finished_queue);

        virtual void Run();
        void Terminate(bool waiting = true);
    private:
        BlockingQueue<Task>* m_pending_queue;
        BlockingQueue<int64_t>* m_finished_queue;
        bool m_exit;
        bool m_waiting;
    };

    std::vector<InnerThread*> m_threads;
    BlockingQueue<Task> m_pending_queue;
    BlockingQueue<int64_t> m_finished_queue;
    bool m_exit;
};

} // namespace common
} // namespace pebble

#endif // __PEBBLE_SOURCE_COMMON_THREAD_POOL_H__
