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

# Return (print) the position of a first character in $1 that's not in $2.
function get_unique_hotkey_pos() {
	from=$1
	used_keys=$2
	for (( i=0; i<${#from}; i++ )); do
		if [[ "${used_keys}" != *"${from:$i:1}"* ]]; then
			echo "$i"
			return
		fi
	done
	echo ""
}

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
		dfcmd='df -T | awk "-F " '\''{n=NF; while (n>5 && ! ($n ~ "/")) n--; for (;n<NF;n++) printf "%s ", $n; print $n "\t" $2 }'\'
	else
		dfcmd='df -t | awk "-F " '\''{n=NF; while (n>5 && ! ($n ~ "/")) n--; for (;n<NF;n++) printf "%s ", $n; print $n "\t" $1 }'\'
	fi

	while IFS=$'\n' read -r line; do
		if [[ "$line" == "/"* ]] \
		  && [[ "$line" != /run/* ]] && [[ "$line" != /snap/* ]] \
		  && [[ "$line" != /sys/* ]] && [[ "$line" != /dev/* ]] && [[ "$line" != "/dev"$'\t'* ]] \
		  && [[ "$line" != /System/* ]] && [[ "$line" != /private/* ]] ; then
			# Absence of quotes is intentional.
			DISK_LABEL="$(basename $line)"
			DISK_LABEL_LC="$(echo "${DISK_LABEL}" | tr '[:upper:]' '[:lower:]')"
			HOTKEY_POS=$(get_unique_hotkey_pos "${DISK_LABEL_LC}" "${ALL_HOTKEYS}")
			if [[ -n "${HOTKEY_POS}" ]]; then
				# https://superuser.com/a/1472573/32412 - "In a bash regex, quoted parts are literal".
				# Alas, bash doesn't have a nongreedy match.
				if [[ $line =~ (.*)"${DISK_LABEL}"(.*) ]]; then
					ALL_HOTKEYS="${ALL_HOTKEYS}${DISK_LABEL_LC:${HOTKEY_POS}:1}"
					HOTKEY_POS=$((HOTKEY_POS+1))
					DISK_LABEL_HOTKEYED="$(echo "${DISK_LABEL}" | sed "s/./\&&/${HOTKEY_POS}")"
					line="${BASH_REMATCH[1]}${DISK_LABEL_HOTKEYED}${BASH_REMATCH[2]}"
				fi
			fi
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
