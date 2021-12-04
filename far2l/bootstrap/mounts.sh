#!/usr/bin/env bash

##########################################################
#This script used during FAR's Alt+F1/Alt+F2 menu building
#---------------------------------------------------------
# Mode of operation controlled by $1 and can be one of: 'enum' 'umount'
# Other arguments and output depend on mode operation:
# $1 == 'enum' - enumerates all locations
#   Outputs multiple lines with format:
#     Path1<TAB>Info1
#     Path2<TAB>Info2
#   FAR extracts path, replaces TABs with vertical lines and
#   adds resulting strings to menu
#---------------------------------------------------------
# $1 == 'umount' - unmounts given location
#   $2 - path to unmount
#   $3 - optional 'force' flag
##########################################################

# Optional per-user script
if [ -x ~/.config/far2l/mounts.sh ]; then
. ~/.config/far2l/mounts.sh
fi

##########################################################
# This optional file may contain per-user extra values added to df output,
# its content must be looking like:
#path1<TAB>info1
#path2<TAB>info2
#-<TAB>separator_label
#path3<TAB>info3
FAVORITES=~/.config/far2l/favorites

##########################################################
if [ "$1" == 'umount' ]; then
	if [ "$3" == 'force' ]; then
		sudo umount -f "$2"
	else
		umount "$2"
	fi

##########################################################
else
	#FIXME: paths that contain repeated continuos spaces

	sysname="$(uname)"
	if [ "$sysname" == "Linux" ] || [ "$sysname" == "FreeBSD" ]; then
		DF_ARGS='-T'
		DF_AVAIL=5
		DF_TOTAL=3
		DF_PATH=2
		DF_DIVBY=1
	else
		DF_ARGS='-t'
		DF_AVAIL=4
		DF_TOTAL=2
		DF_PATH=1
		DF_DIVBY=2
	fi

	dfcmd='df '$DF_ARGS' | awk "-F " '\''{
		n=NF;
		while (n>5 && ! ($n ~ "/")) n--;
		for (;n<NF;n++) printf "%s ", $n;
		B=1
		avail=($'$DF_AVAIL'+0.0) / '$DF_DIVBY';
		total=($'$DF_TOTAL'+0.0) / '$DF_DIVBY';
		units="K";
		if (total > 1024*1024*1024) { units="T" ; total/= 1024*1024*1024; avail/= 1024*1024*1024;}
		else if (total > 1024*1024) { units="G" ; total/= 1024*1024; avail/= 1024*1024;}
		else if (total > 1024) { units="M" ; total/= 1024; avail/= 1024;}
		printf "%s\t%.1f/%.1f%s %8s", $n, avail, total, units, $'$DF_PATH';
		print ""
	}'\'

	while IFS=$'\n' read -r line; do
		if [[ "$line" == "/"* ]] \
		  && [[ "$line" != /run/* ]] && [[ "$line" != /snap/* ]] \
		  && [[ "$line" != /sys/* ]] && [[ "$line" != /dev/* ]] ; then
			echo "$line"
		fi
	done < <(eval "$dfcmd")

	if [ -s "$FAVORITES" ]; then
		while IFS=$'\n' read -r line; do
			if [[ $line != "#"* ]] && [[ $line != "" ]]; then
				echo "$line"
			fi
		done < "$FAVORITES"
	fi
fi
