gen_rule(
    name = 'gen_miniserver',
    srcs = [
        'miniserver.pebble',
    ],
    cmd = '$BUILD_DIR/source/rpc/compiler/cpp/src/pebble -r -out $BUILD_DIR/tutorial/pebble_coroutine/ --gen cpp $SRCS',
    deps = [
        '//source/rpc/compiler/cpp/src:pebble'
    ],
    outs = [
        'miniserver.cpp',
        'miniserver_BaseService.cpp',
    ],
)

cc_binary(
    name = 'server',
    srcs = [
        'pebble_coroutine_tutorial.cpp',
        'miniserver.cpp',
        'miniserver_BaseService.cpp',
    ],
    incs = [
        '../../source/',
        '../../source/rpc/',
    ],
    deps = [
        '//source/rpc:pebble_rpc',
        '//source/app:pebble_app',
        ':gen_miniserver',
    ],
)

cc_binary(
    name = 'client',
    srcs = [
        'client.cpp',
        'miniserver.cpp',
        'miniserver_BaseService.cpp',
    ],
    incs = [
        '../../source/',
        '../../source/rpc/',
    ],
    deps = [
        '//source/rpc:pebble_rpc',
        ':gen_miniserver',
    ],
)
