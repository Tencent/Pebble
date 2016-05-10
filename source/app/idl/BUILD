
gen_rule(
    name = 'gen_control_service',
    srcs = [
        'control_service.pebble',
    ],
    cmd = '$BUILD_DIR/source/rpc/compiler/cpp/src/pebble -r -out $BUILD_DIR/source/app/idl --gen cpp $SRCS',
    deps = [
        '//source/rpc/compiler/cpp/src:pebble'
    ],
    outs = [
        'control_service.cpp',
        'control_service_ControlCommand.cpp',
    ],
)


