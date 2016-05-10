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
#include <string>
#include "source/common/coroutine.h"
#include "source/common/pebble_common.h"
#include "source/common/string_utility.h"
#include "source/common/zookeeper_client.h"
#include "source/log/log.h"
#include "source/service/address_puller.h"

namespace pebble {
namespace service {

using namespace cxx::placeholders;

struct ZkCoroutineRsp
{
    static const int32_t WAIT_FOR_RETURN = 99999;

    explicit ZkCoroutineRsp(CoroutineSchedule* cor_sche)
        : _rc(WAIT_FOR_RETURN), _cor_id(-1), _cor_sche(cor_sche) {}

    ~ZkCoroutineRsp() {}

    int32_t _rc;
    int64_t _cor_id;
    CoroutineSchedule* _cor_sche;

    std::vector<std::string> _childrens;
    std::string _value;

    void WaitRsp()
    {
        if (0 == _rc) {
            _rc = WAIT_FOR_RETURN;
            _cor_id = _cor_sche->CurrentTaskId();
            if (_cor_id >= 0) {
                _cor_sche->Yield();
            }
        }
    }

    void GetRsp(int rc, const char* value, int value_len,
                const Stat* stat)
    {
        _rc = rc;
        if (value) {
            _value.assign(value, value_len);
        }
        _cor_sche->Resume(_cor_id);
    }

    void GetChildrenRsp(int rc, const String_vector* strings,
        const Stat* stat)
    {
        _rc = rc;
        _childrens.clear();
        if (strings) {
            for (int i = 0; i < strings->count; i++) {
                _childrens.push_back(std::string(strings->data[i]));
            }
        }
        _cor_sche->Resume(_cor_id);
    }
};

inline bool IsZkRetry(int32_t rc)
{
    return (rc == kZK_CONNECTIONLOSS
            || rc == kZK_OPERATIONTIMEOUT
            || rc == kZK_CLOSING
            || rc == kZK_SESSIONMOVED);
}

//////////////////////////////////////////////////////////////////////////

class ServiceAddressPullerImp
{
public:
    explicit ServiceAddressPullerImp(ServiceAddressPuller* address_puller);
    ~ServiceAddressPullerImp();

    int32_t Init(const std::string& host, CoroutineSchedule* cor_schedule, int time_out = 5000);

    void RegisterCallback(ServiceAddrChanged cb);

    int32_t GetServiceAddress(int64_t game_id, const std::string& game_key,
        const std::string& service_name, std::vector<ServiceAddressInfoEx>* address);

    int32_t Update();

private:
    void WatchCallback(int event, const char* path);

    int AddDigestAuth(int64_t game_id, const std::string& game_key);

    int32_t String2Address(int unitid, const std::string& str,
        std::vector<ServiceAddressInfoEx>* addr_vec);

private:
    ServiceAddressPuller*   m_address_puller;
    CoroutineSchedule*      m_schedule;
    ZookeeperClient*        m_zk_client;
    ServiceAddrChanged      m_service_addr_changed;
};

ServiceAddressPullerImp::ServiceAddressPullerImp(ServiceAddressPuller* address_puller)
    : m_address_puller(address_puller), m_schedule(NULL), m_zk_client(NULL)
{
    m_zk_client = new ZookeeperClient;
}

ServiceAddressPullerImp::~ServiceAddressPullerImp()
{
    if (NULL != m_zk_client) {
        delete m_zk_client;
        m_zk_client = NULL;
    }
}

int32_t ServiceAddressPullerImp::Init(const std::string& host,
    CoroutineSchedule* cor_schedule, int time_out)
{
    m_schedule = cor_schedule;
    m_zk_client->Init(host, time_out, "/");
    m_zk_client->SetWatchCallback(
        cxx::bind(&ServiceAddressPullerImp::WatchCallback, this, _1, _2));
    return m_zk_client->Connect();
}

void ServiceAddressPullerImp::RegisterCallback(ServiceAddrChanged cb) {
    m_service_addr_changed = cb;
}

int32_t ServiceAddressPullerImp::GetServiceAddress(int64_t game_id,
    const std::string& game_key, const std::string& service_name,
    std::vector<ServiceAddressInfoEx>* address)
{
    if (m_schedule->CurrentTaskId() == -1) {
        return GET_FAILED;
    }

    int32_t ret = GET_SUCCESS;
    char node_name[512] = {0};
    ZkCoroutineRsp* cor_rsp = new ZkCoroutineRsp(m_schedule);

    do {
        ret = AddDigestAuth(game_id, game_key);
        if (ret != 0) {
            ret = GET_FAILED;
            break;
        }

        // 1. 获取gameid下的所有unit
        int32_t max_retry = 3;
        do {
            snprintf(node_name, sizeof(node_name), "/%ld/Runtime/UnitList", game_id);
            ZkStringsCompletionCb cob = cxx::bind(&ZkCoroutineRsp::GetChildrenRsp,
                cor_rsp, _1, _2, _3);
            cor_rsp->_rc = m_zk_client->AGetChildren(node_name, 1, cob);
            cor_rsp->WaitRsp();
        } while (IsZkRetry(cor_rsp->_rc) && (max_retry--));

        if (cor_rsp->_rc != kZK_OK) {
            ret = GET_PART_SUCCESS;
            PLOG_INFO("Get %s Children failed, ret = %d", node_name, cor_rsp->_rc);
            break;
        }

        std::vector<std::string> unit_vec = cor_rsp->_childrens;

        for (std::vector<std::string>::iterator it = unit_vec.begin();
            it != unit_vec.end(); ++it)
        {
            // 2. 获取每个unit下指定service name的实例
            snprintf(node_name, sizeof(node_name), "/%ld/Runtime/UnitList/%s/ServiceList/%s",
                game_id, (*it).c_str(), service_name.c_str());
            ZkStringsCompletionCb cob = cxx::bind(&ZkCoroutineRsp::GetChildrenRsp,
                cor_rsp, _1, _2, _3);
            cor_rsp->_rc = m_zk_client->AGetChildren(node_name, 1, cob);
            cor_rsp->WaitRsp();

            if (cor_rsp->_rc != kZK_OK) {
                ret = GET_PART_SUCCESS;
                PLOG_INFO("Get %s Children failed, ret = %d", node_name, cor_rsp->_rc);
                continue;
            }

            // 3. 获取每个实例的地址
            std::vector<std::string> instance_vec = cor_rsp->_childrens;

            for (std::vector<std::string>::iterator it1 = instance_vec.begin();
                it1 != instance_vec.end(); ++it1)
            {
                snprintf(node_name, sizeof(node_name),
                    "/%ld/Runtime/UnitList/%s/ServiceList/%s/%s",
                    game_id, (*it).c_str(), service_name.c_str(), (*it1).c_str());
                ZkDataCompletionCb cob = cxx::bind(&ZkCoroutineRsp::GetRsp,
                    cor_rsp, _1, _2, _3, _4);
                cor_rsp->_rc = m_zk_client->AGet(node_name, 1, cob);
                cor_rsp->WaitRsp();

                if (cor_rsp->_rc != kZK_OK) {
                    ret = GET_PART_SUCCESS;
                    PLOG_INFO("Get %s failed, ret = %d", node_name, cor_rsp->_rc);
                    continue;
                }

                // 4. 合并地址
                String2Address(atoi((*it).c_str()), cor_rsp->_value, address);
            }
        }
    } while (0);

    delete cor_rsp;

    // 中间有错误就会返回非0，结果不完整
    return ret;
}

int32_t ServiceAddressPullerImp::Update()
{
    return m_zk_client->Update(false);
}

void ServiceAddressPullerImp::WatchCallback(int event, const char* path)
{
    if (!path || 0 == *path) {
        return;
    }

    PLOG_DEBUG("WatchCallback, event = %d, path = %s", event, path);

    if (!m_service_addr_changed) {
        return;
    }

    int64_t game_id = -1;
    int unit_id = -1;
    std::string service_name;
    do {
        // "/gameid/Runtime/UnitList/unitid/ServiceList/xxx/instanceid"
        int len = strlen(path);
        if (len < static_cast<int>(strlen("/./Runtime/UnitList"))) {
            break;
        }

        char* nptr = const_cast<char*>(path + 1);
        char* endptr = NULL;

        game_id = strtoll(nptr, &endptr, 10);
        if (endptr == nptr || *endptr != '/') {
            break;
        }
        nptr = endptr + strlen("/Runtime/UnitList/");
        if (nptr - path > len) {
            break;
        }
        unit_id = strtoll(nptr, &endptr, 10);
        if (endptr == nptr || *endptr != '/') {
            break;
        }

        nptr = endptr + strlen("/ServiceList/");
        if (nptr - path > len) {
            break;
        }
        std::string endstr(nptr);
        service_name = endstr;
        size_t found = endstr.find('/');
        if (found != std::string::npos) {
            service_name = endstr.substr(0, found);
        }
    } while (0);

    PLOG_DEBUG("WatchCallback, gameid=%ld, unitid=%d, servicename=%s",
        game_id, unit_id, service_name.c_str());

    if (game_id < 0) {
        return;
    }

    // 通知到服务
    m_service_addr_changed(game_id, service_name);
}

int ServiceAddressPullerImp::AddDigestAuth(int64_t game_id, const std::string& game_key)
{
    char game_id_tmp[256] = {0};
    snprintf(game_id_tmp, sizeof(game_id_tmp), "%ld:%s", game_id, game_key.c_str());
    std::string user_auth = game_id_tmp;

    int ret = m_zk_client->AddDigestAuth(user_auth);
    PLOG_IF_ERROR(kSM_OK != ret, "zoo_add_auth error!ret:%d", ret);
    return ret;
}

int32_t ServiceAddressPullerImp::String2Address(int unitid, const std::string& str,
    std::vector<ServiceAddressInfoEx>* addr_vec)
{
    std::vector<std::string> addr_str;
    StringUtility::Split(str, ",", &addr_str);
    for (std::vector<std::string>::iterator it = addr_str.begin();
        it != addr_str.end(); ++it)
    {
        std::vector<std::string> info_item;
        StringUtility::Split(*it, "@", &info_item);
        if (2 == info_item.size()) {
            ServiceAddressInfoEx addr_item;
            addr_item.addr.protocol_type = atoi(info_item[0].c_str());
            addr_item.addr.address_url = StringUtility::Trim(info_item[1]);
            addr_item.unitid = unitid;
            addr_vec->push_back(addr_item);
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////

ServiceAddressPuller::ServiceAddressPuller()
    : m_address_puller_imp(NULL)
{
}

ServiceAddressPuller::~ServiceAddressPuller()
{
    if (NULL != m_address_puller_imp) {
        delete m_address_puller_imp;
        m_address_puller_imp = NULL;
    }
}

int32_t ServiceAddressPuller::Init(const std::string& zk_address,
    CoroutineSchedule* co, int time_out_ms)
{
    m_address_puller_imp = new ServiceAddressPullerImp(this);
    ServiceAddrChanged cb = cxx::bind(&ServiceAddressPuller::WachCallback, this, _1, _2);
    m_address_puller_imp->RegisterCallback(cb);
    return m_address_puller_imp->Init(zk_address, co, time_out_ms);
}

void ServiceAddressPuller::RegisterCallback(ServiceAddrChanged cb) {
    m_service_addr_changed = cb;
}

int32_t ServiceAddressPuller::GetServiceAddress(int64_t game_id,
    const std::string& game_key, const std::string& service_name,
    std::vector<ServiceAddressInfoEx>** address)
{
    std::pair<int64_t, std::string> key = std::make_pair(game_id, service_name);
    do {
        std::map<std::pair<int64_t, std::string>, ServiceData>::iterator it =
            m_service_address.find(key);

        if (m_service_address.end() == it) {
            // 无此游戏+服务的cache
            ServiceData servcie_data;
            servcie_data.m_doing    = true;
            servcie_data.m_gameid   = game_id;
            servcie_data.m_gamekey  = game_key;
            m_service_address[key]  = servcie_data;
            PLOG_DEBUG("not find %ld:%s cache", game_id, service_name.c_str());
            break;
        }

        if ((it->second).m_doing) {
            *address = NULL;
            return GET_DUPLICATE;
        }

        if (game_key != (it->second).m_gamekey) {
            // game_key 变化，需要重新获取
            (it->second).m_gamekey = game_key;
            (it->second).m_address.clear();
            PLOG_DEBUG("%ld's game_key changed", game_id);
            break;
        }

        if ((it->second).m_address.empty()) {
            // 为空时重新获取，防止首次get无效的问题
            break;
        }

        PLOG_DEBUG("use gameid=%ld service=%s cache", game_id, service_name.c_str());

        *address = &((it->second).m_address);
        return 0;
    } while (0);

    int ret = m_address_puller_imp->GetServiceAddress(game_id, game_key, service_name,
        &(m_service_address[key].m_address));

    *address = &(m_service_address[key].m_address);

    m_service_address[key].m_doing = false;

    return ret;
}

int32_t ServiceAddressPuller::Update()
{
    return m_address_puller_imp->Update();
}

void ServiceAddressPuller::WachCallback(int64_t game_id, const std::string& service_name)
{
    std::pair<int64_t, std::string> key = std::make_pair(game_id, service_name);
    std::map<std::pair<int64_t, std::string>, ServiceData>::iterator it =
        m_service_address.find(key);
    if (m_service_address.end() == it) {
        return;
    }

    if ((it->second).m_doing) {
        return;
    }

    if (!service_name.empty()) {
        m_service_address[key].m_address.clear();
    }

    if (m_service_addr_changed) {
        m_service_addr_changed(game_id, service_name);
    }
}

} // namespace service
} // namespace pebble

