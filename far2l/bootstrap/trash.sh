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

gvfs-trash -f "$1"
