gen_rule(
    name = 'gen_helloworld',
    srcs = [
        'helloworld.pebble',
    ],
    cmd = '$BUILD_DIR/source/rpc/compiler/cpp/src/pebble -r -out $BUILD_DIR/tutorial/helloworld --gen cpp $SRCS',
    deps = [
        '//source/rpc/compiler/cpp/src:pebble'
    ],
    outs = [
        'helloworld.cpp',
        'helloworld_Tutorail.cpp',
    ],
)

cc_binary(
    name = 'server',
    srcs = [
           'server.cpp',
           'helloworld.cpp',
           'helloworld_Tutorial.cpp',
    ],
    incs = [
        '../../deps/tsf4g/include/',
        '../../source/app/',
        '../../source/',
    ],
    deps = [
            '//source/app:pebble_app',
            ':gen_helloworld'
    ]
)

