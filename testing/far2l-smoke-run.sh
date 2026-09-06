#!/bin/bash

set -e

mkdir -p /tmp/far2l-smoke/output
APP="$1"
if [ "$APP" = "" ]; then
	echo 'Please specify path to far2l binary as argument'
	echo 'Note that far2l must be built with -DTESTING=Yes'
	exit 1
fi
cd src
make
cd ..

echo 'Cleaning up...'
for test in tests/*; do
	rm -rf "$test"/workdir
done

if [ "$2" == "clean" ]; then
	exit 0
fi

echo 'Starting tests:' tests/"$2"*
for test in tests/*; do
	mkdir -p "$test"/workdir
	if [ -e "$test"/initdir ]; then
		cp -r -f "$test"/initdir/* "$test"/workdir/
	fi
done

./far2l-smoke "$APP" tests/"$2"*
