#!/bin/sh
############################################################
#This script used by FAR to open files by other applications
############################################################

what=$1
shift

if [ "$what" = "dir" ]; then
	if command -v thunar >/dev/null 2>/dev/null ; then
		thunar "$@" >/dev/null 2>/dev/null &
		exit 0
	elif command -v caja >/dev/null 2>/dev/null ; then
		caja "$@" >/dev/null 2>/dev/null &
		exit 0
	elif command -v nautilus >/dev/null 2>/dev/null ; then
		nautilus "$@" >/dev/null 2>/dev/null &
		exit 0
	fi
elif [ "$what" = "exec" ]; then
	if command -v xterm >/dev/null 2>/dev/null ; then
		xterm -e "$@" >/dev/null 2>/dev/null &
		exit 0
	fi
fi

xdg-open "$@"
