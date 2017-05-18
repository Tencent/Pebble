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

#include <ctime>
#include <sstream>

#include "common/dir_util.h"
#include "common/file_util.h"
#include "common/log.h"
#include "framework/gdata_api.h"
#include "framework/gdata_err.h"

namespace pebble {
namespace oss {


#define DEFAULT_LOG_FILE_PATH              "./log"
#define DEFAULT_LOG_FILE_PREFIX            "gdata"
#define DEFAULT_LOG_FILE_SUFFIX            "log"
#define DEFAULT_LOG_FILE_SEPARATOR         "."
#define DEFAULT_DATA_LOG_SEPARATOR         "&"
#define DEFAULT_LOG_DIR_SEPARATOR          "/"
#define DEFAULT_WATER_LOG_NAME             "water"
#define DEFAULT_MONITOR_LOG_NAME           "monitor"
#define DEFAULT_DATA_LOG_LAYOUT_FORMAT     "%m%n"
#define DEFAULT_TIMESTAMP_DATE_FORMAT      "%Y%m%d"
#define DEFAULT_TIMESTAMP_HOUR_FORMAT      "%H"

#define DEFAULT_LOG_MAX_ROTATE      10
#define DEFAULT_LOG_FILE_SIZE_LIMIT 20971520


static RollUtil* roll_util_water   = NULL;
static RollUtil* roll_util_monitor = NULL;
static char g_game_id[128] = {0};
static char g_unit_id[128] = {0};
static char g_server_id[128]   = {0};
static char g_instance_id[128] = {0};


/*
  *Convert data pair vector to string
  */
std::string DataPair2String(const std::vector<data_pair> &vec_data_pair) 
{
    std::ostringstream oss;
    oss.str("");
    for (unsigned int i = 0; i < vec_data_pair.size(); ++i) 
    {
        data_pair pair = vec_data_pair[i];
        oss << "&" << pair.first 
            << "=" << pair.second;
    }
    return oss.str();
}

/*
  *Time stamp format
  */
static std::string FormatTimeStamp(time_t utc_time, const std::string& format)
{
    if (utc_time < 0)
    {
        return std::string("null");
    }
    std::tm *ptm = std::localtime(&utc_time);
    if (NULL == ptm)
    {
        return std::string("null");
    }
    char buffer[64] = {0};
    std::strftime(buffer, sizeof(buffer), format.c_str(), ptm);
    return std::string(buffer);
}

int CLogDataAPI::InitDataLog(IN const SUniqueSeriesID& unique_series_id, IN const std::string& file_path)
{
    std::string data_file_path = file_path;
    snprintf(g_game_id, sizeof(g_game_id), "%s", unique_series_id.m_game_id.c_str());
    snprintf(g_unit_id, sizeof(g_unit_id), "%s", unique_series_id.m_unit_id.c_str());
    snprintf(g_server_id, sizeof(g_server_id), "%s", unique_series_id.m_server_id.c_str());
    snprintf(g_instance_id, sizeof(g_instance_id), "%s", unique_series_id.m_instance_id.c_str());
    if(data_file_path.empty())
    {
        data_file_path = DEFAULT_LOG_FILE_PATH;
    }

    if (DirUtil::MakeDirP(data_file_path) != 0) {
        PLOG_ERROR("mkdir %s failed %s", data_file_path.c_str(), DirUtil::GetLastError());
        return DATA_API_ERR_CREATE_FILE_PATH;
    }

    /*
     *Water log file path. e.g./data/pepple/gdata.water.2.2.1.1.log.*
     */
    std::ostringstream oss;
    oss.str("");
    oss << DEFAULT_LOG_FILE_PREFIX 
        << DEFAULT_LOG_FILE_SEPARATOR << DEFAULT_WATER_LOG_NAME
        << DEFAULT_LOG_FILE_SEPARATOR << g_game_id
        << DEFAULT_LOG_FILE_SEPARATOR << g_unit_id
        << DEFAULT_LOG_FILE_SEPARATOR << g_server_id
        << DEFAULT_LOG_FILE_SEPARATOR << g_instance_id
        << DEFAULT_LOG_FILE_SEPARATOR << DEFAULT_LOG_FILE_SUFFIX;
    std::string water_name = oss.str();

    /*
     *Monitor log file path. e.g./data/pepple/gdata.monitor.2.2.1.1.log.*
     */
    oss.str("");
    oss << DEFAULT_LOG_FILE_PREFIX 
        << DEFAULT_LOG_FILE_SEPARATOR << DEFAULT_MONITOR_LOG_NAME
        << DEFAULT_LOG_FILE_SEPARATOR << g_game_id
        << DEFAULT_LOG_FILE_SEPARATOR << g_unit_id
        << DEFAULT_LOG_FILE_SEPARATOR << g_server_id
        << DEFAULT_LOG_FILE_SEPARATOR << g_instance_id
        << DEFAULT_LOG_FILE_SEPARATOR << DEFAULT_LOG_FILE_SUFFIX;
    std::string monitor_name = oss.str();

    delete roll_util_water;
    roll_util_water = new RollUtil(data_file_path, water_name);
    roll_util_water->SetFileSize(DEFAULT_LOG_FILE_SIZE_LIMIT);
    roll_util_water->SetRollNum(DEFAULT_LOG_MAX_ROTATE);

    delete roll_util_monitor;
    roll_util_monitor = new RollUtil(data_file_path, monitor_name);
    roll_util_monitor->SetFileSize(DEFAULT_LOG_FILE_SIZE_LIMIT);
    roll_util_monitor->SetRollNum(DEFAULT_LOG_MAX_ROTATE);

    return DATA_LOG_SUCCESS;
}

void CLogDataAPI::LogWater(IN const SWaterData& water_data)
{
    if (roll_util_water == NULL) {
        return;
    }

    std::ostringstream oss;
    oss.str("");
    oss << "water_type=" << water_data.type 
        << DEFAULT_DATA_LOG_SEPARATOR << "buzid=" << water_data.business_id
        << DEFAULT_DATA_LOG_SEPARATOR << "logid=" << water_data.log_id
        << DEFAULT_DATA_LOG_SEPARATOR << "date=" << FormatTimeStamp(water_data.datetime, DEFAULT_TIMESTAMP_DATE_FORMAT)
        << DEFAULT_DATA_LOG_SEPARATOR << "hour=" << FormatTimeStamp(water_data.datetime, DEFAULT_TIMESTAMP_HOUR_FORMAT)
        << DEFAULT_DATA_LOG_SEPARATOR << "gameid=" << g_game_id 
        << DEFAULT_DATA_LOG_SEPARATOR << "unitid=" << g_unit_id
        << DEFAULT_DATA_LOG_SEPARATOR << "serverid=" << g_server_id 
        << DEFAULT_DATA_LOG_SEPARATOR << "instanceid=" << g_instance_id
        << DEFAULT_DATA_LOG_SEPARATOR << "ip=" << water_data.ip 
        << DEFAULT_DATA_LOG_SEPARATOR << "timestamp=" << water_data.datetime
        << DEFAULT_DATA_LOG_SEPARATOR << "transid=" << water_data.trans_id
        << DataPair2String(water_data.extend_data) << "\n";

    FILE* file = roll_util_water->GetFile();
    if (file) {
        fwrite(oss.str().c_str(), oss.str().length(), 1, file);
    }
}

void CLogDataAPI::LogMonitor(IN const SMonitorData& monitor_data) 
{
    if (roll_util_monitor == NULL) {
        return;
    }

    std::ostringstream oss;
    oss.str("");
    oss << "monitor_type=" << monitor_data.type
        << DEFAULT_DATA_LOG_SEPARATOR << "buzid=" << monitor_data.business_id
        << DEFAULT_DATA_LOG_SEPARATOR << "logid=" << monitor_data.log_id
        << DEFAULT_DATA_LOG_SEPARATOR << "date=" << FormatTimeStamp(monitor_data.datetime, DEFAULT_TIMESTAMP_DATE_FORMAT)
        << DEFAULT_DATA_LOG_SEPARATOR << "hour=" << FormatTimeStamp(monitor_data.datetime, DEFAULT_TIMESTAMP_HOUR_FORMAT)
        << DEFAULT_DATA_LOG_SEPARATOR << "gameid=" << g_game_id 
        << DEFAULT_DATA_LOG_SEPARATOR << "unitid=" << g_unit_id
        << DEFAULT_DATA_LOG_SEPARATOR << "serverid=" << g_server_id 
        << DEFAULT_DATA_LOG_SEPARATOR << "instanceid=" << g_instance_id 
        << DEFAULT_DATA_LOG_SEPARATOR << "service_name=" << monitor_data.service_name
        << DEFAULT_DATA_LOG_SEPARATOR << "interface_name=" << monitor_data.interface_name
        << DEFAULT_DATA_LOG_SEPARATOR << "cost_time=" << monitor_data.cost_time
        << DEFAULT_DATA_LOG_SEPARATOR << "return_msg=" << monitor_data.return_msg
        << DEFAULT_DATA_LOG_SEPARATOR << "src_ip=" << monitor_data.src_ip
        << DEFAULT_DATA_LOG_SEPARATOR << "dst_ip=" << monitor_data.dst_ip
        << DEFAULT_DATA_LOG_SEPARATOR << "count=" << monitor_data.counts
        << DEFAULT_DATA_LOG_SEPARATOR << "return_type=" << monitor_data.return_type
        << DEFAULT_DATA_LOG_SEPARATOR << "return_code=" << monitor_data.return_code
        << DEFAULT_DATA_LOG_SEPARATOR << "timestamp=" << monitor_data.datetime
        << DataPair2String(monitor_data.extend_data) << "\n";

    FILE* file = roll_util_monitor->GetFile();
    if (file) {
        fwrite(oss.str().c_str(), oss.str().length(), 1, file);
    }
}

void CLogDataAPI::FiniDataLog()
{
    delete roll_util_monitor;
    delete roll_util_water;
    roll_util_monitor = NULL;
    roll_util_water   = NULL;
}
}// namespace pebble
}  // namespace oss
