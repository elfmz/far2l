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

if command -v cut  >/dev/null 2>&1 && \
		command -v grep >/dev/null 2>&1 && \
		command -v lsof >/dev/null 2>&1 ; then
#	leading_char="$(echo "$path" | cut -c 1)"
#	if [ $leading_char = "." ]; then
#		path="$(pwd)$(echo "$path" | cut -c 2-)"
#	elif [ $leading_char != "/" ]; then
#		path="$(pwd)/$path"
#	fi
	# give app 3 seconds to open file
	sleep 3
	if [ -f "$path" ]; then
		echo "Waiting for: $path"
		while lsof | grep -q "$path" ; do
			sleep 1
		done
	else
		echo "Not a file: $path"
	fi
else
	echo "wait: requires 'cut' 'grep' and 'lsof' utilities to be installed" >&2
	# fallback to 5 seconds sleep
	sleep 5
fi

if [ "$action" = "--delete" ]; then
	echo "Deleting: $path"
	rm -f "$path"
fi
