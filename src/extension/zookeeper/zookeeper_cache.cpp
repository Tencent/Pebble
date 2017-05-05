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


#include "common/log.h"
#include "extension/zookeeper/zookeeper_cache.inh"

namespace pebble {

using namespace cxx::placeholders;

/// @brief 判断带*扩展符的服务名信息是否匹配
/// @return 0 匹配
/// @return >0 部分匹配，继续下一条的匹配
/// @return <0 完全不匹配，可以退出匹配了
int32_t IsPathMatched(const std::string& match_name,
                             const std::string& name_expanded)
{
    size_t wpos1 = match_name.find_first_of('*');
    if (0 != name_expanded.compare(0, wpos1, match_name, 0, wpos1)) {
        return -1;
    }

    size_t wpos2 = wpos1;
    while (std::string::npos != wpos1) {
        size_t sub_pos1 = match_name.find_first_of('/', wpos1);
        size_t sub_pos2 = name_expanded.find_first_of('/', wpos2);
        if (std::string::npos == sub_pos1 || std::string::npos == sub_pos2) {
            if (sub_pos1 != sub_pos2) {
                return 1;
            }
            size_t soff = name_expanded.size() - match_name.size() + wpos1 + 1;
            int32_t ret = name_expanded.compare(soff, std::string::npos,
                                                match_name, wpos1 + 1, std::string::npos);
            return (0 == ret ? 0 : 1);
        }
        size_t soff = sub_pos1 - wpos1 - 1;
        size_t next_pos = match_name.find_first_of('*', sub_pos1);
        size_t comp_len =
            (std::string::npos != next_pos ? (soff + next_pos - sub_pos1) : next_pos);
        if (name_expanded.compare(sub_pos2 - soff, comp_len, match_name, wpos1 + 1, comp_len)) {
            return 1;
        }
        wpos1 = next_pos;
        wpos2 = (std::string::npos != next_pos ? (sub_pos2 + next_pos - sub_pos1) : next_pos);
    }
    return 0;
}

ZookeeperCache::ZookeeperCache(ZookeeperClient* zookeeper_client)
    :   m_zk_client(zookeeper_client),
        m_refresh_time_ms(300000), m_invalid_time_ms(700000),
        m_curr_time(0), m_last_refresh(0)
{
}

ZookeeperCache::~ZookeeperCache()
{
}

int32_t ZookeeperCache::SetConfig(int32_t refresh_time_ms, int32_t invalid_time_ms)
{
    m_refresh_time_ms = refresh_time_ms;
    m_invalid_time_ms = invalid_time_ms;
    return 0;
}

void ZookeeperCache::Clear()
{
    m_value_change_cb = NULL;
    m_cache_infos.clear();
    m_cache_keys.clear();
}

int32_t ZookeeperCache::Get(const std::string& name, std::vector<std::string>* urls)
{
    if (m_last_refresh + m_invalid_time_ms < m_curr_time
        || m_cache_keys.end() == m_cache_keys.find(name)) {
        return -1;
    }

    urls->clear();
    std::map<std::string, CacheInfo>::iterator it =
        m_cache_infos.lower_bound(name.substr(0, name.find_first_of('*')));
    for ( ; it != m_cache_infos.end(); ++it) {
        int32_t ret = IsPathMatched(name, it->first);
        if (0 == ret) {
            urls->assign(it->second._name_info._urls.begin(),
                it->second._name_info._urls.end());
        } else if (ret < 0) {
            break;
        }
    }
    return 0;
}

void ZookeeperCache::OnDelete(const std::string& name)
{
    std::map<std::string, CacheInfo>::iterator it = m_cache_infos.find(name);
    if (it == m_cache_infos.end()) {
        return;
    }
    if (true == it->second._is_watch && NULL != m_value_change_cb) {
        m_value_change_cb(name, std::vector<std::string>());
    }
    m_cache_infos.erase(it);
}

void ZookeeperCache::OnGetCb(int rc, GetAllNameResults& results, const CbReturnValue& cb) // NOLINT
{
    if (cb == NULL) {
        return;
    }
    std::vector<std::string> urls;
    if (0 == rc) {
        GetAllNameResults::iterator it = results.begin();
        for (uint32_t idx = 0 ; it != results.end() ; ++it, ++idx) {
            for (std::vector<std::string>::iterator vit = it->second._urls.begin();
                vit != it->second._urls.end(); ++vit) {
                urls.push_back(*vit);
            }
        }
    }
    // 返回
    cb(rc, urls);
}

void ZookeeperCache::OnUpdateCb(const std::string& req_name, bool is_watch,
                                          int rc, GetAllNameResults& results, CbReturnValue cb) // NOLINT
{
    // 加入到定时更新中
    if ((kZK_NOAUTH != rc && kZK_BADARGUMENTS != rc)
        && NULL != cb
        && m_cache_keys.end() == m_cache_keys.find(req_name)) {
        m_cache_keys[req_name] = is_watch;
    }
    OnGetCb(rc, results, cb);
    // 更新cache
    if (0 == rc) {
        UpdateCache(results, is_watch);
    }
}

void ZookeeperCache::OnWatchCb(const std::string& req_name,
                                         int rc, GetAllNameResults& results, CbReturnCode cb) // NOLINT
{
    // 加入到定时更新中
    if (kZK_NOAUTH != rc && kZK_BADARGUMENTS != rc) {
        m_cache_keys[req_name] = true;
    }
    // 回调
    if (cb != NULL) {
        cb(rc);
    }
    // 更新cache
    if (0 == rc) {
        UpdateCache(results, true);
    }
}

void ZookeeperCache::Update()
{
    timeval now;
    gettimeofday(&now, NULL);

    m_curr_time = now.tv_sec * 1000 + now.tv_usec / 1000;
    if (m_curr_time > m_last_refresh + m_refresh_time_ms) {
        std::map<std::string, bool>::iterator it = m_cache_keys.begin();
        for ( ; it != m_cache_keys.end() ; ++it) {
            CbReturnValue null_cob = NULL;
            CbGetAllReturn get_cb = cxx::bind(&ZookeeperCache::OnUpdateCb,
                                              this, it->first, it->second, _1, _2, null_cob);
            GetExpandedValueHandle* op_handle = new GetExpandedValueHandle(m_zk_client);
            op_handle->Start(it->first, get_cb, true);
        }
        m_last_refresh = m_curr_time;
    }
}

void ZookeeperCache::UpdateCache(GetAllNameResults& results, bool is_watch) // NOLINT
{
    // 更新cache和回调通知
    std::map<std::string, CacheInfo>::iterator cache_it;
    GetAllNameResults::iterator bit = results.begin();
    for ( ; bit != results.end() ; ++bit)
    {
        cache_it = m_cache_infos.find(bit->first);
        // 节点存在，比较版本号
        if (cache_it != m_cache_infos.end())
        {
            if (cache_it->second._name_info._version >= bit->second._version
                && (true == cache_it->second._is_watch || false == is_watch)) {
                continue;
            }
            cache_it->second._name_info = bit->second;
            if ((is_watch || cache_it->second._is_watch) && NULL != m_value_change_cb) {
                cache_it->second._is_watch = true;
                m_value_change_cb(cache_it->second._name,
                                 cache_it->second._name_info._urls);
            }
        }
        else // 新增的节点
        {
            CacheInfo& new_cache = m_cache_infos[bit->first];
            new_cache._is_watch = is_watch;
            new_cache._name = bit->first;
            new_cache._name_info = bit->second;
            if (is_watch && NULL != m_value_change_cb) {
                m_value_change_cb(new_cache._name,
                                 new_cache._name_info._urls);
            }
        }
    }
}

} // namespace pebble

