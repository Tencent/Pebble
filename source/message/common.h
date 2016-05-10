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


#ifndef PEBBLE_NET_COMMON_h_
#define PEBBLE_NET_COMMON_h_

#ifdef __GNUC__
#define LIKELY(val) (__builtin_expect((val), 1))
#define UNLIKELY(val) (__builtin_expect((val), 0))
#else
#define LIKELY(val) (val)
#define UNLIKELY(val) (val)
#endif

#define NET_LIB_ERR_RETURN(msg) \
    snprintf(Message::error_msg, sizeof(Message::error_msg), "%s", (msg));\
    Message::p_error_msg = Message::error_msg;\
    return -1;

#define TBUS_ERR_RETURN(ret) \
    Message::p_error_msg = tsf4g_tbus::BusEnv::GetErrorStr(ret);\
    return -1;

#define ERRNO_RETURN() \
    Message::p_error_msg = strerror(errno);\
    return -1;

namespace pebble { namespace net {

} /* namespace pebble */ } /* namespace net */

#endif // PEBBLE_NET_COMMON_h_
