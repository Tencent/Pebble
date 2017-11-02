
cc_binary(
    name = 'server',
    srcs = [
        'server.cpp', 
    ],
    incs = [
    ],
    deps = [
        '#pthread',
        '//src/server/:pebble_server',
    ],
)

gen_rule(
    name = 'cp_cfg',
    srcs = [
        'cfg',
    ],
    cmd = 'cp $BUILD_DIR/../example/pebble_ctrl_cmd/cfg -rf $BUILD_DIR/example/pebble_ctrl_cmd',
    deps = [
        ':server',
    ],
    outs = [
        'pebble.ini',
    ],
)

