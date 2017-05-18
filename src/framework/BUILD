gen_rule(
    name = 'gen_exception',
    srcs = [
        'exception.pebble',
    ],
    cmd = '$BUILD_DIR/tools/compiler/dr/pebble -r -out $BUILD_DIR/src/framework/ --gen cpp $SRCS',
    deps = [
        '//tools/compiler/dr:pebble',
    ],
    outs = [
        'exception.cpp',
        'exception.h',
    ],
)

gen_rule(
    name = 'gen_broadcast',
    srcs = [
        'broadcast.pebble',
    ],
    cmd = '$BUILD_DIR/tools/compiler/dr/pebble -r -out $BUILD_DIR/src/framework/ --gen cpp $SRCS',
    deps = [
        '//tools/compiler/dr:pebble',
    ],
    outs = [
        'broadcast.cpp',
        'broadcast__PebbleBroadcast.cpp',
        'broadcast.h',
        'broadcast__PebbleBroadcast.h',
        'broadcast__PebbleBroadcast.inh',
    ],
)

gen_rule(
    name = 'gen_protobuf_rpc_head',
    srcs = [
        'protobuf_rpc_head.pebble',
    ],
    cmd = '$BUILD_DIR/tools/compiler/dr/pebble -r -out $BUILD_DIR/src/framework/ --gen cpp $SRCS',
    deps = [
        '//tools/compiler/dr:pebble',
    ],
    outs = [
        'protobuf_rpc_head.cpp',
        'protobuf_rpc_head.h',
    ],
)

cc_library(
    name = 'pebble_framework',
    srcs = [
        'broadcast.cpp',
        'broadcast__PebbleBroadcast.cpp',
        'broadcast_mgr.cpp',
        'channel_mgr.cpp',
        'event_handler.cpp',
        'exception.cpp',
        'gdata_api.cpp',
        'message.cpp',
        'naming.cpp',
        'net_message.cpp',
        'options.cpp',
        'pebble_rpc.cpp',
        'processor.cpp',
        'protobuf_rpc_head.cpp',
        'raw_message_driver.cpp',
        'register_error.cpp',
        'router.cpp',
        'rpc.cpp',
        'rpc_plugin.cpp',
        'rpc_util.cpp',
        'session.cpp',
        'stat_manager.cpp',
        'stat.cpp',
        'when_all.cpp',
    ],
    incs = [
    ],
    extra_cppflags = [
        '--std=c++0x',
    ],
    deps = [
        ':gen_broadcast',
        ':gen_exception',
        ':gen_protobuf_rpc_head',
        '//src/common/:pebble_common',
        '//src/framework/dr:pebble_dr',
    ],
)
