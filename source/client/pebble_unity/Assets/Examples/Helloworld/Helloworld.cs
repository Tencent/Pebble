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
using Pebble;
using System;

public class Helloworld : MonoBehaviour
{

    // Use this for initialization
    void Start()
    {
        if (!HttpTransportCreator.Initial())
        {
            throw new Exception("Init HTTPTransportCreator fail!");
        }

        Rpc.Instance.SetDefaultCommunicationParam("http://10.12.234.103:8300");//调用这个API设置默认的通讯参数
        client = Rpc.Instance.GetClient<Tutorial.Client>();//可填通讯参数，不填时用SetDefaultCommunicationParam所设置的值
    }

    Tutorial.Client client = null;

    // Update is called once per frame
    void Update()
    {
        Rpc.Instance.Update();
    }

    void OnGUI()
    {
        if (GUI.Button(new Rect(10, 100, 200, 60), "Hello"))
        {
            client.helloworld("John", (ex, result) =>
            {
                if (ex != null)
                {
                    Debug.Log("RPC BaseService.heartbeat exception: " + ex);
                }
                else
                {
                    Debug.Log("RPC BaseService.heartbeat return: " + result);
                }
            });
        }
    }
}
