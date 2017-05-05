#!/bin/bash

pebble_home=../..

function pack_release_files()
{
	cd $pebble_home/release/x86_64/doc
	zip -m -r ./doc.zip ./*
	cd -
	tar -C $pebble_home -zvcf PEBBLE-$1_X86_64_Release.tar.gz release/ --exclude=.svn 
}

release_name=`date +%Y%m%d%H%M%S`
if [ $# -gt 0 ]
then
	release_name=$1_$release_name
fi 

pack_release_files $release_name

