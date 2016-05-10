cc_library(
    name = 'pebble_service_manage',
    srcs = [
        'service_manage.cpp',
        'address_cache.cpp',
        'service_handler.cpp',
        'address_puller.cpp',
    ],
    incs = [
    ],
    deps = [
        '//source/common/:pebble_common',
        '//source/log:pebble_log',
        '#pthread'
    ],
    link_all_symbols = True
)
