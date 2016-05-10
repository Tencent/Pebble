#!/bin/sh

game_id="11"
game_key="game_key"
app_id="800.1.1"
zk_url="127.0.0.1:1888"
service_name="Demo"
service_url="tcp://127.0.0.1:8000"

function usage()
{
    print "Usage : ./run.sh [game_id] [game_key] [app_id] [zk_url] [service_name] [service_url]"
}

function parse_argv()
{
    if [ "!" != "!$1" ]
    then
        game_id=$1
    fi
    if [ "!" != "!$2" ]
    then
        game_key=$2
    fi
    if [ "!" != "!$3" ]
    then
        app_id=$3
    fi
    if [ "!" !=  "!$4" ]
    then
        zk_url=$4
    fi
    if [ "!" != "!$5" ]
    then
        service_name=$5
    fi
    if [ "!" != "!$6" ]
    then
        service_url=$6
    fi
}

# parse argv
parse_argv "$@"

# prepare env
../../tools/utility/server_prepare add ${game_id} ${game_key} ${app_id} ${zk_url} >> /dev/null

# run
echo -e "\n[Demo run begin]\n"
./service_demo ${game_id} ${game_key} ${app_id} ${zk_url} ${service_name} ${service_url}
echo -e "\n[Demo run end]\n"

# destroy env
../../tools/utility/server_prepare del ${game_id} ${game_key} ${app_id} ${zk_url} >> /dev/null

