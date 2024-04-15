#!/bin/bash

set -e

mkdir -p /tmp/far2l-smoke/output
APP="$1"
if [ "$APP" = "" ]; then
	echo 'Please specify path to far2l binary as argument'
	echo 'Note that far2l must be built with -DTESTING=Yes'
	exit 1
fi
if [ ! -f ./far2l-smoke ] || [ ./far2l-smoke -ot ./far2l-smoke.go ]; then
	echo '--->' Prepare
	go get far2l-smoke
	echo PREPARE: downloading modules
	go mod download
	echo PREPARE: building
	go build
	echo PREPARE: done
fi
rm -rf workdir
mkdir -p workdir
./far2l-smoke "$APP" workdir tests/*.js
