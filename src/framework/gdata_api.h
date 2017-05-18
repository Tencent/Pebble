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

#ifndef _PEBBLE_EXTENSION_GDATA_API_H_
#define _PEBBLE_EXTENSION_GDATA_API_H_


#include <map>
#include <string>
#include <vector>

#ifndef IN
#define IN
#endif
#ifndef OUT
#define OUT
#endif
#ifndef INOUT
#define INOUT
#endif

// 替换掉gdata api对tlog的依赖，名字空间及接口、实现暂时保持不变，方便替换
namespace pebble {
namespace oss {

typedef std::pair<std::string, std::string> data_pair;

/*
 * 流水日志信息的结构体
 */
struct SWaterData 
{
    std::string type;                           /*流水类型，用户自定义，建议为英文，如 Pay ，Login等*/
    int32_t business_id;                        /*业务类型，必填*/
    int32_t log_id;                             /*日志类型，必填*/
    std::string trans_id;                       /*染色ID，可以为空*/
    std::string ip;                             /*ip地址*/
    std::vector<data_pair> extend_data;         /*自定义扩展数据，key-value格式*/
    time_t datetime;                            /*当前时间*/
    SWaterData(int32_t buzid, int32_t logid)
    {
        business_id = buzid;
        log_id      = logid;
        datetime    = 0;
    }
};

enum EReturnType 
{
    kSUC, kBUSERR, kSYSERR
};

/*
 * 告警日志信息的结构体
 */
struct SMonitorData 
{
    std::string type;                            /*监控类型，包括：Inner、 RPC、 DB、 Cache， Inner表示内部调用等*/
    int32_t business_id;                         /*业务类型，必填*/
    int32_t log_id;                              /*日志类型，必填*/
    std::string service_name;                    /*调用远端服务名，Inner表示内部调用*/
    std::string interface_name;                  /*接口名，自定义，可以设置为rpc的method名*/
    std::string return_msg;                      /*返回信息*/
    std::string src_ip;                          /*源IP*/
    std::string dst_ip;                          /*目标IP*/
    std::vector< data_pair > extend_data;        /*自定义扩展数据，key-value格式*/
    int32_t cost_time;                           /*时耗：ms*/
    int32_t counts;                              /*请求量，默认为1，单条上报*/
    enum EReturnType return_type;                /*返回类型（0：成功，1:业务错误，2：系统错误）*/
    int32_t return_code;                         /*返回码*/
    time_t datetime;                             /*当前时间*/
    SMonitorData(int32_t buzid, int32_t logid)
    {
        business_id = buzid;
        log_id      = logid;
        cost_time   = 0;
        counts      = 0;
        return_type = kSUC;
        return_code = 0;
        datetime    = 0;
    }
};


struct SUniqueSeriesID 
{ 
    std::string m_game_id;                       /*游戏ID号*/
    std::string m_unit_id;                       /*区ID号*/
    std::string m_server_id;                     /*服务进程类型ID*/
    std::string m_instance_id;                   /*进程号*/
    SUniqueSeriesID(const std::string& game_id,
                        const std::string& unit_id,
                        const std::string& svr_id,
                        const std::string& ins_id)
    {
        m_game_id = game_id;
        m_unit_id = unit_id;
        m_server_id = svr_id;
        m_instance_id = ins_id;
    }
};


class CLogDataAPI 
{
public:

    /*
     * 初始化Data Log
     * @param[in] file_path 日志文件路径e.g./data/gdata/
     * @param[in] unique_series_id 用来区分业务进程的唯一ID序列号
     * @return 0 成功
     *         非0失败
     * */
    static int InitDataLog(IN const SUniqueSeriesID& unique_series_id, IN const std::string& file_path);

    /*
     * 写流水日志
     * @param[in] water_data 流水日志结构体
     * @return 
     * */
    static void LogWater(IN const SWaterData& water_data);

    /*
     * 写告警日志
     * @param[in] water_data 告警日志结构体
     * @return 
     * */
    static void LogMonitor(IN const SMonitorData& monitor_data);

    /*
     * 销毁 Log
     * @return 
     * */
    static void FiniDataLog();
};

}  // namespace oss
}  // namespace pebble
#endif   // _PEBBLE_EXTENSION_GDATA_API_H_

