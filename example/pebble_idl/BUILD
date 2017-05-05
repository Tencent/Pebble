gen_rule(
    name = 'gen_idl',
    srcs = [
        'idl.pebble',
    ],
    cmd = '$BUILD_DIR/tools/compiler/dr/pebble -out $BUILD_DIR/example/pebble_idl --gen cpp $SRCS',
    deps = [
        '//tools/compiler/dr:pebble',
    ],
    outs = [
        'idl.cpp',
        'idl.h',
        'idl_UserInfoManager.h',
        'idl_UserInfoManager.inh',
        'idl_UserInfoManager.cpp',
    ],
)

gen_rule(
    name = 'gen_idlex',
    srcs = [
        'idlex.pebble',
    ],
    cmd = '$BUILD_DIR/tools/compiler/dr/pebble -out $BUILD_DIR/example/pebble_idl --gen cpp $SRCS',
    deps = [
        '//tools/compiler/dr:pebble',
    ],
    outs = [
        'idlex.cpp',
        'idlex.h',
        'idlex_UserInfoManagerEx.h',
        'idlex_UserInfoManagerEx.inh',
        'idlex_UserInfoManagerEx.cpp',
    ],
)

cc_library(
    name = 'idl',
    srcs = [
        'idl.cpp',
        'idl_UserInfoManager.cpp',
        'idlex.cpp',
        'idlex_UserInfoManagerEx.cpp',
    ],
    incs = [
    ],
    deps = [
        ':gen_idl',
        ':gen_idlex',
        '//src/framework/:pebble_framework',
    ]
)

cc_binary(
    name = 'client',
    srcs = [
        'client.cpp', 
    ],
    incs = [
    ],
    deps = [
        ':idl',
        '#pthread',
        '//src/server/:pebble_server',
    ],
)
cc_binary(
    name = 'server',
    srcs = [
        'server.cpp', 
    ],
    incs = [
    ],
    deps = [
        ':idl',
        '#pthread',
        '//src/server/:pebble_server',
    ],
)

