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


#ifndef SERVICE_ADDRESS_CACHE_H_
#define SERVICE_ADDRESS_CACHE_H_

#include <set>
#include <string>
#include "source/service/service_handler.h"
#include "source/service/service_manage.h"

namespace pebble
{
namespace service
{

class ServiceAddressCache
{
public:
    explicit ServiceAddressCache(ServiceManager* service_manager);
    ~ServiceAddressCache();

    int SetConfig(int32_t refresh_time, int32_t invaild_time);

    void Clear();

    void SetWatchFunc(CbServiceAddrChanged change_cb)
    {
        m_addr_change_cb = change_cb;
    }

    int32_t GetServiceAddress(const std::string& service_name,
                              int* route_type, ServiceAddressVec* address);

    int32_t GetAllServiceAddress(ServicesAddress* address);

    // GetServiceAddressAsyn
    void OnGetAddrCb(int rc, GetAddrReturn& ret_addr, CbReturnAddr cob); // NOLINT

    // GetAllServiceAddressAsyn
    void OnGetAllAddrCb(int rc, GetAllAddrReturn& ret_addr, CbReturnAddr cob); // NOLINT

    // WatchServiceAddressAsyn的回调
    void OnWatchAddrCb(int rc, GetAddrReturn& ret_addr, CbReturnCode cob); // NOLINT

    // WatchAllServiceAddressAsyn的回调
    void OnWatchAllAddrCb(int rc, GetAllAddrReturn& ret_addrs, CbReturnCode cob); // NOLINT

    // Update的异步回调
    void OnUpdateCb(int rc, GetAllAddrReturn& ret_addr); // NOLINT

    void Update();

private:
    ServiceManager* m_service_manager;
    CbServiceAddrChanged m_addr_change_cb;

    int32_t m_refresh_time;
    int32_t m_invaild_time;

    int64_t m_curr_time;
    int64_t m_last_uptime;
    int64_t m_last_refresh;
    GetAllAddrReturn m_services_address;

    // watch信息记录
    bool m_is_watch_all;
    std::set<std::string> m_watched_services;
};


} // namespace service
} // namespace pebble

#endif // SERVICE_ADDRESS_CACHE_H_
