gen_rule(
    name = 'gen_calculator',
    srcs = [
        'calculator.pebble',
    ],
    cmd = '$BUILD_DIR/tools/compiler/dr/pebble -r -out $BUILD_DIR/example/pebble_server --gen cpp $SRCS',
    deps = [
        '//tools/compiler/dr:pebble',
    ],
    outs = [
        'calculator.cpp',
        'calculator_Calculator.cpp',
        'calculator.h',
        'calculator_Calculator.h',
        'calculator_Calculator.inh',
    ],
)

cc_library(
    name = 'calculator',
    srcs = [
        'calculator.cpp',
        'calculator_Calculator.cpp',
    ],
    incs = [
    ],
    deps = [
        ':gen_calculator',
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
        ':calculator',
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
        ':calculator',
        '//src/server/:pebble_server',
    ],
)

