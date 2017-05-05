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


#ifndef _PEBBLE_COMMON_PROCESSOR_H_
#define _PEBBLE_COMMON_PROCESSOR_H_

#include "common/platform.h"

namespace pebble {

/// @brief send函数定义
/// @param flag message接口可选参数，默认为0
typedef cxx::function< // NOLINT
    int32_t(int64_t handle, const uint8_t* buff, uint32_t buff_len, int32_t flag)> SendFunction;

/// @brief sendv函数定义
/// @param flag message接口可选参数，默认为0
typedef cxx::function< // NOLINT
    int32_t(int64_t handle, uint32_t msg_frag_num,
    const uint8_t* msg_frag[], uint32_t msg_frag_len[], int32_t flag)> SendVFunction;

/// @brief broadcast函数定义
typedef cxx::function< // NOLINT
    int32_t(const std::string& channel_name, const uint8_t* buff, uint32_t buff_len)> BroadcastFunction;

/// @brief broadcastv函数定义
typedef cxx::function< // NOLINT
    int32_t(const std::string& channel_name, uint32_t msg_frag_num,
    const uint8_t* msg_frag[], uint32_t msg_frag_len[])> BroadcastVFunction;


/// @brief IEventHandler是对Processor通用处理的抽象，和Processor配套使用
///     一般建议Processor本身处理和协议、业务强相关的逻辑，针对消息的一些通用处理由EventHandler完成
class IEventHandler {
public:
    virtual ~IEventHandler() {}

    /// @brief 请求处理完成事件，在请求消息处理完成时被回调
    /// @param name 消息名称
    /// @param result 处理结果
    /// @param time_cost_ms 处理耗时，单位毫秒
    /// @note 多维度的数据由用户自己组装到name中，最终落地到文件，计算与展示由业务的数据分析系统完成
    virtual void OnRequestProcComplete(const std::string& name,
        int32_t result, int32_t time_cost_ms) = 0;

    /// @brief 响应处理完成事件，在响应消息处理完成时被回调
    /// @param name 消息名称
    /// @param result 请求结果
    /// @param time_cost_ms 请求耗时，单位毫秒
    /// @note 多维度的数据由用户自己组装到name中，最终落地到文件，计算与展示由业务的数据分析系统完成
    virtual void OnResponseProcComplete(const std::string& name,
        int32_t result, int32_t time_cost_ms) = 0;
};

/// @brief Pebble消息处理抽象接口
class IProcessor {
public:
    virtual ~IProcessor() {}

    /// @brief 设置send函数，processor使用设置的send函数发送消息
    /// @param send  函数原型为int32_t(int64_t, const uint8_t*, uint32_t, int32_t)
    /// @param sendv 函数原型为int32_t(int64_t, uint32_t, const uint8_t*[], uint32_t[], int32_t)
    /// @return 0 成功
    /// @return 非0 失败
    virtual int32_t SetSendFunction(const SendFunction& send, const SendVFunction& sendv) = 0;

    /// @brief 设置广播函数
    /// @return <0 失败
    /// @return >=0 发送成功的消息数
    virtual int32_t SetBroadcastFunction(const BroadcastFunction& broadcast,
        const BroadcastVFunction& broadcastv) = 0;

    /// @brief 设置EventHandler，在processor中调用EventHandler接口以实现消息通用功能
    /// @param event_handler IEventHandler接口实例，不同类型的processor上有不同的EventHandler实现
    /// @return 0 成功
    /// @return 非0 失败
    virtual int32_t SetEventHandler(IEventHandler* event_handler) = 0;

    /// @brief 消息处理入口(Processor的输入)
    /// @param handle 网络句柄(用于传输消息的连接或消息队列标识)
    /// @param buff 消息
    /// @param buff_len 消息长度
    /// @param is_overload 是否过载(框架根据程序运行情况告知Processor目前是否过载，由Processor自行过载处理)
    ///     涵盖了具体的过载类型，具体可参考 @see OverLoadType
    /// @return 0 成功
    /// @return 非0 失败
    // TODO: 每个Processor建议维护一个消息白名单或优先级列表，允许某些消息即使在过载情况下仍然要处理
    virtual int32_t OnMessage(int64_t handle, const uint8_t* buff, uint32_t buff_len, uint32_t is_overload) = 0;

    /// @brief 事件驱动
    /// @return 处理的事件数，0表示无事件
    virtual int32_t Update() = 0;

    /// @brief 获取未结束的任务数，用于过载判断
    /// @return 实际未结束的任务数
    virtual uint32_t GetUnFinishedTaskNum() = 0;

    /// @brief Processor上报动态资源使用情况，由各Processor实现者填写内部维护的动态资源信息，框架定期调用
    ///     如 Processor内部session的个数 myprocessor:session:9999
    /// @param resource_info name-value的表，name为资源名称，value为值
    /// @note Processor的对象标示可以组合在name中
    virtual void GetResourceUsed(cxx::unordered_map<std::string, int64_t>* resource_info) = 0;
};

} // namespace pebble

#endif // _PEBBLE_COMMON_PROCESSOR_H_
