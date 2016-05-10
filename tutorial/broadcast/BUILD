gen_rule(
    name = 'gen_broadcast_tutorial',
    srcs = [
        'broadcast_tutorial.pebble',
    ],
    cmd = 'mkdir $BUILD_DIR/tutorial/broadcast/idl -p && $BUILD_DIR/source/rpc/compiler/cpp/src/pebble -r -out $BUILD_DIR/tutorial/broadcast/idl --gen cpp $SRCS',
    deps = [
        '//source/rpc/compiler/cpp/src:pebble'
    ],
    outs = [
        'idl/broadcast_tutorial.cpp',
        'idl/broadcast_tutorial_BroadcastTest.cpp',
        'idl/broadcast_tutorial_BroadcastTrigger.cpp',
        'idl/broadcast_tutorial_BroadcastTest_if.h',
        'idl/broadcast_tutorial_BroadcastTrigger_if.h',
        'idl/broadcast_tutorial.h',
    ],
)

cc_binary(
    name = 'broadcast_client',
    srcs = [
           'broadcast_client.cpp',
           'idl/broadcast_tutorial.cpp',
           'idl/broadcast_tutorial_BroadcastTrigger.cpp',
    ],
    incs = [
        '../../source/',
    ],
    deps = [
            '//source/rpc:pebble_rpc',
            ':gen_broadcast_tutorial',
    ]
)

cc_binary(
    name = 'broadcast_server',
    srcs = [
           'broadcast_server.cpp',
           'idl/broadcast_tutorial.cpp',
           'idl/broadcast_tutorial_BroadcastTest.cpp',
           'idl/broadcast_tutorial_BroadcastTrigger.cpp',
    ],
    incs = [
        '../../source/',
    ],
    deps = [
            '//source/app:pebble_app',
            ':gen_broadcast_tutorial',
            '//source/broadcast:pebble_broadcast',
    ]
)

