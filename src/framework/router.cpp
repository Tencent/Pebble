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

#include "framework/router.h"

namespace pebble {

Router::Router(const std::string& name_path)
    :   m_route_name(name_path), m_route_type(kROUND_ROUTE),
        m_route_policy(NULL), m_naming(NULL)
{
    Naming::FormatNameStr(&m_route_name);
}

Router::~Router()
{
    if (kUSER_ROUTE != m_route_type && NULL != m_route_policy) {
        delete m_route_policy;
        m_route_policy = NULL;
    }
    if (NULL != m_naming) {
        m_naming->UnWatchName(m_route_name);
        m_naming = NULL;
    }
}

int32_t Router::Init(Naming* naming)
{
    if (NULL == naming) {
        return kROUTER_INVAILD_PARAM;
    }
    if (NULL != m_naming) {
        m_naming->UnWatchName(m_route_name);
    }
    m_naming = naming;
    WatchFunc cob = cxx::bind(&Router::NameWatch, this, cxx::placeholders::_1);
    int32_t ret = m_naming->WatchName(m_route_name, cob);
    if (0 != ret) {
        return kROUTER_INVAILD_PARAM;
    }
    return SetRoutePolicy(m_route_type, m_route_policy);
}

int32_t Router::SetRoutePolicy(RoutePolicyType policy_type, IRoutePolicy* policy)
{
    switch (policy_type) {
    case kUSER_ROUTE:
        if (policy == NULL) {
            return kROUTER_INVAILD_PARAM;
        }
        break;
    case kQUALITY_ROUTE:
        return kROUTER_NOT_SUPPORTTED;
    case kROUND_ROUTE:
        policy = new pebble::RoundRoutePolicy;
        break;
    case kMOD_ROUTE:
        policy = new pebble::ModRoutePolicy;
        break;
    default:
        return kROUTER_INVAILD_PARAM;
    }
    if (kUSER_ROUTE != m_route_type && NULL != m_route_policy) {
        delete m_route_policy;
    }
    m_route_type = policy_type;
    m_route_policy = policy;
    return 0;
}

int64_t Router::GetRoute(uint64_t key)
{
    if (NULL != m_route_policy) {
        return m_route_policy->GetRoute(key, m_route_handles);
    }
    return kROUTER_NOT_SUPPORTTED;
}

void Router::NameWatch(const std::vector<std::string>& urls)
{
    // TODO: 后续优化，目前实现有点粗暴
    for (uint32_t idx = 0 ; idx < m_route_handles.size() ; ++idx) {
        Message::Close(m_route_handles[idx]);
    }
    m_route_handles.clear();

    for (uint32_t idx = 0 ; idx < urls.size() ; ++idx) {
        int64_t handle = Message::Connect(urls[idx]);
        if (handle < 0) {
            continue;
        }
        m_route_handles.push_back(handle);
    }

    if (m_on_address_changed) {
        m_on_address_changed(m_route_handles);
    }
}

void Router::SetOnAddressChanged(const OnAddressChanged& on_address_changed)
{
    m_on_address_changed = on_address_changed;
    // 设置之后立即通知一次，避免router已经拿到handle列表后，后续不再变化，就无法通知了
    if (m_on_address_changed && !m_route_handles.empty()) {
        m_on_address_changed(m_route_handles);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// 避免全局变量构造、析构顺序问题
static cxx::unordered_map<int32_t, cxx::shared_ptr<RouterFactory> > * g_router_factory_map = NULL;
struct RouterFactoryMapHolder {
    RouterFactoryMapHolder() {
        g_router_factory_map = &_router_factory_map;
    }
    ~RouterFactoryMapHolder() {
        g_router_factory_map = NULL;
    }
    cxx::unordered_map<int32_t, cxx::shared_ptr<RouterFactory> > _router_factory_map;
};

cxx::shared_ptr<RouterFactory> GetRouterFactory(int32_t type) {
    cxx::shared_ptr<RouterFactory> null_factory;
    if (!g_router_factory_map) {
        return null_factory;
    }

    cxx::unordered_map<int32_t, cxx::shared_ptr<RouterFactory> >::iterator it =
        g_router_factory_map->find(type);
    if (it != g_router_factory_map->end()) {
        return it->second;
    }

    return null_factory;
}

int32_t SetRouterFactory(int32_t type, const cxx::shared_ptr<RouterFactory>& factory) {
    static RouterFactoryMapHolder fouter_factory_map_holder;
    if (!factory) {
        return kROUTER_INVAILD_PARAM;
    }
    if (g_router_factory_map == NULL) {
        return kROUTER_FACTORY_MAP_NULL;
    }
    if (false == g_router_factory_map->insert({type, factory}).second) {
        return kROUTER_FACTORY_EXISTED;
    }
    return 0;
}

} // namespace pebble
