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


#ifndef PEBBLE_SERVICE_ADDRESS_PULLER_H
#define PEBBLE_SERVICE_ADDRESS_PULLER_H

#include <map>
#include <tr1/functional>
#include <utility>
#include <vector>
#include "source/service/service_manage.h"


class CoroutineSchedule;
class ZookeeperClient;

namespace pebble {
namespace service {

class ServiceAddressInfo;
class ServiceAddressPuller;
class ServiceAddressPullerImp;

/// @brief 地址信息增加unitid信息
class ServiceAddressInfoEx {
public:
    int unitid;
    ServiceAddressInfo addr;

    ServiceAddressInfoEx() : unitid(-1) {}
};

/// @brief 服务地址变化通知回调
/// @para game_id game_id
/// @para service_name service_name
/// @note 回调被调用后，需要业务另外主动触发GetServiceAddress，注意不能直接在回调函数中触发
/// @note service_name 为空时，需要业务重新获取此gameid下的所有服务地址
typedef std::tr1::function<void(int64_t game_id, const std::string& service_name)>
    ServiceAddrChanged;

enum GetResult {
    GET_FAILED = -1, // 获取失败
    GET_SUCCESS = 0, // 获取成功
    GET_PART_SUCCESS = 1, // 获取到部分数据，获取过程中有错误
    GET_DUPLICATE = 2, // 重复获取
};

/// @brief 获取指定游戏的某个服务的全量地址信息
class ServiceAddressPuller {
public:
    ServiceAddressPuller();
    virtual ~ServiceAddressPuller();

    /// @brief 初始化
    /// @para zk_address zk地址，如 "127.0.0.1:1888"
    /// @para co 协程调度器
    /// @para time_out_ms 与zk连接超时时间，单位ms，默认5s
    /// @return 0 成功，非0 失败
    int32_t Init(const std::string& zk_address, CoroutineSchedule* co, int time_out_ms = 5000);

    /// @brief 注册服务地址变化通知回调
    /// @para cb 回调函数
    void RegisterCallback(ServiceAddrChanged cb);

    /// @brief 获取指定游戏的某个服务的全量地址
    /// @para game_id game_id
    /// @para game_key game_key
    /// @para service_name service_name
    /// @para address 获取到的地址信息
    /// @return 0 成功，非0 失败，非0时也可能只获取到了部分地址
    /// @note 协程同步接口，需要在协程内调用(如rpc服务处理)
    /// @note TimerTask本身也是协程，禁止在TimerTask中使用
    int32_t GetServiceAddress(int64_t game_id, const std::string& game_key,
        const std::string& service_name, std::vector<ServiceAddressInfoEx>** address);

    /// @brief 消息驱动
    /// @return 0 空闲
    /// @return 1 忙
    int Update();

private:
    void WachCallback(int64_t game_id, const std::string& service_name);

    class ServiceData {
    public:
        bool m_doing; // 是否正在获取
        int64_t m_gameid;
        std::string m_gamekey;
        std::vector<ServiceAddressInfoEx> m_address;
        ServiceData() : m_doing(false), m_gameid(-1) {}
    };

    ServiceAddressPullerImp* m_address_puller_imp;
    std::map<std::pair<int64_t, std::string>, ServiceData> m_service_address;
    ServiceAddrChanged m_service_addr_changed;
};

} // namespace service
} // namespace pebble

#endif // PEBBLE_SERVICE_ADDRESS_PULLER_H
