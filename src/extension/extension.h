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


#ifndef PEBBLE_EXTENSION_EXTENSION_H
#define PEBBLE_EXTENSION_EXTENSION_H

//#include "extension/pipe/pipe_processor.h"
//#include "extension/tbuspp/tbuspp_message.h"
//#include "extension/tbuspp/tbuspp_naming.h"
//#include "extension/tbuspp/tbuspp_router.h"
#include "extension/zookeeper/zookeeper_naming.h"
#include "common/log.h"


// 扩展插件安装 : 对于Pebble的扩展部分，为避免显式依赖，需要在使用时显式安装，不使用时不用安装

/// @brief 安装Tbuspp
#define INSTALL_TBUSPP(ret) \
    do { \
        (ret) = pebble::TbusppMessage::Instance()->Init(); \
        if ((ret) != 0) { \
            PLOG_ERROR("TbusppMessage Init failed, ret_code: %d, ret_msg: %s", \
                       ret, pebble::TbusppMessage::Instance()->GetLastError()); \
            break; \
        } \
        pebble::Message::SetMessageDriver(pebble::TbusppMessage::Instance()); \
        cxx::shared_ptr<pebble::NamingFactory> naming_factory(new pebble::TbusppNamingFactory()); \
        ret = pebble::SetNamingFactory(pebble::kNAMING_TBUSPP, naming_factory); \
        if ((ret) != 0) { \
            PLOG_ERROR("SetNamingFactory failed, ret_code: %d", ret); \
            break; \
        } \
        cxx::shared_ptr<pebble::RouterFactory> router_factory(new pebble::TbusppRouterFactory()); \
        (ret) = pebble::SetRouterFactory(pebble::kROUTER_TBUSPP, router_factory); \
        if ((ret) != 0) { \
            PLOG_ERROR("SetRouterFactory failed, ret_code: %d", ret); \
            break; \
        } \
    } while (0)


/// @brief 安装zookeeper名字服务(使用广播功能时需要)
#define INSTALL_ZOOKEEPER_NAMING(ret) \
    do { \
        cxx::shared_ptr<pebble::NamingFactory> naming_factory(new pebble::ZookeeperNamingFactory()); \
        (ret) = pebble::SetNamingFactory(pebble::kNAMING_ZOOKEEPER, naming_factory); \
        if ((ret) != 0) { \
            PLOG_ERROR("SetNamingFactory failed, ret_code: %d", ret); \
            break; \
        } \
    } while (0)


/// @brief 安装Pipe消息处理器(对接gconnd需要)
#define INSTALL_PIPE_PROCESSOR(ret) \
    do { \
        pebble::PipeErrorStringRegister::RegisterErrorString(); \
        cxx::shared_ptr<pebble::ProcessorFactory> processor_factory(new pebble::PipeProcessorFactory()); \
        (ret) = pebble::SetProcessorFactory(pebble::kPEBBLE_PIPE, processor_factory); \
        if ((ret) != 0) { \
            PLOG_ERROR("SetProcessorFactory failed, ret_code: %d", ret); \
            break; \
        } \
    } while (0)


namespace pebble {
} // namespace pebble

#endif // PEBBLE_EXTENSION_EXTENSION_H

