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


#ifndef _PEBBLE_EXTENSION_ZOOKEEPER_ERROR_H_
#define _PEBBLE_EXTENSION_ZOOKEEPER_ERROR_H_


namespace pebble {

/// @brief zk调用返回码
enum ZookeeperErrorCode {
    kZK_OK = 0, ///< Everything is OK
    kZK_ERROR_BASE = 0,
    kZK_SYSTEMERROR = kZK_ERROR_BASE - 1, ///< System and server-side errors.
    kZK_RUNTIMEINCONSISTENCY = kZK_ERROR_BASE - 2, ///< A runtime inconsistency was found
    kZK_DATAINCONSISTENCY = kZK_ERROR_BASE - 3, ///< A data inconsistency was found
    kZK_CONNECTIONLOSS = kZK_ERROR_BASE - 4, ///< Connection to the server has been lost
    kZK_MARSHALLINGERROR = kZK_ERROR_BASE - 5, ///< Error while marshalling or unmarshalling data
    kZK_UNIMPLEMENTED = kZK_ERROR_BASE - 6, ///< Operation is unimplemented
    kZK_OPERATIONTIMEOUT = kZK_ERROR_BASE - 7, ///< Operation timeout
    kZK_BADARGUMENTS = kZK_ERROR_BASE - 8, ///< Invalid arguments
    kZK_INVALIDSTATE = kZK_ERROR_BASE - 9, ///< Invliad zhandle state
    kSM_DNSFAILURE = kZK_ERROR_BASE - 10, ///< Error occurs when dns lookup

    kZK_APIERROR = kZK_ERROR_BASE - 100,
    kZK_NONODE = kZK_ERROR_BASE - 101, ///< Node does not exist
    kZK_NOAUTH = kZK_ERROR_BASE - 102, ///< Not authenticated
    kZK_BADVERSION = kZK_ERROR_BASE - 103, ///< Version conflict
    kZK_NOCHILDRENFOREPHEMERALS = kZK_ERROR_BASE - 108, ///< Ephemeral nodes may not have children
    kZK_NODEEXISTS = kZK_ERROR_BASE - 110, ///< The node already exists
    kZK_NOTEMPTY = kZK_ERROR_BASE - 111, ///< The node has children
    kZK_SESSIONEXPIRED = kZK_ERROR_BASE - 112, ///< The session has been expired by the server
    kZK_INVALIDCALLBACK = kZK_ERROR_BASE - 113, ///< Invalid callback specified
    kZK_INVALIDACL = kZK_ERROR_BASE - 114, ///< Invalid ACL specified
    kZK_AUTHFAILED = kZK_ERROR_BASE - 115, ///< Client authentication failed
    kZK_CLOSING = kZK_ERROR_BASE - 116, ///< ZooKeeper is closing
    kZK_NOTHING = kZK_ERROR_BASE - 117, ///< (not error) no server responses to process
    kZK_SESSIONMOVED = kZK_ERROR_BASE - 118, ///< session moved to another server, so operation is ignored
    kZK_NOQUOTA = kZK_ERROR_BASE - 119, ///< quota is not enough.
    kZK_SERVEROVERLOAD = kZK_ERROR_BASE - 120, ///< server overload.
    kZK_NOT_SET_APPKEY = kZK_ERROR_BASE - 200, ///< 未设置appkey

};

} // namespace pebble


#endif  // _PEBBLE_EXTENSION_ZOOKEEPER_ERROR_H_

