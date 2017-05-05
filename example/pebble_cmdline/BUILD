
cc_binary(
    name = 'server',
    srcs = [
        'server.cpp', 
    ],
    incs = [
        '../../thirdparty/gflags',
    ],
    deps = [
        '#pthread',
        '//src/server/:pebble_server',
        '//thirdparty/gflags:gflags',
    ],
)

