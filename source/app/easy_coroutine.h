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


#ifndef SOURCE_APP_EASY_COROUTINE_H_
#define SOURCE_APP_EASY_COROUTINE_H_

#include <list>
#include "source/app/pebble_server.h"
#include "source/common/pebble_common.h"


/// @brief YieldUntil所使用的参数，用来检查是否应该Resume回去
typedef bool (*CheckResumeCallback)(int cookie);


namespace pebble {

// 用于在主循环中产生Resume的协程登记项目
struct YieldItem {
    int64_t id;
    int skip_times;
    int max_skip_times;
    cxx::function<bool()> check_resume;  // 用于回调的闭包对象
};

/// @brief 用于简化协程的使用，你可以选择两种用法：一、使用YieldReturn
/// 方法，无需编写特殊的Resume函数，程序会在调用此方法处切换出去，再下次
/// 主循环被切换进来。因此你可以用一个循环包含此语句，把需要反复检查等待的
/// 代码放在此方法调用后;二、使用YieldUntil方法，此方法也会切换出去，同
/// 时此方法需要输入一个函数指针，用以让框架每次主循环都调用它，如果其返回
/// 值是true，则Resume回来。你同时可以输入一个cookie参数，每次对于此切
/// 换时都会重复传入此cookie的值，用于识别此次YieldUntil。需要特别注意
/// 的是，这个回调函数不得使用YieldUntil调用所在函数中的任何局部变量，因
/// 为在协程切换出去后，这些变量都会被拷贝走而导致不可用。
class EasyCoroutine : public pebble::UpdaterInterface {
public:
    EasyCoroutine();
    virtual ~EasyCoroutine();
    virtual int Update(PebbleServer *pebble_server);
    int Init(PebbleServer *server);

    /// @brief 调用此方法即可中断当前协程，并且在下次主循环空闲时回到此处
    /// @param[in] max_skip_time 如果主循环一直不空闲，
    ///            最大等待max_skip_times后强制返回一次
    void YieldReturn(int max_skip_times = 1000);

    /// @brief 调用此方法可中断当前协程，然后框架会在每次主循环时调用输入
    ///        的函数，如果其返回true，则Resume回本方法被调用的协程。
    /// @param[in] check_resume 用于检查切换回本协程的函数指针，注意
    ///            此函数指针指向的函数，是在本协程外运行的，因此所有的
    ///            内存访问都要注意不能用本方法所在函数的局部变量（栈变
    ///            量）
    /// @param[in] cookie 此参数用于每个主循环都会在调用check_resume
    ///                   时传入，可用此参数来区别不同批次的YieldUnitl()
    ///                   调用。
    void YieldUntil(CheckResumeCallback check_resume_cb, int cookie);

    void YieldUntil(cxx::function<bool()> check_resume_cb);

private:
    std::list<YieldItem> yield_ids_;
    std::list<YieldItem> new_yield_ids_;
    PebbleServer* my_server_;
    void Yield(int max_skip_times, cxx::function<bool()> check_resume);
};

}  // namespace pebble

#endif  // SOURCE_APP_EASY_COROUTINE_H_
