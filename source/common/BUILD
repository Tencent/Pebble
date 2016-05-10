cc_library(
    name = 'pebble_common',
    srcs = ['base64.cpp', 
            'sha1.cpp',
            'string_utility.cpp',
            'zookeeper_client.cpp',
            'coroutine.cpp',
            'time_utility.cpp',
            'thread.cpp',
            'thread_pool.cpp',
            'condition_variable.cpp',       
            'gcloud_env.cpp',
    ],
    incs = [
        '../../thirdparty/zookeeper/include/'
    ],
    deps = [
        '//thirdparty/zookeeper:zookeeper_st',
        '//source/log:pebble_log',
        '#pthread'
    ]
)

