gen_rule(
    name = 'gen_hello',
    srcs = [
        'hello.pebble',
    ],
    cmd = '$BUILD_DIR/tools/compiler/dr/pebble -r -out $BUILD_DIR/example/hello_world --gen cpp $SRCS',
    deps = [
        '//tools/compiler/dr:pebble',
    ],
    outs = [
        'hello.cpp',
        'hello_HelloWorld.cpp',
        'hello.h',
        'hello_HelloWorld.h',
        'hello_HelloWorld.inh',
    ],
)

cc_library(
    name = 'hello',
    srcs = [
        'hello.cpp',
        'hello_HelloWorld.cpp',
    ],
    incs = [
    ],
    deps = [
        ':gen_hello',
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
        ':hello',
        '//src/client/:pebble_client',
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
        ':hello',
        '//src/server/:pebble_server',
        '//thirdparty/gflags:gflags',
    ],
)

gen_rule(
    name = 'cp_cfg',
    srcs = [
        'cfg',
    ],
    cmd = 'cp $BUILD_DIR/../example/hello_world/cfg -rf $BUILD_DIR/example/hello_world',
    deps = [
        ':server',
    ],
    outs = [
        'pebble.ini',
    ],
)

