# HelloWorld

----------
##环境搭建
1、下载Pebble
首先，你得先到这里下载Pebble
2、编译安装
根据这篇文档，完成Pebble的编译、安装

##例子

###通过IDL定义前后端交互接口
	namespace cpp tutorial

	service Tutorial {
	   string helloworld(1: string who)
	}

###生成代码
####C++服务端
	pebble -gen cpp helloworld.pebble
####客户端（Unity）
	pebble -gen csharp helloworld.pebble

###C++服务端
####服务实现
	class TutorialHandler : public TutorialCobSvIf {
	public:
		void helloworld(const std::string& who, cxx::function<void(std::string const& _return)> cob) {
			std::cout << who << " say helloworld!" << std::endl;
			cob("Hello " + who + "!");
		}
	};
####服务启动
	// 初始化PebbleServer
	pebble::PebbleServer pebble_server;
	pebble_server.Init(argc, argv, "cfg/pebble.ini");
	// 注册服务
	TutorialHandler service;
	pebble_server.RegisterService(&service);
	// 添加服务监听地址
	pebble_server.AddServiceManner("http://0.0.0.0:8300", pebble::rpc::PROTOCOL_BINARY);
	// 启动server
	pebble_server.Start();


###Unity客户端
####初始化
	if (!HttpTransportCreator.Initial())
	{
		throw new Exception("Init HTTPTransportCreator fail!");
	}

	Rpc.Instance.SetDefaultCommunicationParam("http://10.12.234.103:8300");//调用这个API设置默认的通讯参数
	client = Rpc.Instance.GetClient<Tutorial.Client>();//可填通讯参数，不填时用SetDefaultCommunicationParam所设置的值

####每帧的Tick
	Rpc.Instance.Update();

####RPC调用
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






