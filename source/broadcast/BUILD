
gen_rule(
    name = 'gen_broadcast',
    srcs = [
        'idl/broadcast.pebble',
    ],
    cmd = 'mkdir $BUILD_DIR/source/broadcast/idl -p && $BUILD_DIR/source/rpc/compiler/cpp/src/pebble -r -out $BUILD_DIR/source/broadcast/idl --gen cpp $SRCS',
    deps = [
        '//source/rpc/compiler/cpp/src:pebble'
    ],
    outs = [
        'idl/broadcast.cpp',
        'idl/broadcast_PebbleBroadcastService.cpp',
        'idl/broadcast_PebbleChannelMgrService.cpp',
        'idl/broadcast.h',
        'idl/broadcast_PebbleBroadcastService_if.h',
        'idl/broadcast_PebbleChannelMgrService_if.h',
    ],
)

cc_library(
    name = 'pebble_broadcast',
    srcs = [
        'idl/broadcast.cpp',
        'idl/broadcast_PebbleBroadcastService.cpp',
        'idl/broadcast_PebbleChannelMgrService.cpp',
        'storage/channel_conf_manage.cpp',
        'storage/zk_section_handle.cpp',
        'app_config.cpp',
        'channel.cpp',
        'channel_mgr.cpp',
        'channel_conf_adapter.cpp',
        'isubscriber.cpp',
    ],
    incs = [
        '../../source/'
    ],
    deps = [
        '//source/message:pebble_message',
        ':gen_broadcast'
    ]
)
