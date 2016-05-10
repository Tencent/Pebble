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
using System.Collections;
using System.Threading;
using UnityEngine;
using System.Text;
using System.Collections.Generic;

namespace Pebble.Transport
{
    public class THTTPTransport : TTransport
    {
        private string url;

        private List<WWW> www_list = new List<WWW>();

        public THTTPTransport(string url)
        {
            this.url = url;
        }

        public override void Open()
        {
            
        }

        public override void Close()
        {
            Dispose(true);
        }

        protected override void Dispose(bool disposing)
        {
            foreach (var www in www_list)
            {
                www.Dispose();
            }
            www_list.Clear();
        }

        public override bool IsOpen
        {
            get
            {
                return true;
            }
        }

        private byte[] read_buffer = null;
        private int read_pos = 0;

        public bool hasMessage()
        {
            if (read_buffer != null && read_pos >= read_buffer.Length)
            {
                read_buffer = null;
                read_pos = 0;
            }
            if (read_buffer == null)
            {
                List<WWW> new_www_list = new List<WWW>();
                for (int i = 0; i < www_list.Count; i++)
                {
                    if(www_list[i].isDone)
                    {
                        if (string.IsNullOrEmpty(www_list[i].error)) // ok
                        {
                            if (read_buffer == null)
                            {
                                read_buffer = www_list[i].bytes;
                            }
                            else
                            {
                                new_www_list.Add(www_list[i]);
                            }
                        }
                    }
                    else
                    {
                        new_www_list.Add(www_list[i]);
                    }
                }
                if(new_www_list.Count != www_list.Count)
                {
                    www_list = new_www_list;
                }
            }
            return read_buffer != null && read_pos < read_buffer.Length;
        }

        public override int Read(byte[] buf, int off, int len)
        {
            if(!hasMessage())
            {
                return 0;
            }

            len = Math.Min(len, read_buffer.Length - read_pos);
            Array.Copy(read_buffer, read_pos, buf, off, len);
            read_pos += len;

            return len;
        }

        private byte[] write_buffer = null;
        private int write_pos = 0;

        public override void Write(byte[] buf, int off, int len)
        {
            if (write_buffer == null)
            {
                write_buffer = new byte[Math.Max(512, len * 2)];
            }
            else if (write_buffer.Length < write_pos + len)
            {
                byte[] new_write_buffer = new byte[(write_pos + len) * 2];
                write_buffer.CopyTo(new_write_buffer, 0);
                write_buffer = new_write_buffer;
            }
            Array.Copy(buf, off, write_buffer, write_pos, len);
            write_pos += len;
        }

        public override void Flush()
        {
            WWW www = new WWW(url, write_buffer);
            write_buffer = null;
            write_pos = 0;

            www_list.Add(www);
        }

        public override string ServiceName
        {
            set {}
        }
    }
}