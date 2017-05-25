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


#ifndef _PEBBLE_COMMON_THREAD_POOL_H_
#define _PEBBLE_COMMON_THREAD_POOL_H_

/*
    线程池：
    1、固定线程个数。
    2、线程关系对等。如果有不对等的场景，可以使用不同的线程池。
    3、添加一个任务后，先放到队列里，由多个线程同时去抢，由抢到者负责执行。
*/

#include <pthread.h>
#include <stdint.h>
#include <vector>

#include "common/blocking_queue.h"
#include "common/platform.h"
#include "common/thread.h"

namespace pebble {


class ThreadPool {
public:
    // 线程池的运行状态，放到结构体里，便于后面扩展
    struct Stats
    {
        Stats() : pending_task_num(0), working_thread_num(0) {}
        size_t pending_task_num;    // 等待被执行任务数
        size_t working_thread_num;  // 处于忙状态的线程数
    };

    enum Mode {
        NO_PENDING = 0, // 当所有线程忙时，不再接受新的任务，因为线程为抢占运行，所有状态均为瞬态，存在误差
        PENDING,        // 当所有线程忙时，新增任务暂时被缓存起来
    };

    ThreadPool();
    ~ThreadPool();

    /// @brief 线程池中线程个数
    ///
    /// @param[in] thread_num 线程个数，默认为4, 最大为256
    /// @param[in] mode 运行模式，默认为PENDING模式
    /// @return 0: 成功 其他: 失败
    int Init(int32_t thread_num = 4, int32_t mode = PENDING);

    /// @brief 向线程池中增加一个待执行的任务
    //         线程池中的线程有空闲时，就会争抢并且执行该任务
    //
    //  如果用户需要主动轮询判断任务是否完成，则需要指定一个大于0的任务ID,
    //  任务执行完成后，会把这个任务ID写入完成队列，用户可在之后通过
    //  GetFinishedTaskID()获得已经完成的任务。
    ///
    /// @param[in] fun 待执行任务函数指针
    /// @param[in] task_id 待执行任务id，由用户指定。
    //  task_id < 0, 用户不需要通过GetFinishedTaskID主动查询任务是否完成
    //  task_id >= 0, 用户需主动调用GetFinishedTaskID获得已完成任务id，
    //                  否则已完成队列会不断堆积
    //
    /// @return 0: 成功 其他: 失败
    int AddTask(cxx::function<void()>& fun, int64_t task_id = -1);

    /// @brief 获得线程池的运行状态（暂时还未实现）
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
        cxx::function<void()> fun;
        int64_t task_id;
    };

    class InnerThread : public Thread {
    public:
        InnerThread(BlockingQueue<Task>* pending_queue,
            BlockingQueue<int64_t>* finished_queue, BlockingQueue<int64_t>* working_queue);

        virtual void Run();
        void Terminate(bool waiting = true);
    private:
        BlockingQueue<Task>* m_pending_queue;
        BlockingQueue<int64_t>* m_finished_queue;
        BlockingQueue<int64_t>* m_working_queue;
        bool m_exit;
        bool m_waiting;
    };

    std::vector<InnerThread*> m_threads;
    BlockingQueue<Task> m_pending_queue;
    BlockingQueue<int64_t> m_finished_queue;
    BlockingQueue<int64_t> m_working_queue;
    bool m_exit;
    bool m_initialized;
    uint32_t m_thread_num;
    int32_t  m_mode;
};

} // namespace pebble

#endif // _PEBBLE_COMMON_THREAD_POOL_H_