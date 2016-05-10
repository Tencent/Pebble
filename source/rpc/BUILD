
cc_library(
    name = 'pebble_rpc',
    srcs = [
               'rpc.cpp'
    ],
    incs = [
    ],
    deps = [
        '//source/rpc/common:pebble_rpc_common',
        '//source/rpc/transport:pebble_rpc_transport',
        '//source/rpc/protocol:pebble_rpc_protocol',
        '//source/message:pebble_message',
        '//source/common:pebble_common',
        '//source/service:pebble_service_manage',
	'//source/broadcast:pebble_broadcast',
    ],
    link_all_symbols=True
)

gen_rule(
    name = 'ar_rpc_lib',
    srcs = [
        'common/libpebble_rpc_common.a',
        'protocol/libpebble_rpc_protocol.a',
        'transport/libpebble_rpc_transport.a',
        'libpebble_rpc.a',
    ],
    cmd = 'cd $BUILD_DIR/source/rpc;cp ../broadcast/libpebble_broadcast.a .;cp ../../thirdparty/consistent_hash/libconsistent_hash.a .;ar -x libpebble_broadcast.a;ar -x libconsistent_hash.a;ar -x common/libpebble_rpc_common.a;ar -x protocol/libpebble_rpc_protocol.a;ar -x transport/libpebble_rpc_transport.a;ar -x libpebble_rpc.a;ar -rcs libpebble_rpc_s.a *.o;rm *.o;rm libpebble_broadcast.a;rm libconsistent_hash.a',
    deps = [
        '//source/rpc:pebble_rpc'
    ],
    outs = [
        'libpebble_rpc_s.a',
    ],
)
