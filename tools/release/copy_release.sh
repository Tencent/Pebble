#!/bin/bash

pebble_home=../..
build_sys=x86_64
example_path=$pebble_home/release/$build_sys/example
pebble_lib_path=$pebble_home/release/$build_sys/lib/pebble_module
pebble_lib_s_path=$pebble_home/release/$build_sys/lib/pebble
tools_path=$pebble_home/release/$build_sys/tools

function copy_file_list()
{
    while read src_file dst_path
    do
        mkdir -p $pebble_home/release/$build_sys/$2/$dst_path
        cp -rfv $pebble_home/$1/$src_file $pebble_home/release/$build_sys/$2/$dst_path
        find $pebble_home/release/$build_sys/$2/$dst_path -name BUILD | xargs rm -rf
        find $pebble_home/release/$build_sys/$2/$dst_path -name .svn | xargs rm -rf
    done
}

function copy_library_files()
{
    echo ===== Copy library files =====

    copy_file_list "build64_release/thirdparty" "lib" <<- LIB_LIST
        protobuf/libprotobuf.a                              thirdparty/
        zookeeper/libzookeeper_st.a                         thirdparty/
        gflags-2.0/src/libgflags.a                          thirdparty/
	LIB_LIST

    copy_file_list "build64_release/src" "lib" <<- LIB_LIST
        common/libpebble_common.a                               pebble_module/
        framework/dr/libpebble_dr.a                             pebble_module/
        framework/libpebble_framework.a                         pebble_module/
        extension/zookeeper/libpebble_zookeeper.a               pebble_module/
        server/libpebble_server.a                               pebble_module/
        client/libpebble_client.a                               pebble_module/
	LIB_LIST
}


function copy_include_files()
{
    echo ===== Copy include files =====

    copy_file_list "thirdparty" "include/thirdparty" <<- INC_LIST
        rapidjson/*.h                                       rapidjson/
        rapidjson/error/*.h                                 rapidjson/error/
        rapidjson/internal/*.h                              rapidjson/internal/
        rapidjson/msinttypes/*.h                            rapidjson/msinttypes/
        zookeeper/include/*                                 zookeeper/include/
        gflags-2.0/src/gflags/*.h                           gflags/
        protobuf/include/*                                  ./
	INC_LIST

    copy_file_list "src" "include/pebble" <<- INC_LIST
        client/*.h                                          client/
        common/*.h                                          common/
        extension/*.h                                       extension/
        extension/zookeeper/*.h                             extension/zookeeper/
        framework/*.h                                       framework/
        framework/dr/*.h                                    framework/dr/
        framework/dr/common/*.h                             framework/dr/common/
        framework/dr/protocol/*.h                           framework/dr/protocol/
        framework/dr/protocol/*.tcc                         framework/dr/protocol/
        framework/dr/transport/*.h                          framework/dr/transport/
        server/*.h                                          server/
	INC_LIST
}

function copy_tool_files()
{
    echo ===== Copy tool files =====
    copy_file_list "tools" "tools" <<- TOOL_LIST
        pebble_control_client                           ./
	TOOL_LIST

    copy_file_list "build64_release/tools" "tools" <<- TOOL_LIST
        compiler/dr/pebble                              ./
        compiler/pb/protobuf_rpc                        ./
	TOOL_LIST

    copy_file_list "thirdparty/protobuf/bin/" "tools" <<- TOOL_LIST
        protoc                                          ./
	TOOL_LIST

    chmod a+x $tools_path/pebble
    chmod a+x $tools_path/protobuf_rpc
    strip -s  $tools_path/pebble
    strip -s  $tools_path/protobuf_rpc
}

function copy_example_files()
{
    echo ===== Copy example files =====
    copy_file_list "example" "example" <<- EXAMPLE_LIST
        pebble_server/*                                     pebble_server/
	pebble_cmdline/*                                    pebble_cmdline/
        pebble_ctrl_cmd/*                                   pebble_ctrl_cmd/
        pebble_idl/*                                        pebble_idl/
        protobuf_rpc/*                                      protobuf_rpc/
	EXAMPLE_LIST
}

function copy_document_files()
{
    echo ===== Copy document files =====
    copy_file_list "doc" "doc" <<- EXAMPLE_LIST
        ./*                                                 ./
	EXAMPLE_LIST
}

function combine_pebble_libs()
{
    echo ===== combine pebble libs =====

    echo ar libpebble.a
    ar x $pebble_lib_path/libpebble_common.a
    ar x $pebble_lib_path/libpebble_dr.a
    ar x $pebble_lib_path/libpebble_framework.a
    ar x $pebble_lib_path/libpebble_server.a
    ar x $pebble_lib_path/libpebble_zookeeper.a

    mkdir -p $pebble_lib_s_path
    rm $pebble_lib_s_path/libpebble.a

    ar -rcs $pebble_lib_s_path/libpebble.a *.o

    rm *.o

    echo ar libpebble_client.a
    ar x $pebble_lib_path/libpebble_common.a
    ar x $pebble_lib_path/libpebble_dr.a
    ar x $pebble_lib_path/libpebble_framework.a
    ar x $pebble_lib_path/libpebble_client.a
    ar x $pebble_lib_path/libpebble_zookeeper.a

    rm $pebble_lib_s_path/libpebble_client.a

    ar -rcs $pebble_lib_s_path/libpebble_client.a *.o

    rm *.o

    rm $pebble_lib_path -rf

}

function copy_file_all()
{
    rm $pebble_home/release/$build_sys/* -rf

    copy_library_files
    copy_include_files
    copy_tool_files
    copy_example_files
    copy_document_files
    combine_pebble_libs
}

copy_file_all

