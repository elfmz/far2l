#!/usr/bin/env bash

##########################################################
#This script used during FAR's Alt+F1/Alt+F2 menu building
#---------------------------------------------------------
# Mode of operation controlled by $1 and can be one of: 'enum' 'favorite' 'favorite-remove' 'umount'
# Other arguments and output depend on mode operation:
# $1 == 'enum' - enumerates all locations
#   $2 - path selected in current panel
#   $3 - path selected in another panel
#   Outputs multiple lines with format:
#     Path1\tFormatted\tInfo1\n
#     Path2\tFormatted\tInfo2\n
#   FAR extracts path, replaces \t with vertical lines and 
#   adds resulting string to menu
#---------------------------------------------------------
# $1 == 'umount' - unmounts mounted location
#   $2 - path to unmount
#   $3 - optional 'force' flag
##########################################################

##########################################################
# This optional file may contain per-user extra values added to df output,
# its content must be looking like:
#path1<TAB>label1
#path2<TAB>label2
#-<TAB>separator_label
#path3<TAB>label3
USEREXTRA=~/.config/far2l/locations

##########################################################
# Limit on length of visual representation of path,
# longer pathes will be shortened by ... in the middle
MAXFORMATTEDPATH=48


##########################################################
if [ "$1" == 'umount' ]; then
	if [ "$3" == 'force' ]; then
		sudo umount -f "$2"
	else
		umount "$2"
	fi

##########################################################
else
	current=$2
	another=$3

	eol=$'\n'
	tab=$'\t'

	#FIXME: pathes that contain repeated continuos spaces

	sysname=`uname`
	if [ "$sysname" == "Linux" ] || [ "$sysname" == "FreeBSD" ]; then
		dfcmd='df -T | awk "-F " '\''{n=NF; while (n>5 && ! ($n ~ "/")) n--; for (;n<NF;n++) printf "%s ", $n; print $n "\t" $2 }'\'
	else
		dfcmd='df -t | awk "-F " '\''{n=NF; while (n>5 && ! ($n ~ "/")) n--; for (;n<NF;n++) printf "%s ", $n; print $n "\t" $1 }'\'
	fi

	dfout=()
	if [ "$another" != "" ]; then
		dfout+=("$another" '&-')
	fi
	dfout+=(~ '&~')
	rootfs=
	while IFS=$'\n' read -r line 
	do
		if [[ "$line" == "/"* ]] \
		  && [[ "$line" != /run/* ]] && [[ "$line" != /snap/* ]] \
		  && [[ "$line" != /sys/* ]] && [[ "$line" != /dev/* ]] ; then
			IFS=$'\t'
			dfout+=($line)
			if [ "${dfout[${#dfout[@]} - 2]}" == "/" ]; then
				dfout[${#dfout[@]} - 1]="${dfout[${#dfout[@]} - 1]} &/"
				rootfs=1
			fi
			IFS=$'\n'
		fi
	done < <(eval "$dfcmd")

	if [ "$rootfs" == "" ]; then
		dfout+=(/ '&/')
	fi

	if [ -s $USEREXTRA ]; then
		while IFS=$'\n' read -r line
		do
			if [[ $line != "#"* ]] && [[ $line != "" ]]; then
				IFS=$'\t'
				dfout+=($line)
				IFS=$'\n'
			fi
		done < $USEREXTRA
	fi

	max_len_path=4
	max_len_label=4

	for ((i=0; i < ${#dfout[@]}; i+=2)); do
#		echo "$i:" ${dfout[$i]} ${dfout[$i+1]}
		if [ ${#dfout[$i]} -gt $max_len_path ]; then
			max_len_path=${#dfout[$i]}
		fi
		if [ ${#dfout[$i+1]} -gt $max_len_label ]; then
			max_len_label=${#dfout[$i+1]}
		fi
	done

#echo "max_len_path=$max_len_path max_len_label=$max_len_label"
	if [ $max_len_path -gt $MAXFORMATTEDPATH ]; then
		max_len_path=$MAXFORMATTEDPATH
	fi

	for ((i=0; i < ${#dfout[@]}; i+=2)); do
		path=${dfout[$i]}
		label=${dfout[$i+1]}
		if [ "$path" != '-' ]; then
			if [ ${#path} -gt $max_len_path ]; then
				#truncate too long path with ... in the middle
				begin_len=$((max_len_path / 4))
				end_len=$((max_len_path - begin_len - 3))
				path="${path:0:$begin_len}...${path:${#path}-$end_len:$end_len}"
			fi
			while [ ${#path} -lt $max_len_path ]; do
				path=" $path"
			done
			while [ ${#label} -lt $max_len_label ]; do
				label=" $label"
			done
			echo "${dfout[$i]}$tab$path$tab$label"

		else
			echo "-$tab$label"
		fi
	done
fi
