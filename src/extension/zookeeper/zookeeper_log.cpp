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

#include <stdio.h>

#include "common/file_util.h"
#include "common/log.h"
#include "extension/zookeeper/zookeeper_log.inh"
#include "zookeeper/include/zookeeper.h"


namespace pebble {

ZookeeperLog::ZookeeperLog() : m_roll_util(NULL) {
}

ZookeeperLog::~ZookeeperLog() {
    delete m_roll_util;
}

int32_t ZookeeperLog::Init() {
    SetLogLevel(ZOO_LOG_LEVEL_INFO);

    if (m_roll_util == NULL) {
        m_roll_util = new RollUtil();
    }
    m_roll_util->SetFileName("zookeeper.log");

    FILE* log = m_roll_util->GetFile();
    if (log == NULL) {
        PLOG_ERROR("open log file failed(%s).", m_roll_util->GetLastError());
        return -1;
    }

    zoo_set_log_stream(log);

    return 0;
}

int32_t ZookeeperLog::SetLogLevel(uint32_t level) {
    if (level < ZOO_LOG_LEVEL_ERROR || level > ZOO_LOG_LEVEL_DEBUG) {
        PLOG_ERROR("level invalid %d(zk log level between 1(err) - 4(debug))", level);
        return -1;
    }

    zoo_set_debug_level(static_cast<ZooLogLevel>(level));
    return 0;
}

int32_t ZookeeperLog::SetLogPath(const std::string& path) {
    if (m_roll_util == NULL) {
        m_roll_util = new RollUtil();
    }
    return m_roll_util->SetFilePath(path);
}

int32_t ZookeeperLog::SetFileSize(uint32_t file_size) {
    if (m_roll_util == NULL) {
        m_roll_util = new RollUtil();
    }
    return m_roll_util->SetFileSize(file_size);
}

int32_t ZookeeperLog::SetRollNum(uint32_t roll_num) {
    if (m_roll_util == NULL) {
        m_roll_util = new RollUtil();
    }
    return m_roll_util->SetRollNum(roll_num);
}

int32_t ZookeeperLog::Update() {
    if (!m_roll_util) {
        return 0;
    }

    FILE* log = m_roll_util->GetFile();
    if (log == NULL) {
        return -1;
    }
    zoo_set_log_stream(log);

    return 1;
}


} // namespace pebble

