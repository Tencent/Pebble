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


#include "source/service/address_cache.h"

using namespace pebble::service;


ServiceAddressCache::ServiceAddressCache(ServiceManager* service_manager)
    :   m_service_manager(service_manager), m_addr_change_cb(NULL),
        m_refresh_time(300000), m_invaild_time(330000), m_curr_time(0),
        m_last_uptime(0), m_last_refresh(0), m_is_watch_all(false)
{
}

ServiceAddressCache::~ServiceAddressCache()
{
}

int32_t ServiceAddressCache::SetConfig(int32_t refresh_time, int32_t invaild_time)
{
    m_refresh_time = refresh_time;
    m_invaild_time = invaild_time;
    return 0;
}

void ServiceAddressCache::Clear()
{
    m_last_uptime = 0;
    m_last_refresh = 0;
    m_services_address.clear();
    m_addr_change_cb = NULL;
    m_is_watch_all = false;
    m_watched_services.clear();
}

int32_t ServiceAddressCache::GetServiceAddress(const std::string& service_name,
                                               int* route_type, ServiceAddressVec* address)
{
    if (m_last_uptime + m_invaild_time < m_curr_time)
    {
        return -1;
    }

    GetAddrReturn be_find;
    be_find._service_name = service_name;
    GetAllAddrReturn::iterator it = m_services_address.find(be_find);
    if (it == m_services_address.end())
    {
        return -1;
    }

    *route_type = it->_route_type;
    *address = it->_addr;
    return 0;
}

int32_t ServiceAddressCache::GetAllServiceAddress(ServicesAddress* address)
{
    if (m_last_uptime + m_invaild_time < m_curr_time)
    {
        return -1;
    }

    GetAllAddrReturn::iterator it = m_services_address.begin();
    for (; it != m_services_address.end() ; ++it)
    {
        (*address)[it->_service_name].first = it->_route_type;
        (*address)[it->_service_name].second = it->_addr;
//         address->insert(std::pair<std::string,
//             std::pair< int, ServiceAddressVec > >(it->_service_name,
//                 std::pair< int, ServiceAddressVec >(it->_route_type, it->_addr)));
    }
    return 0;
}

void ServiceAddressCache::OnGetAddrCb(int rc, GetAddrReturn& ret_addr, CbReturnAddr cob) // NOLINT
{
    // 先更新
    if (0 == rc)
    {
        GetAllAddrReturn::iterator it = m_services_address.find(ret_addr);
        if (it != m_services_address.end())
        {
            it->_route_type = ret_addr._route_type;
            it->_version = ret_addr._version;
            it->_addr = ret_addr._addr;
        }
        else
        {
            m_services_address.insert(ret_addr);
        }
        m_last_uptime = m_curr_time;
    }
    // 再返回
    if (cob != NULL)
    {
        ServicesAddress addrs;
        addrs.insert(std::pair<std::string,
            std::pair< int, ServiceAddressVec > >(ret_addr._service_name,
                std::pair< int, ServiceAddressVec >(ret_addr._route_type, ret_addr._addr)));
        cob(rc, addrs);
    }
}

void ServiceAddressCache::OnGetAllAddrCb(int rc, GetAllAddrReturn& ret_addr, CbReturnAddr cob) // NOLINT
{
    // 如果返回为空，为Update的更新
    if (cob == NULL)
    {
        OnUpdateCb(rc, ret_addr);
    }
    else
    {
        ServicesAddress addrs;
        // 先更新
        if (0 == rc)
        {
            m_services_address.swap(ret_addr);
            m_last_uptime = m_curr_time;
            GetAllServiceAddress(&addrs);
        }
        // 再返回
        cob(rc, addrs);
    }
}

void ServiceAddressCache::OnWatchAddrCb(int rc, GetAddrReturn& ret_addr, CbReturnCode cob) // NOLINT
{
    // 监控设置了就记录下来，不管是否zk上是否存在watch点
    m_watched_services.insert(ret_addr._service_name);
    // 先返回
    if (cob != NULL)
    {
        cob(rc);
    }
    // 监控设置失败了直接返回
    if (0 != rc)
    {
        return;
    }

    // 再更新cache和回调通知
    do
    {
        GetAllAddrReturn::iterator it = m_services_address.find(ret_addr);
        if (it != m_services_address.end())
        {
            // 没有更新，不回调
            if (it->_version >= ret_addr._version)
            {
                break;
            }
            it->_route_type = ret_addr._route_type;
            it->_version = ret_addr._version;
            it->_addr = ret_addr._addr;
        }
        else
        {
            m_services_address.insert(ret_addr);
        }

        // 回调通知
        if (m_addr_change_cb != NULL)
        {
            m_addr_change_cb(ret_addr._service_name, ret_addr._route_type, ret_addr._addr);
        }
    } while (0);
    m_last_uptime = m_curr_time;
}

void ServiceAddressCache::OnWatchAllAddrCb(int rc, GetAllAddrReturn& ret_addrs, CbReturnCode cob) // NOLINT
{
    // 监控设置了就记录下来，不管是否zk上是否存在watch点
    m_is_watch_all = true;
    // 先返回
    if (cob != NULL)
    {
        cob(rc);
    }
    // 监控设置失败了直接返回
    if (0 != rc)
    {
        return;
    }

    // 再更新cache和回调通知
    GetAllAddrReturn::iterator old_bit = m_services_address.begin();
    GetAllAddrReturn::iterator old_eit = m_services_address.end();
    GetAllAddrReturn::iterator new_bit = ret_addrs.begin();
    GetAllAddrReturn::iterator new_eit = ret_addrs.end();
    while (old_bit != old_eit && new_bit != new_eit)
    {
        int32_t ret = old_bit->_service_name.compare(new_bit->_service_name);
        // 节点仍在，比较版本号
        if (0 == ret)
        {
            if (old_bit->_version < new_bit->_version && m_addr_change_cb != NULL)
            {
                m_addr_change_cb(new_bit->_service_name, new_bit->_route_type, new_bit->_addr);
            }
            ++old_bit;
            ++new_bit;
        }
        else if (0 < ret) // 新增的节点，插入到了old_bit前，导致old_bit比new_bit大
        {
            if (m_addr_change_cb != NULL)
            {
                m_addr_change_cb(new_bit->_service_name, new_bit->_route_type, new_bit->_addr);
            }
            ++new_bit;
        }
        else // 删除的节点，old_bit同后面的节点比较，导致old_bit比new_bit小
        {
            if (m_addr_change_cb != NULL)
            {
                m_addr_change_cb(old_bit->_service_name, kUnknownRoute, ServiceAddressVec());
            }
            ++old_bit;
        }
    }
    if (m_addr_change_cb != NULL)
    {
        for (; old_bit != old_eit ; ++old_bit)
        {
            m_addr_change_cb(old_bit->_service_name, kUnknownRoute, ServiceAddressVec());
        }
        for (; new_bit != new_eit ; ++new_bit)
        {
            m_addr_change_cb(new_bit->_service_name, new_bit->_route_type, new_bit->_addr);
        }
    }

    m_last_uptime = m_curr_time;
    m_services_address.swap(ret_addrs);
}

void ServiceAddressCache::OnUpdateCb(int rc, GetAllAddrReturn& ret_addrs) // NOLINT
{
    if (0 != rc)
    {
        return;
    }
    if (true == m_is_watch_all)
    {
        OnWatchAllAddrCb(0, ret_addrs, NULL);
        return;
    }

    // watch回调，防止watch丢失
    if (m_addr_change_cb != NULL)
    {
        for (std::set<std::string>::iterator it = m_watched_services.begin() ;
             it != m_watched_services.end() ; ++it)
        {
            GetAddrReturn tmp_addr;
            tmp_addr._service_name = *it;
            GetAllAddrReturn::iterator old_it = m_services_address.find(tmp_addr);
            GetAllAddrReturn::iterator new_it = ret_addrs.find(tmp_addr);

            // 节点仍不存在
            if (old_it == m_services_address.end() && new_it == ret_addrs.end())
            {
                continue;
            }
            // 节点删除
            else if (old_it != m_services_address.end() && new_it == ret_addrs.end())
            {
                m_addr_change_cb(old_it->_service_name, kUnknownRoute, ServiceAddressVec());
            }
            // 节点恢复
            else if (old_it == m_services_address.end() && new_it != ret_addrs.end())
            {
                m_addr_change_cb(new_it->_service_name, new_it->_route_type, new_it->_addr);
            }
            // 版本号变化
            else if (old_it->_version < new_it->_version)
            {
                m_addr_change_cb(new_it->_service_name, new_it->_route_type, new_it->_addr);
            }
        }
    }

    // 更新地址信息
    m_last_uptime = m_curr_time;
    m_services_address.swap(ret_addrs);
}

void ServiceAddressCache::Update()
{
    timeval now;
    gettimeofday(&now, NULL);

    m_curr_time = now.tv_sec * 1000 + now.tv_usec / 1000;
    if (m_curr_time > m_last_refresh + m_refresh_time)
    {
        m_service_manager->GetAllServiceAddressAsync(NULL);
        m_last_refresh = m_curr_time;
    }
}
