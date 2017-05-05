
gen_rule(
    name = 'gen_calculator_message',
    srcs = [
        'calculator.proto',
    ],
    cmd = '$BUILD_DIR/../thirdparty/protobuf/bin/protoc --cpp_out=$BUILD_DIR/ $SRCS',
    deps = [
    ],
    outs = [
        'calculator.pb.cc',
        'calculator.pb.h',
    ],
)

gen_rule(
    name = 'gen_calculator_rpc',
    srcs = [
        'calculator.proto',
    ],
    cmd = '$BUILD_DIR/../thirdparty/protobuf/bin/protoc --plugin=protoc-gen-rpc=$BUILD_DIR/tools/compiler/pb/protobuf_rpc --rpc_out=$BUILD_DIR/ $SRCS',
    deps = [
        '//tools/compiler/pb:protobuf_rpc',
    ],
    outs = [
        'calculator.rpc.pb.cc',
        'calculator.rpc.pb.h',
        'calculator.rpc.pb.inh',
    ],
)

cc_library(
    name = 'calculator',
    srcs = [
        'calculator.pb.cc',
        'calculator.rpc.pb.cc',
    ],
    incs = [
        '../../thirdparty/protobuf/include/',
    ],
    deps = [
        ':gen_calculator_message',
        ':gen_calculator_rpc',
        '//src/framework/:pebble_framework',
    ]
)

cc_binary(
    name = 'client',
    srcs = [
        'client.cpp', 
    ],
    incs = [
        '../../thirdparty/protobuf/include/',
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
        '../../thirdparty/protobuf/include/',
    ],
    deps = [
        ':calculator',
        '//src/server/:pebble_server',
    ],
)

