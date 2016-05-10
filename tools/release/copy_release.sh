#!/bin/bash

pebble_home=../..
build_sys=x86_64
tools_path=$pebble_home/release/$build_sys/tools

function copy_file_list()
{
	while read src_file dst_path 
	do
		mkdir -p $pebble_home/release/$build_sys/$2/$dst_path
		cp -rfv $pebble_home/$1/$src_file  $pebble_home/release/$build_sys/$2/$dst_path
	done
}

function copy_library_files()
{
	echo ===== Copy library files =====

	copy_file_list "build64_release/thirdparty" "lib" <<- LIB_LIST
		inih-master/*.a										inih-master/
		zeromq/*.a											zeromq/
        zookeeper/libzookeeper_st.a				        zookeeper/
	LIB_LIST
	
	copy_file_list "build64_release/source" "lib" <<- LIB_LIST
		app/libpebble_app.a										app/
		common/libpebble_common.a								common/
		message/libpebble_message.a								message/
		rpc/libpebble_rpc_s.a									rpc/
		service/libpebble_service_manage.a						service/
        log/libpebble_log.a                                     log/
	LIB_LIST
}


function copy_include_files()
{
	echo ===== Copy include files =====
	
	copy_file_list "thirdparty" "include/thirdparty" <<- INC_LIST
		zookeeper/include/*                      			zookeeper/include/
		zeromq/include/*                      			    zeromq/include/
		inih-master/*.h                      			    inih-master/include/
		inih-master/cpp/*.h                      			inih-master/include/cpp/
		rapidjson/*.h                      					rapidjson/include/
		rapidjson/internal/*.h             					rapidjson/include/internal/
	INC_LIST
	
	copy_file_list "source" "include/source" <<- INC_LIST
		app/pebble_server.h									app/
        app/pebble_server_fd.h								app/
		app/coroutine_rpc.h									app/
        app/broadcast_mgr.h									app/
        app/subscriber.h									app/
		common/*.h											common/
		message/*.h											message/
		rpc/*.h												rpc/
		rpc/common/*.h										rpc/common/
		rpc/processor/*.h									rpc/processor/
		rpc/protocol/*.h									rpc/protocol/
		rpc/protocol/*.tcc									rpc/protocol/
		rpc/router/*.h										rpc/router/
		rpc/transport/*.h									rpc/transport/
		service/*.h											service/
        broadcast/isubscriber.h								broadcast/
        log/*.h                                             log/
	INC_LIST
}

function copy_tool_files()
{
	echo ===== Copy tool files =====
	copy_file_list "tools" "tools" <<- TOOL_LIST
		zookeeper/*											zookeeper/
		utility/server_prepare								utility/
	TOOL_LIST
	
	copy_file_list "build64_release/source" "tools" <<- TOOL_LIST
		rpc/compiler/cpp/src/pebble							pebble/
	TOOL_LIST
	strip -s  $tools_path/pebble/pebble
	chmod a+x $tools_path/pebble/pebble
	chmod a+x $tools_path/utility/*
}

function copy_tutorial_files()
{
	echo ===== Copy tutorial files =====
	copy_file_list "tutorial" "tutorial" <<- EXAMPLE_LIST
		coroutine/*.*                                       coroutine/
		pebble_coroutine/*.*                                pebble_coroutine/
		pebble_coroutine/cfg/*                              pebble_coroutine/cfg/
		pebble_server/*.*                                   pebble_server/
		pebble_server/cfg/*                                 pebble_server/cfg/
		reverse_rpc/*.*                                     reverse_rpc/
		rpc/*.*                                             rpc/
		service/*.*                                         service/
		message/*.*                                         message/
        broadcast/*.*                                       broadcast/
        broadcast/cfg/*                                     broadcast/cfg/
        broadcast_1/*.*                                     broadcast_1/
        broadcast_1/cfg/*                                   broadcast_1/cfg/
	EXAMPLE_LIST
}

function copy_document_files()
{
	echo ===== Copy document files =====
	copy_file_list "doc" "docs" <<- EXAMPLE_LIST
		./*                                       			./
	EXAMPLE_LIST
}

function copy_file_all()
{
	copy_library_files
	copy_include_files
	copy_tool_files
	copy_tutorial_files
	copy_document_files
}

copy_file_all

