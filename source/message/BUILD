cc_library(
    name = 'pebble_message',
    srcs = [
               'message.cpp',
               'http_parser.c',
               'http_driver.cpp',
               'tcp_driver.cpp',
               'connection_mgr.cpp',
    ],
    incs = [
    ],
    deps = [
        '//source/rpc/transport/:pebble_rpc_transport',
        '//source/common/:pebble_common',
        '#zmq',
        '#rt',
    ]
)

