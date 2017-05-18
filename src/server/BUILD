gen_rule(
    name = 'gen_control',
    srcs = [
        'control.pebble',
    ],
    cmd = '$BUILD_DIR/tools/compiler/dr/pebble -r -out $BUILD_DIR/src/server/ --gen cpp $SRCS',
    deps = [
        '//tools/compiler/dr:pebble',
    ],
    outs = [
        'control.cpp',
        'control__PebbleControl.cpp',
        'control.h',
        'control__PebbleControl.h',
        'control__PebbleControl.inh',
    ],
)

cc_library(
    name = 'pebble_server',
    srcs = [
        'pebble_server.cpp', 
        'pebble_cmdline.cpp', 
        'control.cpp',
        'control__PebbleControl.cpp',
    ],
    incs = [
    ],
    extra_cppflags = [
        '--std=c++0x',
    ],
    deps = [
        ':gen_control',
        '//src/framework/:pebble_framework',
    ],
)

cc_binary(
    name = 'pebble_control_client',
    srcs = [
        'control_client.cpp',
        'control.cpp',
        'control__PebbleControl.cpp',
    ],
    incs = [
    ],
    deps = [
        '#pthread',
        '#readline',
        ':gen_control',
        '//src/client/:pebble_client',
        '//src/extension/tbuspp:pebble_tbuspp',
        '//thirdparty/gflags:gflags',
    ],
)

cc_binary(
    name = 'pebble_error_parser',
    srcs = [
        'error_parser.cpp',
    ],
    incs = [
    ],
    deps = [
        '//src/framework/:pebble_framework',
    ],
)

gen_rule(
    name = 'cp_ctrl_client_2_tools',
    srcs = [
        'pebble_control_client',
    ],
    cmd = 'cp $BUILD_DIR/src/server/pebble_control_client  $BUILD_DIR/../tools;strip -s $BUILD_DIR/../tools/pebble_control_client',
    deps = [
        ':pebble_control_client'
    ],
    outs = [
        'ctrl_cmd_tool',
    ],
)

gen_rule(
    name = 'cp_error_parser_2_tools',
    srcs = [
        'pebble_error_parser',
    ],
    cmd = 'cp $BUILD_DIR/src/server/pebble_error_parser $BUILD_DIR/../tools;strip -s $BUILD_DIR/../tools/pebble_error_parser',
    deps = [
        ':pebble_error_parser'
    ],
    outs = [
        'error_parser_tool',
    ],
)

