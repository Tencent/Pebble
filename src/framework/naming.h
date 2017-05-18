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


#ifndef _PEBBLE_COMMON_NAMING_H_
#define _PEBBLE_COMMON_NAMING_H_

#include <string>
#include <vector>
#include "common/platform.h"
#include "common/error.h"

namespace pebble {

typedef cxx::function<void(const std::vector<std::string>& urls)> WatchFunc;
/// @brief 异步注册接口的执行结果回调
/// @param rc 执行结果返回值
typedef cxx::function<void(int rc)> CbReturnCode;
/// @brief 异步查询接口的执行结果回调
/// @param rc 执行结果返回值
/// @param values 查询结果(url列表)
typedef cxx::function<void(int rc, const std::vector<std::string>& urls)> CbReturnValue;


typedef enum {
    kNAMING_NOT_SUPPORTTED          =   NAMING_ERROR_CODE_BASE,
    kNAMING_INVAILD_PARAM           =   NAMING_ERROR_CODE_BASE - 1,
    kNAMING_URL_REGISTERED          =   NAMING_ERROR_CODE_BASE - 2,     ///< 已经注册过了
    kNAMING_URL_NOT_BINDED          =   NAMING_ERROR_CODE_BASE - 3,     ///< 注册的地址没有bind
    kNAMING_REGISTER_FAILED         =   NAMING_ERROR_CODE_BASE - 4,     ///< 注册地址失败
    kNAMING_FACTORY_MAP_NULL        =   NAMING_ERROR_CODE_BASE - 5,     ///< 名字工厂map为空
    kNAMING_FACTORY_EXISTED         =   NAMING_ERROR_CODE_BASE - 6,     ///< 名字工厂已存在
}NamingErrorCode;

class NamingErrorStringRegister {
public:
    static void RegisterErrorString() {
        SetErrorString(kNAMING_INVAILD_PARAM, "invalid paramater");
        SetErrorString(kNAMING_URL_REGISTERED, "url already registered");
        SetErrorString(kNAMING_URL_NOT_BINDED, "url not binded");
        SetErrorString(kNAMING_REGISTER_FAILED, "register failed");
        SetErrorString(kNAMING_FACTORY_MAP_NULL, "naming factory map is null");
        SetErrorString(kNAMING_FACTORY_EXISTED, "naming factory is existed");
    }
};


class Naming
{
public:
    Naming() {}
    virtual ~Naming() {}

    /// @brief 设置app的密钥
    virtual int32_t SetAppInfo(const std::string& app_id, const std::string& app_key)
    { return kNAMING_NOT_SUPPORTTED; }

    /// @brief 注册到名字服务
    /// @param name 注册的名字的绝对路径，格式："appid[(.path)*].service"
    /// @param url 注册的名字的地址
    ///    格式："tbuspp://name/inst" "tbus://number[(.number)*]" "http://ip:port/name"
    /// @param instance_id 注册的实例ID
    /// @return 0 成功，其它 @see NamingErrorCode
    virtual int32_t Register(const std::string& name,
                            const std::string& url,
                            int64_t instance_id = 0)
    { return kNAMING_NOT_SUPPORTTED; }

    /// @brief 从名字服务中取消注册
    /// @param name 取消注册的名字的绝对路径
    /// @param instance_id 取消注册的实例ID
    virtual int32_t UnRegister(const std::string& name, int64_t instance_id = 0)
    { return kNAMING_NOT_SUPPORTTED; }

    /// @brief 获取名字对应的所有地址列表
    /// @param name 指定的名字的绝对路径
    /// @param urls 返回名字下的地址列表
    virtual int32_t GetUrlsByName(const std::string& name, std::vector<std::string>* urls)
    { return kNAMING_NOT_SUPPORTTED; }

    /// @brief 观察名字，在名字的地址列表变化时主动通知
    virtual int32_t WatchName(const std::string& name, const WatchFunc& wc)
    { return kNAMING_NOT_SUPPORTTED; }

    /// @brief 取消对名字的观察
    virtual int32_t UnWatchName(const std::string& name)
    { return kNAMING_NOT_SUPPORTTED; }

    /// @brief 驱动名字服务检查更新
    virtual int32_t Update()
    { return kNAMING_NOT_SUPPORTTED; }

    /// @brief 获取上次的错误信息
    virtual const char* GetLastError() { return NULL; }

    /// @brief 兼容升级的适配接口
    static int32_t MakeName(int64_t app_id,
                            const std::string& service_dir,
                            const std::string& service,
                            std::string* name);

    static int32_t MakeTbusppUrl(const std::string& name,
                                 int64_t inst_id,
                                 std::string* url);

    static int32_t FormatNameStr(std::string* name);

public:
    // 下面为名字服务的扩展接口
    virtual int32_t Register(const std::string& name,
                            const std::vector<std::string>& urls,
                            int64_t instance_id = 0)
    { return kNAMING_NOT_SUPPORTTED; }

    virtual int32_t RegisterAsync(const std::string& name,
                            const std::string& url,
                            const CbReturnCode& cb,
                            int64_t instance_id = 0)
    { return kNAMING_NOT_SUPPORTTED; }

    virtual int32_t RegisterAsync(const std::string& name,
                            const std::vector<std::string>& urls,
                            const CbReturnCode& cb,
                            int64_t instance_id = 0)
    { return kNAMING_NOT_SUPPORTTED; }

    virtual int32_t UnRegisterAsync(const std::string& name,
                            const CbReturnCode& cb,
                            int64_t instance_id = 0)
    { return kNAMING_NOT_SUPPORTTED; }

    virtual int32_t GetUrlsByNameAsync(const std::string& name, const CbReturnValue& cb)
    { return kNAMING_NOT_SUPPORTTED; }

    virtual int32_t WatchNameAsync(const std::string& name, const WatchFunc& wc, const CbReturnCode& cb)
    { return kNAMING_NOT_SUPPORTTED; }
};

class NamingFactory {
public:
    NamingFactory() {}
    virtual ~NamingFactory() {}
    virtual Naming* GetNaming() = 0;
    virtual Naming* GetNaming(const std::string& host, int time_out_ms) = 0;
};

/// @brief 设置指定类型的名字服务工厂
/// @param type 名字服务的类型
/// @param factory 名字服务的工厂实例
/// @return 0成功，非0失败
int32_t SetNamingFactory(int32_t type, const cxx::shared_ptr<NamingFactory>& factory);

/// @brief 获取指定类型的名字服务工厂
/// @param type 名字服务的类型
/// @return 名字服务的工厂实例，为NULL时说明未set这种类型的工厂
cxx::shared_ptr<NamingFactory> GetNamingFactory(int32_t type);


} // namespace pebble

#endif // _PEBBLE_COMMON_NAMING_H_
