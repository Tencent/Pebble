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


#ifndef _PEBBLE_COMMON_ROUTER_H_
#define _PEBBLE_COMMON_ROUTER_H_

#include "framework/message.h"
#include "framework/naming.h"

namespace pebble {

typedef enum {
    kROUTER_NOT_SUPPORTTED          =   ROUTER_ERROR_CODE_BASE,
    kROUTER_INVAILD_PARAM           =   ROUTER_ERROR_CODE_BASE - 1,
    kROUTER_NONE_VALID_HANDLE       =   ROUTER_ERROR_CODE_BASE - 2,
    kROUTER_FACTORY_MAP_NULL        =   ROUTER_ERROR_CODE_BASE - 3,     ///< 名字工厂map为空
    kROUTER_FACTORY_EXISTED         =   ROUTER_ERROR_CODE_BASE - 4,     ///< 名字工厂已存在
}RouterErrorCode;

class RouterErrorStringRegister {
public:
    static void RegisterErrorString() {
        SetErrorString(kROUTER_INVAILD_PARAM, "invalid paramater");
        SetErrorString(kROUTER_NONE_VALID_HANDLE, "none valid handle");
        SetErrorString(kROUTER_FACTORY_MAP_NULL, "router factory map is null");
        SetErrorString(kROUTER_FACTORY_EXISTED, "router factory is existed");
    }
};


typedef enum {
    kUSER_ROUTE,        ///< 用户自定义路由类型
    kQUALITY_ROUTE,     ///< 根据访问质量的路由类型
    kROUND_ROUTE,       ///< 轮询路由类型
    kMOD_ROUTE,         ///< 取模路由类型
    kHASH_ROUTE = kMOD_ROUTE,   ///< 哈希路由类型，由外部传入hash_key，因此等价于kMOD_ROUTE
}RoutePolicyType;

class IRoutePolicy
{
public:
    virtual ~IRoutePolicy() {}
    virtual int64_t GetRoute(uint64_t key, const std::vector<int64_t>& handles) = 0;
};

class RoundRoutePolicy      :   public IRoutePolicy
{
public:
    RoundRoutePolicy() : m_round(0) {}
    int64_t GetRoute(uint64_t key, const std::vector<int64_t>& handles)
    {
        if (0 == handles.size()) {
            return kROUTER_NONE_VALID_HANDLE;
        }
        return handles[(m_round++) % handles.size()];
    }
private:
    uint32_t    m_round;
};

class ModRoutePolicy        :   public IRoutePolicy
{
public:
    int64_t GetRoute(uint64_t key, const std::vector<int64_t>& handles)
    {
        if (0 == handles.size()) {
            return kROUTER_NONE_VALID_HANDLE;
        }
        return handles[key % handles.size()];
    }
};

/// @brief 目标地址列表变化回调函数
/// @param handles 变化后的全量handle列表
typedef cxx::function<void(const std::vector<int64_t>& handles)> OnAddressChanged;

class Router
{
public:
    /// @brief 根据名字来生成路由
    /// @param name_path 名字的绝对路径，同@see Naming中的name_path
    explicit Router(const std::string& name_path);

    virtual ~Router();

    /// @brief 初始化
    /// @param naming 提供的Naming
    /// @return 0 成功，其它失败@see RouterErrorCode
    /// @note 具体的地址列表由Router从naming中获取，地址的变化也在Router中内部处理
    virtual int32_t Init(Naming* naming);

    /// @brief 设置路由策略
    /// @param route_policy 用户路由策略类型
    /// @param policy 用户自定义路由策略，仅当route_policy为kUSER_ROUTE时有效
    /// @return 0 成功，其它失败@see RouterErrorCode
    virtual int32_t SetRoutePolicy(RoutePolicyType policy_type, IRoutePolicy* policy);

    /// @brief 根据key获取路由
    /// @param key 传入的key
    /// @return 非负数 - 成功，其它失败@see RouterErrorCode
    virtual int64_t GetRoute(uint64_t key = 0);

    /// @brief 设置router监测到地址列表发生变化时的回调函数
    /// @param on_address_changed 当地址列表发生变化时，调用此函数
    virtual void SetOnAddressChanged(const OnAddressChanged& on_address_changed);

protected:
    void NameWatch(const std::vector<std::string>& urls);

    std::string             m_route_name;
    RoutePolicyType         m_route_type;
    IRoutePolicy*           m_route_policy;
    Naming*                 m_naming;
    std::vector<int64_t>    m_route_handles;
    OnAddressChanged        m_on_address_changed;
};

class RouterFactory {
public:
    RouterFactory() {}
    virtual ~RouterFactory() {}
    virtual Router* GetRouter(const std::string& name_path) {
        return new Router(name_path);
    }
};

/// @brief 设置指定类型的路由器工厂
/// @param type 路由器的类型
/// @param factory 路由器的工厂实例
/// @return 0成功，非0失败
int32_t SetRouterFactory(int32_t type, const cxx::shared_ptr<RouterFactory>& factory);

/// @brief 获取指定类型的路由器工厂
/// @param type 路由器的类型
/// @return 路由器的工厂实例，为NULL时说明未set这种类型的工厂
cxx::shared_ptr<RouterFactory> GetRouterFactory(int32_t type);


} // namespace pebble

#endif // _PEBBLE_COMMON_ROUTER_H_
