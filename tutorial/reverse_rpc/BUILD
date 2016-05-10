gen_rule(
    name = 'gen_rerpc',
    srcs = [
        'rerpc.pebble',
    ],
    cmd = '$BUILD_DIR/source/rpc/compiler/cpp/src/pebble -r -out $BUILD_DIR/tutorial/reverse_rpc --gen cpp $SRCS',
    deps = [
        '//source/rpc/compiler/cpp/src:pebble'
    ],
    outs = [
        'rerpc.cpp',
        'rerpc_BaseService.cpp',
        'rerpc_PushService.cpp',
    ],
)

cc_binary(
    name = 'rerpc_client',
    srcs = [
           'rerpc_client.cpp',
           'rerpc.cpp',
           'rerpc_BaseService.cpp',
           'rerpc_PushService.cpp',
    ],
    incs = [
        '../../source/rpc/',
        '../../source/',
    ],
    deps = [
            '//source/rpc:pebble_rpc',
            ':gen_rerpc'
    ]
)

cc_binary(
    name = 'rerpc_server',
    srcs = [
           'rerpc_server.cpp',
           'rerpc.cpp',
           'rerpc_BaseService.cpp',
           'rerpc_PushService.cpp',
    ],
    incs = [
        '../../source/app/',
        '../../source/',
    ],
    deps = [
            '//source/app:pebble_app',
            ':gen_rerpc'
    ]
)

