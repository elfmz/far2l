#!/bin/sh
############################################################
#This script used by FAR to open files by other applications
############################################################
#$1= [exec|dir|other] wher:
#  exec - execute given command in other terminal
#  dir - open given directory witg GUI
#  other - open given file witg GUI

what=$1
shift

if [ "$what" = "exec" ]; then
	if command -v xterm >/dev/null 2>/dev/null ; then
		xterm -e "$@" >/dev/null 2>/dev/null &
		exit 0
	fi
fi

xdg-open "$@"
