cc_binary(
    name = 'protobuf_rpc',
    srcs = [
        'src/cpp_generator.cpp',
        'src/cpp_plugin.cpp',
    ],
    incs = [
        '../../../thirdparty/protobuf/include/',
    ],
    deps = [
        '//src/common:pebble_common',
        '//thirdparty/protobuf:protoc',
    ]
)

