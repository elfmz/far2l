#!/bin/sh
if [ "$2" = 'force' ]; then
	sudo umount -f "$1"
else
	umount "$1"
fi
