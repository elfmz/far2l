#!/bin/sh
##########################################################
#This script used by FAR to move files to Trash
##########################################################
#For per user customization - create:
#~/.config/far2l/trash.sh
##########################################################

set -e

if [ -x ~/.config/far2l/trash.sh ]; then
. ~/.config/far2l/trash.sh
fi

if command -v gvfs-trash >/dev/null 2>&1; then
	gvfs-trash -f -- "$1"
elif command -v gio >/dev/null 2>&1; then
	gio trash -f -- "$1"
fi
