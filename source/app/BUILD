cc_library(
    name = 'pebble_app',
    srcs = [
            'pebble_server.cpp',
            'coroutine_rpc.cpp',
            'timer_task.cpp',
            'control_handler.cpp',
            'control_command.cpp',
			'./idl/control_service.cpp',
			'./idl/control_service_ControlCommand.cpp',
            'broadcast_mgr.cpp',
            'subscriber.cpp',
            'easy_coroutine.cpp',
    ],
    incs = [
            '../rpc/',
            '../'
    ],
    deps = [
            '//thirdparty/inih-master/:inih',
            '//source/rpc/:pebble_rpc',
            '//source/app/idl:gen_control_service',
            '//source/broadcast:pebble_broadcast',
            '//source/log:pebble_log',
	],
    optimize = 'O'
)

cc_binary(
    name = 'control_client',
    srcs = 'control_client.cpp',
    incs = [
    ],
    deps = [
            ':pebble_app',
    ],
)
