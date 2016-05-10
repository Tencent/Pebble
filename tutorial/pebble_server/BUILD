gen_rule(
    name = 'gen_tutorial',
    srcs = [
        'tutorial.pebble',
    ],
    cmd = '$BUILD_DIR/source/rpc/compiler/cpp/src/pebble -r -out $BUILD_DIR/tutorial/pebble_server --gen cpp $SRCS',
    deps = [
        '//source/rpc/compiler/cpp/src:pebble'
    ],
    outs = [
        'tutorial.cpp',
        'tutorial_BaseService.cpp',
    ],
)

cc_binary(
    name = 'client',
    srcs = [
           'client.cpp',
           'tutorial.cpp',
           'tutorial_BaseService.cpp',
    ],
    incs = [
        '../../source/rpc/',
        '../../source/',
    ],
    deps = [
            '//source/rpc:pebble_rpc',
            ':gen_tutorial'
    ]
)

cc_binary(
    name = 'server',
    srcs = [
           'server.cpp',
           'tutorial.cpp',
           'tutorial_BaseService.cpp',
    ],
    incs = [
        '../../source/app/',
        '../../source/',
    ],
    deps = [
            '//source/app:pebble_app',
            ':gen_tutorial'
    ]
)

