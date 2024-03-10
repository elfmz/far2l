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

if command -v kioclient >/dev/null 2>&1; then
	kioclient move "$1" trash:/ 2>"$2"

elif command -v gio >/dev/null 2>&1; then
	gio trash -f -- "$1" 2>"$2"

elif command -v gvfs-trash >/dev/null 2>&1; then
	gvfs-trash -f -- "$1" 2>"$2"

elif command -v osascript >/dev/null 2>&1; then
	osascript -e "tell application \"Finder\" to delete POSIX file \"$1\"" 2>"$2"

else
	echo 'No command-line trash tool available' >"$2"
	exit 1
fi
