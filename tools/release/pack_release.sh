#!/bin/bash

pebble_home=../..

function pack_release_files()
{
	cd $pebble_home/release/x86_64/docs
	zip -m -r ./docs.zip ./*
	cd -
	tar -C $pebble_home -zvcf PEBBLE-$1_X86_64_Release.tar.gz release/ --exclude=.svn
}

release_name=`date +%Y%m%d%H%M%S`
if [ $# -gt 0 ]
then
	release_name=$1_$release_name
fi 

echo -e "\033[31 Please make sure you has copyed the pebble tool for windows [yes/no]"
read answer
if [ "$answer" = "yes" ]
then
pack_release_files $release_name
else
echo "\033[31 Please copy the pebble tool for windows first."
fi
