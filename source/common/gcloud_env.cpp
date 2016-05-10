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


#include <cstdlib>
#include <cstring>
#include <vector>
#include "source/common/gcloud_env.h"
#include "source/common/string_utility.h"
#include "thirdparty/rapidjson/document.h"


namespace pebble {

static const char* m_para_tag       = "gcloud_env=";
static const char* m_para_app_id    = "tapp_id";
static const char* m_para_tbus_url  = "tapp_local-tbusmgr-url";
static const char* m_para_game_id   = "rpc_game_id";
static const char* m_para_game_key  = "rpc_game_key";
static const char* m_para_zk_addr   = "rpc_zk_address";

int GCloudEnv::Parse(int argc, const char** argv)
{
    for (int i = 0; i < argc; i++) {
        if (StringUtility::StartsWith(std::string(argv[i]), std::string(m_para_tag))) {
            return ParseJson(argv[i] + strlen(m_para_tag));
        }
    }

    return -1;
}

void GCloudEnv::Reset()
{
    m_game_id = -1;
    m_unit_id = -1;
    m_server_id = -1;
    m_instance_id = -1;
    m_game_key = "";
    m_app_id = "";
    m_zk_address = "";
    m_local_tbusmgr_url = "";
}

int GCloudEnv::ParseJson(const char* json_str)
{
    if (!json_str) {
        return -1;
    }

    rapidjson::Document doc;
    doc.Parse<rapidjson::kParseDefaultFlags>(json_str);

    if (doc.HasMember(m_para_game_key) && doc[m_para_game_key].IsString()) {
        m_game_key = doc[m_para_game_key].GetString();
    }
    if (doc.HasMember(m_para_zk_addr) && doc[m_para_zk_addr].IsString()) {
        m_zk_address = doc[m_para_zk_addr].GetString();
    }
    if (doc.HasMember(m_para_tbus_url) && doc[m_para_tbus_url].IsString()) {
        m_local_tbusmgr_url = doc[m_para_tbus_url].GetString();
    }
    if (doc.HasMember(m_para_game_id) && doc[m_para_game_id].IsNumber()) {
        m_game_id = doc[m_para_game_id].GetInt64();
    }
    if (doc.HasMember(m_para_app_id) && doc[m_para_app_id].IsString()) {
        m_app_id = doc[m_para_app_id].GetString();

        std::vector<std::string> vec;
        StringUtility::Split(m_app_id, ".", &vec);
        if (vec.size() != 3) {
            return -2;
        }
        m_unit_id       = atoi(vec[0].c_str());
        m_server_id     = atoi(vec[1].c_str());
        m_instance_id   = atoi(vec[2].c_str());
    }

    return 0;
}

}  // namespace pebble

