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

using System;
namespace Pebble
{
    public delegate int AsyncRpcReplyCallback(Exception e);
    public class AsyncRpcSessoin
    {
        private DateTime _create_time;
        public DateTime CreateTime
        {
            get { return _create_time; }
        }

        private int _timeout;

        public int Timeout
        {
            get { return _timeout; }
        }

        private AsyncRpcReplyCallback _callback;
        public AsyncRpcReplyCallback Callback
        {
            get { return _callback; }
        }

        public AsyncRpcSessoin(AsyncRpcReplyCallback callback, int timeout = 5 * 1000)
        {
            _create_time = DateTime.Now;
            _callback = callback;
            _timeout = timeout == -1 ? 5 : Math.Max(1, timeout / 1000);
        }
    }
}
