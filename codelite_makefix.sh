#!/bin/bash
WKSPWD=`pwd | sed s,/,\\\\\\\\\\/,g`
for f in * ; do
	if [[ -f $f/$f.mk ]]; then
		cd $f
		pwd
		PRJPWD=`pwd | sed s,/,\\\\\\\\\\/,g`
		cat $f.mk |sed -e "s/${PRJPWD}/\./g">$f.mkx
		cat $f.mkx |sed -e "s/${WKSPWD}/\.\./g">$f.mk
		rm $f.mkx
		cd ..
	fi
done
