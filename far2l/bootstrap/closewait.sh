#!/bin/sh
#############################################################
# This script used by FAR to wait while specified path in use
# and to optionally delete it after it becomes unused
#############################################################

trap true HUP

action=
if [ "$1" = "--delete" ]; then
	action=$1
	shift
fi
path=$1

if command -v lsof >/dev/null 2>&1 ; then
	# give app 3 seconds to open file
	sleep 3
	if [ -f "$path" ]; then
		echo "Waiting for: $path"
		while lsof "$path" >/dev/null 2>&1 ; do
			sleep 1
		done
	else
		echo "Not a file: $path"
	fi
else
	echo "wait: requires 'lsof' utility to be installed" >&2
	# fallback to 5 seconds sleep
	sleep 5
fi

if [ "$action" = "--delete" ]; then
	echo "Deleting: $path"
	rm -f "$path"
fi
