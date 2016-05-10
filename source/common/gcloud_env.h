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


#ifndef PEBBLE_COMMON_GCLOUD_ENV_H
#define PEBBLE_COMMON_GCLOUD_ENV_H
#include <string>
#include <stdint.h>

namespace pebble {

/// @brief 根据命令参数获取pebble运行所需的参数
/// ./program gcloud_env=value(json)
///    gcloud_env=
///    {
///        "tapp_id":"0.0.1",
///        "tapp_local-tbusmgr-url":"udp://127.0.0.1:1599",
///        "rpc_game_id":1,
///        "rpc_game_key":"game_key",
///        "rpc_zk_address":"127.0.0.1:2181"
///    }
/// @note 环境参数可以包含上述部分字段
class GCloudEnv {
public:
    virtual ~GCloudEnv() {}

    static GCloudEnv& Instance() {
        static GCloudEnv _instance;
        return _instance;
    }

    /// @brief 传入命令行参数，若参数中包含pebble_env，则解析pebble环境信息
    /// @note pebble_env一般由运营系统组装好传给启动的程序，格式按照类头部的描述
    int Parse(int argc, const char** argv);

    /// @brief 参数重置为默认值
    void Reset();

    int64_t GameId() const { return m_game_id; }

    int32_t UnitId() const { return m_unit_id; }

    int32_t ServerId() const { return m_server_id; }

    int32_t InstanceId() const { return m_instance_id; }

    const std::string& GameKey() const { return m_game_key; }

    const std::string& AppId() const { return m_app_id; }

    const std::string& ZkAddress() const { return m_zk_address; }

    const std::string& LocalTbusmgrUrl() const { return m_local_tbusmgr_url; }

private:
    GCloudEnv() : m_game_id(-1), m_unit_id(-1), m_server_id(-1), m_instance_id(-1) {}

    int ParseJson(const char* para);

private:
    int64_t m_game_id;
    int32_t m_unit_id;
    int32_t m_server_id;
    int32_t m_instance_id;
    std::string m_game_key;
    std::string m_app_id;
    std::string m_zk_address;
    std::string m_local_tbusmgr_url;
};

}  // namespace pebble

#endif  // PEBBLE_COMMON_GCLOUD_ENV_H

