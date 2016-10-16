#!/bin/sh

if command -v git >/dev/null 2>/dev/null ; then
	git log -1 --date=format:'%y/%m/%d' --format=%cd-%h
else
	while :
	do
		if [ -f .git/refs/heads/master ]; then
			cat .git/refs/heads/master
			break
		fi
		BEFORE_CD=`pwd`
		cd ..
		AFTER_CD=`pwd`
		if [ "$BEFORE_CD" = "$AFTER_CD" ]; then
			echo unknown
			break
		fi
	
	done
fi
