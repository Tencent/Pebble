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
#include <sstream>
#include "framework/naming.h"

namespace pebble {

int32_t Naming::MakeName(int64_t app_id,
                         const std::string& service_dir,
                         const std::string& service,
                         std::string* name)
{
    std::ostringstream out_stream;
    out_stream << "/" << app_id;
    if (false == service_dir.empty() && service_dir[0] != '/') {
        out_stream << "/";
    }
    out_stream << service_dir;
    if (true == service_dir.empty() || service_dir[service_dir.size() - 1] != '/') {
        out_stream << "/";
    }
    out_stream << service;
    name->assign(out_stream.str());
    for (size_t p = name->find("//"); p != std::string::npos; p = name->find("//", p)) {
        name->erase(p, 1);
    }
    return 0;
}

int32_t Naming::MakeTbusppUrl(const std::string& name, int64_t inst_id, std::string* url)
{
    url->assign("tbuspp://");
    for (uint32_t idx = 1 ; idx < name.size() ; ++idx) {
        if ('/' == name[idx]) {
            url->append(1, '.');
        } else {
            url->append(1, name[idx]);
        }
    }
    char tmp[32] = { 0 };
    snprintf(tmp, sizeof(tmp), "/%ld", inst_id);
    url->append(tmp);
    return 0;
}

int32_t Naming::FormatNameStr(std::string* name)
{
    if (NULL == name || name->empty()) {
        return -1;
    }
    // 如果name不含有'/'则将'.'转为'/'
    if (std::string::npos == name->find('/')) {
        uint64_t pos = name->find('.');
        while (std::string::npos != pos) {
            name->at(pos) = '/';
            pos = name->find('.', pos);
        }
    }
    // 开始处没有'/'，则补上
    if (name->at(0) != '/') {
        name->insert(0, 1, '/');
    }
    // 结尾处有'/'，则去掉
    if (name->size() > 1 && name->at(name->size() - 1) == '/') {
        name->erase(name->size() - 1, 1);
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// 避免全局变量构造、析构顺序问题
static cxx::unordered_map<int32_t, cxx::shared_ptr<NamingFactory> > * g_naming_factory_map = NULL;
struct NamingFactoryMapHolder {
    NamingFactoryMapHolder() {
        g_naming_factory_map = &_naming_factory_map;
    }
    ~NamingFactoryMapHolder() {
        g_naming_factory_map = NULL;
    }
    cxx::unordered_map<int32_t, cxx::shared_ptr<NamingFactory> > _naming_factory_map;
};

cxx::shared_ptr<NamingFactory> GetNamingFactory(int32_t type) {
    cxx::shared_ptr<NamingFactory> null_factory;
    if (!g_naming_factory_map) {
        return null_factory;
    }

    cxx::unordered_map<int32_t, cxx::shared_ptr<NamingFactory> >::iterator it =
        g_naming_factory_map->find(type);
    if (it != g_naming_factory_map->end()) {
        return it->second;
    }

    return null_factory;
}

int32_t SetNamingFactory(int32_t type, const cxx::shared_ptr<NamingFactory>& factory) {
    static NamingFactoryMapHolder naming_factory_map_holder;
    if (!factory) {
        return kNAMING_INVAILD_PARAM;
    }
    if (g_naming_factory_map == NULL) {
        return kNAMING_FACTORY_MAP_NULL;
    }
    if (false == g_naming_factory_map->insert({type, factory}).second) {
        return kNAMING_FACTORY_EXISTED;
    }
    return 0;
}

} // namespace pebble

