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
#     M^Path2\tFormatted\tInfo2\n
#     F^Path3\tFormatted\tInfo3\n
#   FAR extracts path, replaces \t with vertical lines and 
#   adds resulting string to menu. Optional M^ prefix means
#   its a mountpoint and F^ prefix means its a favorite.
#
# $1 == 'favorite' - adds or changes existing location
#   $2 - path of favorite to add or change
#   $3 - text of favorite to set
#
# $1 == 'favorite-remove' - removes existing location
#   $2 - path of favorite to remove
#
# $1 == 'umount' - unmounts mounted location
#   $2 - path to unmount
#   $3 - optional 'force' flag
#---------------------------------------------------------
#  Favorites are stored in ~/.config/far2l/favorites text file:
#
#path1<TAB>text1
#path2<TAB>text2
#-<TAB>separator_label
#path3<TAB>text3
##########################################################

FAVORITES=~/.config/far2l/favorites

##########################################################
if [ "$1" == 'favorite' ]; then
	grep -v "$2"$'\t' $FAVORITES > $FAVORITES.tmp
	echo "$2"$'\t'"$3" >> $FAVORITES.tmp
	mv -f $FAVORITES.tmp $FAVORITES

##########################################################
elif [ "$1" == 'favorite-remove' ]; then
	grep -v "$2"$'\t' $FAVORITES > $FAVORITES.tmp
	mv -f $FAVORITES.tmp $FAVORITES

##########################################################
elif [ "$1" == 'umount' ]; then
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
		dfout+=('' "$another" '&-')
	fi
	dfout+=('' ~ '&~')

	while IFS=$'\n' read -r line 
	do
		if [[ "$line" == "/"* ]] \
		  && [[ "$line" != /run/* ]] && [[ "$line" != /snap/* ]] \
		  && [[ "$line" != /sys/* ]] && [[ "$line" != /dev/* ]] ; then
			IFS=$'\t'
			dfout+=("M^" $line)
			IFS=$'\n'
		fi
	done < <(eval "$dfcmd")

	if [ -s $FAVORITES ]; then
		dfout+=('' '-' 'Favorites')

		while IFS=$'\n' read -r line
		do
			if [[ $line != "#"* ]] && [[ $line != "" ]]; then
				IFS=$'\t'
				dfout+=("F^" $line)
				IFS=$'\n'
			fi
		done < $FAVORITES
	fi

	max_len_path=4
	max_len_text=4

	for ((i=0; i < ${#dfout[@]}; i+=3)); do
#		echo "$i:" ${dfout[$i]} ${dfout[$i+1]} ${dfout[$i+2]}
		if [ ${#dfout[$i+1]} -gt $max_len_path ]; then
			max_len_path=${#dfout[$i+1]}
		fi
		if [ ${#dfout[$i+2]} -gt $max_len_text ]; then
			max_len_text=${#dfout[$i+2]}
		fi
	done

#	echo "max_len_path=$max_len_path max_len_text=$max_len_text"
	if [ $max_len_path -gt 48 ]; then
		max_len_path=48
	fi

	for ((i=0; i < ${#dfout[@]}; i+=3)); do
		path=${dfout[$i+1]}
		text=${dfout[$i+2]}
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
			while [ ${#text} -lt $max_len_text ]; do
				text=" $text"
			done
			echo "${dfout[$i]}${dfout[$i+1]}$tab$path$tab$text"

		else
			echo "-$tab$text"
		fi
	done
fi
