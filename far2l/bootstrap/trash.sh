#!/bin/sh
##########################################################
#This script used by FAR's to move files to Trash
##########################################################

set -e
gvfs-trash $1
