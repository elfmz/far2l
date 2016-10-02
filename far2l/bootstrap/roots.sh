#!/bin/bash

##########################################################
#This script used during FAR's Alt+F1/Alt+F2 menu building
#---------------------------------------------------------
#Input:
#$1 - path selected in current panel
#$2 - path selected in another panel
#---------------------------------------------------------
#Outputs multiple lines with format:
#Root1\tFormatted\tInfo1\n
#Root2\tFormatted\tInfo2\n
#FAR extracts root, replaces \t with vertical lines and 
#adds resulting string to menu
##########################################################

current=$1
another=$2


eol=$'\n'
tab=$'\t'


dfout=`df -T | awk "-F " '{ print $NF "\t" $2 }'`
dfout+=$eol
allow=0

root=""
profile=""
lines=()

while :
do
	remain="${dfout#*$eol}"
	line="${dfout:0:$((${#dfout} - ${#eol} - ${#remain}))}"
	dfout="$remain"

	if [ ! -z "$line" ]; then 
		if [ $allow -eq 1 ]; then #skip columns titles
			path="${line%$tab*}"
			if [ "$path" == "$another" ]; then
				another=""
			fi
			if [ "$path" == "/" ]; then
				root=$line
			elif [ "$path" == ~ ]; then
				profile=$line
			else
				lines=( "${lines[@]}" "$line" )
			fi
		else
			allow=1
		fi
	fi

	if [ -z "$dfout" ]; then 
		break
	fi
done

if [ "$profile" == "" ]; then
	profile=~
	profile="$profile$tab&~"
fi
if [ "$root" == "" ]; then
	root="/$tab&/"
else
	root="$root &/"
fi

lines=("$profile" "$root" "${lines[@]}")

if [ ! -z "$another" ]; then
	lines=("$another$tab&-" "${lines[@]}")
fi

max_len_path=4
max_len_comment=4
for line in "${lines[@]}" ; do
	path="${line%$tab*}"
	comment="${line##*$tab}"
	if [ ${#path} -gt $max_len_path ]; then
		max_len_path=${#path}
	fi
	if [ ${#comment} -gt $max_len_comment ]; then
		max_len_comment=${#comment}
	fi
done
#echo "max_len_path=$max_len_path max_len_comment=$max_len_comment"

for line in "${lines[@]}" ; do
	path="${line%$tab*}"
	comment="${line##*$tab}"
	unexpanded="$path"
	while [ ${#path} -lt $max_len_path ]; do
		path=" $path"
	done
	while [ ${#comment} -lt $max_len_comment ]; do
		comment=" $comment"
	done
	echo "$unexpanded$tab$path$tab$comment"
done
