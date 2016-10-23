#!/bin/sh
##########################################################
#This script used by FAR to move files to Trash
##########################################################

set -e
gvfs-trash -f "$1"
