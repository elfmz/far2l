#!/bin/sh

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
if [ "$1" = 'umount' ]; then
	if [ "$3" = 'force' ]; then
		sudo umount -f "$2"
	else
		umount "$2"
	fi

##########################################################
else
	sysname="$(uname)"
	timeout_coreutils="false"
	column_bsdmainutils="false"
	awk_busybox="false"
	if [ "$sysname" = "Linux" ]; then
		timeout_coreutils="$(sh -c 'timeout --version > /dev/null 2>&1 && printf true || printf false')"
		column_bsdmainutils="$(sh -c 'column -h > /dev/null 2>&1 && printf true || printf false')"
		awk_busybox="$([ .$(awk 2>&1 | head -n 1 | cut -d' ' -f1) = .BusyBox ] > /dev/null 2>&1 && printf true || printf false)"
	fi
	if [ "$sysname" = "Linux" ] || [ "$sysname" = "FreeBSD" ]; then
		DF_ARGS='-T'
		DF_AVAIL=5
		DF_TOTAL=3
		DF_NAME=2
		DF_DIVBY=1
		DF_USE=6
		MNT_FS=1
		MNT_PATH=3
		FT_IDX=1
		FT_FS=3
		FT_TYPE=4
		FT_PATH=5
		TIMEOUT=1
		MNT_TYPE=0
		MNT_FILTER='cat'
		FT_HEADER='Filesystem Type 1K-blocks Used Avail Use%% Mountpoint'
		FMT_COLUMN='cat'
		if [ "$sysname" = "Linux" ]; then
			MNT_TYPE=5
			MNT_FILTER='grep -v -e " /proc\(/[\/a-z_]\+\)* " -e " /sys\(/[a-z_,]\+\)* " -e " /dev/\(pts\|hugepages\|mqueue\) " -e " /run/user/\([0-1]\+\)/doc " -e "/home/\([a-z0-9\._]\+\)/.cache/doc"'
			FT_HEADER='Filesystem Type 1K-blocks Used Available Use%% Mounted_on'
			if [ "$timeout_coreutils" = "true" ]; then
				FMT_COLUMN='column -t'
			fi
		fi
		if [ "$sysname" = "FreeBSD" ]; then
			MNT_FILTER='grep -v -e " \(/[a-z]\+\)*/dev/foo "'
			FT_HEADER='Filesystem Type 1K-blocks Used Avail Capacity Mounted_on'
		fi

		# 1} printf $FT_HEADER and mount output
		# 2) grep - use $MNT_FILTER
		# 3) awk - output almost like df
		# 4) sed - prepare for uniq to eliminate duplicated block device names
		# 5) awk - uniq by $FT_IDX and use timeout controlled df output in subshell
		# 6) column - format final output
		if [ "$sysname" = "Linux" ] && [ "$timeout_coreutils" = "true" ]; then
			DF='( printf "'$FT_HEADER'" ; printf "\n" ; mount \
				| '$MNT_FILTER' \
				| awk '"'"'{ printf "%03.0f:",NR ; printf $'$MNT_FS' " $ " ; print $'$MNT_FS' " " $'$MNT_TYPE' " " $'$MNT_PATH' }'"'"' \
				| sed -e "s_^\([0-9]\+\):\(\/dev\/[^\ ]\+\)_000:~\\2_" \
				| awk '"'"' { if ( prev != $'$FT_IDX' ) { var = "" ; "( timeout --signal=TERM '$TIMEOUT' df '$DF_ARGS' " $'$FT_PATH' " 2> /dev/null ) | head -n 2 | tail -n 1" | getline var ; if ( var != "" ) { print var } else { print $'$FT_FS' " " $'$FT_TYPE' " [size] [used] [avail] [use] " $'$FT_PATH' } } ; prev = $'$FT_IDX' } '"'"' ) \
				| '$FMT_COLUMN''
		else
			DF='( printf "'$FT_HEADER'" ; printf "\n" ; mount \
				| '$MNT_FILTER' \
				| awk '"'"' { printf "%03.0f:",NR ; printf $'$MNT_FS' " $ " ; print $'$MNT_FS' " [type] " $'$MNT_PATH' } '"'"' \
				| sed -e "s_^\([0-9]\+\):\(\/dev\/[^\ ]\+\)_000:~\\2_" \
				| awk '"'"' { if ( prev != $'$FT_IDX' ) { var = "" ; "( ( sleep '$TIMEOUT' ; kill $$ 2> /dev/null ) & exec df '$DF_ARGS' " $'$FT_PATH' " 2> /dev/null ) | head -n 2 | tail -n 1" | getline var ; if ( var != "" ) { print var } else { print $'$FT_FS' " " $'$FT_TYPE' " [size] [used] [avail] [use] " $'$FT_PATH' } } ; prev = $'$FT_IDX' } '"'"' ) \
				| '$FMT_COLUMN''
		fi
	else
		DF_ARGS='-t'
		DF_AVAIL=4
		DF_TOTAL=2
		DF_NAME=1
		DF_DIVBY=2
		DF_USE=5
		DF='df '$DF_ARGS''
	fi

	#FIXME: paths that contain repeated continuos spaces
	sh -c "$DF" 2>/dev/null | awk "-F " '{
		path = $NF;
		for (n = NF - 1; n > '$DF_AVAIL' && substr(path, 1, 1) != "/"; n--) {
			path = $n" "path;
		}

		if ( substr(path, 1, 1) == "/" && substr(path, 1, 5) != "/run/" && substr(path, 1, 5) != "/sys/" \
			&& substr(path, 1, 5) != "/dev/" && substr(path, 1, 6) != "/snap/" \
			&& substr(path, 1, 8) != "/System/" && substr(path, 1, 9) != "/private/" \
			&& path != "/dev" ) {

			avail = ($'$DF_AVAIL'+0.0) / '$DF_DIVBY';
			total = ($'$DF_TOTAL'+0.0) / '$DF_DIVBY';

			total_units="K";
			if (total >= 1024*1024*1024) { total_units="T" ; total/= 1024*1024*1024;}
			else if (total >= 1024*1024) { total_units="G" ; total/= 1024*1024;}
			else if (total >= 1024) { total_units="M" ; total/= 1024;}

			avail_units="K";
			if (avail >= 1024*1024*1024) { avail_units="T" ; avail/= 1024*1024*1024;}
			else if (avail >= 1024*1024) { avail_units="G" ; avail/= 1024*1024;}
			else if (avail >= 1024) { avail_units="M" ; avail/= 1024;}

			avail_ident = avail < 99.9 ? ( avail < 9.9 ? "" : "_" ) : "";
			total_ident = total < 99.9 ? ( total < 9.9 ? "" : " " ) : "";

			avail_fraction = avail < 10 ? 1 : 0;
			total_fraction = total < 10 ? 1 : 0;

			separator_symbol = "/"

			if ("'$awk_busybox'" == "true") {

				avail2 = substr((avail_ident avail ".0"), 1, 3);
				total2 = substr((total_ident total ".0"), 1, 3);
					
				print_format = "%s\t%s%s%s%s%s\t%s\n";
				printf print_format, path, avail2, avail_units, separator_symbol, total2, total_units, $'$DF_NAME';

			} else {

				print_format = "%s\t%s%.*f%s%s%s%.*f%s\t%8s\n";
				printf print_format, path, avail_ident, avail_fraction, avail, avail_units, separator_symbol, total_ident, total_fraction, total, total_units, $'$DF_NAME';
			}
		}
	}'

	if [ -s "$FAVORITES" ]; then
		awk "-F " '{
			if ($0 != "" && substr($0, 1, 1) != "#") {
				print $0;
			}
		}' "$FAVORITES"
	fi
fi
