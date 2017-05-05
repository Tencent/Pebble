【目录说明】
------------------------------------------------------------------------------------------------------------------------
├── build64_release                 编译输出目录
├── doc                             文档
├── example                         示例
│   ├── pebble_cmdline              命令行处理示例
│   ├── pebble_ctrl_cmd             控制命令处理
│   ├── pebble_idl                  Pebble IDL语法详细说明
│   ├── pebble_in_swift             在swift框架中使用Pebble
│   ├── pebble_in_tapp              在Tapp框架中使用Pebble
│   └── pebble_server               PebbleServer应用示例
├── release                         用于发布打包
├── src                             框架源码目录
│   ├── client                      后台SDK，即PebbleClient
│   ├── common                      基础库，业务无关
│   ├── extension                   扩展库，对接第三方系统的实现
│   ├── framework                   核心框架，Pebble的基础机制、功能实现
│   └── server                      后台Server参考实现，即PebbleServer
├── test                            测试目录
│   ├── benchmark                   性能测试
│   ├── ci                          CI
│   └── stable                      稳定性
├── thirdparty                      依赖第3方（原则上都要从源码编译）
│   ├── gdata_api
│   ├── gflags
│   ├── gflags-2.0
│   ├── glog
│   ├── glog-0.3.2
│   ├── google
│   ├── gtest
│   ├── gtest-1.6.0
│   ├── mysql
│   ├── protobuf
│   ├── rapidjson
│   ├── tbuspp
│   ├── tsf4g
│   └── zookeeper
└── tools                           工具目录
    ├── blade                       Blade构建
    ├── code_format                 代码格式化脚本
    ├── compiler                    IDL编译器
    └── release                     打包脚本


【开发环境搭建和使用说明】
------------------------------------------------------------------------------------------------------------------------

请确保机器已安装python 2.6以上版本。因为python是公司tlinux标配，此处不再赘述如何安装，如有需要自行百度。
第一次使用需要安装必备软件，请使用root登录。

一、编译
1、如何安装blade
blade的内容已经放在目录tools/blade下

cd blade
./install
source ~/.bash_profile

2、如何安装scons
scons2.3.4的内容已经放在目录tools/scons下

cd scons
tar xvf scons-2.3.4.tar.gz
cd scons-2.3.4
python setup.py install

blade依赖scons 2.0以上版本。如有其他需要，可以到下面地址下载：
http://www.scons.org/download.php

4、编译所有内容
进入项目根目录
blade build ...

5、对blade使用还不熟悉
请看目录：tools/blade/doc


二、运行样例程序
在运行样例程序之前，确保所有编译都成功。
样例源代码所在目录为：
trunk/example/pebble_server

编译好的可执行程序目录为：
trunk/build64_release/example/pebble_server

1、启动server
cd trunk/build64_release/example/pebble_server
./server

2、启动client
打开另外一个终端
cd trunk/build64_release/example/pebble_server
./client




