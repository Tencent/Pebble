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

using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System;
using System.Linq;

using Pebble;
using Pebble.Collections;
using Pebble.Protocol;
using Pebble.Transport;

namespace Pebble {

    public class Rpc {
        private static Rpc instance_;

        public static Rpc Instance
        {
            get
            {
                if (instance_ == null)
                {
                    instance_ = new Rpc();
                }
                return instance_;
            }
        }

        private Dictionary<long, AsyncRpcSessoin> async_rpc_sessions_ = new Dictionary<long, AsyncRpcSessoin>();
        private Dictionary<string, Func<string, Hashtable, TTransport>> creators_ = new Dictionary<string, Func<string, Hashtable, TTransport>>();
        private Dictionary<string, TProtocol> protocols_ = new Dictionary<string, TProtocol>();
        private Dictionary<string, TProcessor> service_processor_map_ = new Dictionary<string, TProcessor>();
        private long seqid_;  // 发包序列号

        public void RegisterTransportCreator(string protocol, Func<string, Hashtable, TTransport> creator)
        {
            creators_[protocol.ToLower()] = creator;
        }

        public enum ProtocolType
        {
            BINARY,
            JSON
        }

        string default_url_ = null;

        public void SetDefaultCommunicationParam(string url, Hashtable cfg = null, ProtocolType protocol_type = ProtocolType.BINARY) {
            addProtocol(url, cfg, protocol_type);
            default_url_ = url;
        }

        private void addProtocol(string url, Hashtable cfg, ProtocolType protocol_type)
        {
            if (protocol_type != ProtocolType.BINARY && protocol_type != ProtocolType.JSON)
            {
                throw new ArgumentException("no support for " + protocol_type);
            }
            if (!protocols_.ContainsKey(url))
            {
                string p_url = url.Substring(0, url.IndexOf(':'));
                TTransport transport = creators_[p_url](url, cfg);
                transport.Open();
                if (protocol_type == ProtocolType.BINARY)
                {
                    protocols_[url] = new TBinaryProtocol(transport);
                }
                else if (protocol_type == ProtocolType.JSON)
                {
                    protocols_[url] = new TJSONProtocol(transport);
                }
            }
        }

        public T GetClient<T>() where T : class
        {
            if (default_url_ == null)
            {
                throw new ArgumentException("you must call SetDefaultCommunicationParam first!");
            }
            return GetClient<T>(default_url_);
        }

        public T GetClient<T>(string url, ProtocolType protocol_type) where T : class
        {
            return GetClient<T>(url, null, protocol_type);
        }

        public T GetClient<T>(string url, Hashtable cfg = null, ProtocolType protocol_type = ProtocolType.BINARY) where T : class
        {
            url = url.ToLower();
            addProtocol(url, cfg, protocol_type);
            TProtocol protocol = protocols_[url];
            T ret = Activator.CreateInstance(typeof(T), new object[] { protocol }) as T;
            
            return ret;
        }

        public int RegisterService(object obj)
        {
            bool is_handler = false;
            foreach (var iftype in obj.GetType().GetInterfaces())
            {
                string ifname = iftype.FullName;
                int iface_pos = ifname.IndexOf("+Iface");
                if (iface_pos > 0)
                {
                    Type ptype = Type.GetType(ifname.Substring(0, iface_pos) + "+Processor");
                    if (ptype != null)
                    {
                        TProcessor processor = Activator.CreateInstance(ptype, new object[] { obj }) as TProcessor;
                        if (processor != null)
                        {
                            service_processor_map_.Add(processor.ServiceName(), processor);
                            is_handler = true;
                        }
                    }
                }
            }
            if (!is_handler)
            {
                throw new ArgumentException("RegisterService accept rpc handler only");
            }
            return 0;
        }


        public long AddNewSession(AsyncRpcSessoin ars)
        {
            lock (this)
            {
                seqid_++;
                async_rpc_sessions_[seqid_] = ars;
                return seqid_;
            }
        }

        /// <summary>
        /// 定期调用以驱动异步代码回调
        /// </summary>
        public int Update()
        {
            int ret = 0;
            foreach (var iprot in protocols_.Values)
            {
                // 无数据就直接返回，将来做成个循环条件
                bool has_data = iprot.Transport.Peek();
                if (!has_data)
                {
                    // 用来准备清理超时会话的变量，只选择空闲时做清理
                    DateTime now = DateTime.Now;
                    async_rpc_sessions_ = async_rpc_sessions_.Where((kv) =>
                    {
                        TimeSpan spend_time = now - kv.Value.CreateTime;
                        //清理超时会话
                        if (spend_time.Seconds >= kv.Value.Timeout)
                        {
                            try
                            {
                                kv.Value.Callback(new TimeoutException("session timeout " + spend_time.Seconds + " seconds."));
                            }
                            catch (Exception e)
                            {
                                Debug.LogError("timeout callback exception:" + e + ", stack:" + e.StackTrace);
                            }
                            return false;
                        }
                        else
                        {
                            return true;
                        }
                    }).ToDictionary((kv) => kv.Key, (kv) => kv.Value);

                    continue;
                }

                try
                {
                    // 读取包头准备分发
                    TMessage msg = iprot.ReadMessageBegin();

                    // 按消息类型分发
                    if (TMessageType.Call == msg.Type
                        || TMessageType.Oneway == msg.Type)
                    {
                        int index = msg.Name.IndexOf(":");
                        if (index < 0)
                        {
                            TProtocolUtil.Skip(iprot, TType.Struct);
                            iprot.ReadMessageEnd();
                            Debug.LogError("invalid msg." + msg.ToString());
                            ret++;
                            continue;
                        }

                        string serviceName = msg.Name.Substring(0, index);
                        TProcessor processor;
                        if (!service_processor_map_.TryGetValue(serviceName, out processor))
                        {
                            TProtocolUtil.Skip(iprot, TType.Struct);
                            iprot.ReadMessageEnd();
                            Debug.LogError("calling unexisted service.");
                            ret++;
                            continue;
                        }

                        TMessage newMessage = new TMessage(
                                    msg.Name.Substring(serviceName.Length + 1),
                                    msg.Type,
                                    msg.SeqID);
                        processor.Process(new StoredMessageProtocol(iprot, newMessage), iprot);
                        ret++;
                        continue;
                    }

                    AsyncRpcSessoin async_rpc_session;
                    if (!async_rpc_sessions_.TryGetValue(msg.SeqID, out async_rpc_session))
                    {
                        TProtocolUtil.Skip(iprot, TType.Struct);
                        iprot.ReadMessageEnd();
                        ret++;
                        continue;
                    }
                    async_rpc_sessions_.Remove(msg.SeqID);

                    if (msg.Type == TMessageType.Exception)
                    {
                        TApplicationException aex = TApplicationException.Read(iprot);
                        iprot.ReadMessageEnd();
                        async_rpc_session.Callback(aex);
                    }
                    else
                    {
                        async_rpc_session.Callback(null);
                    }

                    ret++;
                }
                catch(Exception e)
                {
                    byte[] tmp = new byte[128];
                    while(iprot.Transport.Read(tmp, 0, tmp.Length) > 0) { } //clear read buffer
                    throw e;
                }
            }
            return ret;
        }
    }
}