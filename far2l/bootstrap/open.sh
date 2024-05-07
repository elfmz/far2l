#!/bin/sh
############################################################
#This script used by FAR to open files by other applications
############################################################
#$1= [exec|dir|other] where:
#  exec - execute given command in other terminal
#  dir - open given directory with GUI
#  other - open given file with GUI
#Other arguments - actual command/file to be executed/opened
############################################################
#For per user customization - create:
#~/.config/far2l/open.sh
############################################################

what=$1
shift

# Define EXEC_TERM here to allow overriding its from ~/.config/far2l/open.sh
EXEC_TERM=xterm
if [ ! -z "$DESKTOP_SESSION" ] && [ -x /etc/alternatives/x-terminal-emulator ] ; then
	EXEC_TERM=/etc/alternatives/x-terminal-emulator
fi

if [ -x ~/.config/far2l/open.sh ]; then
. ~/.config/far2l/open.sh
fi

if [ "$what" = "exec" ] ; then
	if { [ -f "$1" ] && [ -x "$1" ]; } || command -v "$1" >/dev/null 2>/dev/null ; then
		if command -v $EXEC_TERM >/dev/null 2>/dev/null ; then
			$EXEC_TERM -e "$@" >/dev/null 2>/dev/null &
			exit 0
		fi
	fi
fi

if command -v xdg-open >/dev/null 2>&1; then #GNOME
	xdg-open "$@"

elif command -v open >/dev/null 2>&1; then #OSX
	open "$@"
fi
