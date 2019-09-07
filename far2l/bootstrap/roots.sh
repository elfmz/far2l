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
#---------------------------------------------------------
#If you want just to add some 'favorites' - don't edit this
#Instead, create ~/.config/far2l/favorites text file with lines:
#
#path1<TAB>label1
#path2<TAB>label2
#-<TAB>label_separator
#path3<TAB>label3
#
#You also may create executable ~/.config/far2l/favorites.sh
#That should produce similar text on output
##########################################################

current=$1
another=$2


eol=$'\n'
tab=$'\t'


#FIXME: pathes that contain repeated continuos spaces

sysname=`uname`
if [ "$sysname" == "Linux" ]; then
	dfout=`df -T | awk "-F " '{n=NF; while (n>5 && ! ($n ~ "/")) n--; for (;n<NF;n++) printf "%s ", $n; print $n "\t" $2 }'`
else
	dfout=`df -t | awk "-F " '{n=NF; while (n>5 && ! ($n ~ "/")) n--; for (;n<NF;n++) printf "%s ", $n; print $n "\t" $1 }'`
fi

dfout+=$eol
if [ -f ~/.config/far2l/favorites ]; then
	dfout+=`grep "^[^#]" ~/.config/far2l/favorites`
	dfout+=$eol
fi
if [ -x ~/.config/far2l/favorites.sh ]; then
	dfout+=`~/.config/far2l/favorites.sh`
	dfout+=$eol
fi

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

#limit maximum length
if [ $max_len_path -gt 48 ]; then
	max_len_path=48
fi

for line in "${lines[@]}" ; do
	path="${line%$tab*}"
	comment="${line##*$tab}"
	if [ "$path" != "-" ]; then
		unexpanded="$path"
		if [ ${#path} -gt $max_len_path ]; then
			#truncate too long path with ... in the middle
			begin_len=$((max_len_path / 4))
			end_len=$((max_len_path - begin_len - 3))
			path="${path:0:$begin_len}...${path:${#path}-$end_len:$end_len}"
		fi
		while [ ${#path} -lt $max_len_path ]; do
			path=" $path"
		done
		while [ ${#comment} -lt $max_len_comment ]; do
			comment=" $comment"
		done
		echo "$unexpanded$tab$path$tab$comment"
	else
		echo "-$tab$comment"
	fi
done
