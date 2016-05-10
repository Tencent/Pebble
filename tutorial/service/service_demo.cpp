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


#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include "source/common/string_utility.h"
#include "source/service/service_manage.h"

bool bNotifyCallBack = false;
int64_t game_id = 11;
std::string game_key("game_key");
std::string app_id("800.1.1");
std::string zk_url("127.0.0.1:1888");

using namespace pebble::service;

void ServiceAddressNotifyCallback(const std::string& service_name,
                                  int service_route,
                                  const ServiceAddressVec& address) {
    std::cout << "[Update] " << service_name << " : " << std::endl
              << "      Route : " << service_route << std::endl
              << "      Addr : ";

    ServiceAddressVec::const_iterator it;
    for (it = address.begin(); it != address.end(); ++it) {
        std::cout << "url - " << it->address_url << " , proto - " << it->protocol_type << " ; ";
    }
    std::cout << std::endl;
    bNotifyCallBack = true;
}

int InitServiceManage(ServiceManager* sm)
{
    int ret = sm->Init(zk_url, 5000);
    if (ret != kSM_OK) {
        std::cout << "ServiceManager init failed. ret = " << ret << std::endl;
        return ret;
    }

    ret = sm->SetAppInfo(game_id, game_key, app_id);
    if (ret != kSM_OK) {
        std::cout << "ServiceManager SetAppInfo failed. ret = " << ret << std::endl;
        return ret;
    }

    return 0;
}

int main(int argc, char* argv[]) {
    // show usage
    std::cout
        << "usage   : ./service_demo game_id game_key app_id zk_url"
        << std::endl;

    // set parameter
    if (argc > 1) { game_id = strtoll(argv[1], NULL, 0); }
    if (argc > 2) { game_key = argv[2]; }
    if (argc > 3) { app_id = argv[3]; }
    if (argc > 4) { zk_url = argv[4]; }

    std::cout << "./service_demo " << game_id << " " << game_key << " "
        << app_id << " " << zk_url << std::endl;

    // 服务端初始化 ServiceManager
    ServiceManager m_server_sm;
    if (0 != InitServiceManage(&m_server_sm)) {
        std::cout << "m_server_sm init failed." << std::endl;
        return -1;
    }

    // 服务端创建服务实例
    std::string service_name("Demo");
    ServiceAddressInfo addr("tcp://127.0.0.1:8000", 21);
    ServiceAddressVec service_address(1, addr);
    int ret = m_server_sm.CreateServiceInstance(service_name, kRoundRoute, service_address);
    std::cout << "CreateServiceInstance 1 ret = " << ret << std::endl;

    // 客户端初始化 ServiceManager
    ServiceManager m_client_sm;
    if (0 != InitServiceManage(&m_client_sm)) {
        std::cout << "m_client_sm init failed." << std::endl;
        return -1;
    }

    // 客户端获取服务信息
    int service_route = 0;
    service_address.clear();
    ret = m_client_sm.GetServiceAddress(service_name, &service_route, &service_address);
    std::cout << "GetService " << service_name << " ret = " << ret << std::endl;
    std::cout << "      Route = " << service_route << std::endl;
    std::cout << "      Addr :";

    ServiceAddressVec::iterator it;
    for (it = service_address.begin(); it != service_address.end(); ++it) {
        std::cout << "url - " << it->address_url << " , proto - " << it->protocol_type << " ; ";
    }
    std::cout << std::endl;

    // 服务端下线观察地址信息观察回调变化
    // 注意回调函数会被触发2次，1次为节点删除，1次为子节点信息变化
    std::cout << "Service shutdown now" << std::endl;
    m_client_sm.WatchServiceAddress(service_name);
    m_client_sm.RegisterWatchFunc(ServiceAddressNotifyCallback);
    m_server_sm.Fini();
    while (false == bNotifyCallBack)
    {
        m_client_sm.Update(true);
    }

    return 0;
}

