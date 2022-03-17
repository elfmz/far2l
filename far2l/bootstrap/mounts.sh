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
# optional 3rd parameter can have predefined values 'du' / 'mnt'
# its content must be looking like:
#path1<TAB>info1<TAB>du
#path2<TAB>info2<TAB>mnt
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
	if [ "$sysname" = "Linux" ] || [ "$sysname" = "FreeBSD" ]; then
		DF_ARGS='-T'
		DF_AVAIL=5
		DF_TOTAL=3
		DF_NAME=2
		DF_DIVBY=1
		DF_USE=6
	else
		DF_ARGS='-t'
		DF_AVAIL=4
		DF_TOTAL=2
		DF_NAME=1
		DF_DIVBY=2
		DF_USE=5
	fi

	#FIXME: paths that contain repeated continuos spaces
	df $DF_ARGS 2>/dev/null | awk "-F " '{
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

			wide = 0;

			if (wide == 1) {

				avail_ident = "";
				total_ident = "";

				avail_fraction = avail < 100 ? ( avail < 10 ? 3 : 2 ) : 1;
				total_fraction = total < 100 ? ( total < 10 ? 3 : 2 ) : 1;

			} else {

				avail_ident = avail < 100 ? ( avail < 10 ? "" : "_" ) : "";
				total_ident = total < 100 ? ( total < 10 ? "" : " " ) : "";

				avail_fraction = avail < 10 ? 1 : 0;
				total_fraction = total < 10 ? 1 : 0;
			}

			printf "%s\t%s%.*f%s %s%.*f%s %3s\t%8s\n", path, avail_ident, avail_fraction, avail, avail_units, total_ident, total_fraction, total, total_units, $'$DF_USE', $'$DF_NAME';
		}
	}'

	FV_PATH=1
	FV_INFO=2
	FV_MISC=3
	if [ -s "$FAVORITES" ]; then
		awk "-F " '{
			if ($0 != "" && substr($0, 1, 1) != "#") {
				if ($'$FV_PATH' == "-") {

					print $0;

				} else {

					misc_info = "";
					misc_width = 9;
					misc_ident = "";

					if ($'$FV_MISC' == "mnt") {
						"mount | grep " $'$FV_PATH' " | tail -n 1 | cut -d'"'"' '"'"' -f1" | getline misc_info;
					}

					if ($'$FV_MISC' == "du") {
						if ("'$sysname'" == "Linux") {
							"( timeout --signal=TERM 1 du -sh " $'$FV_PATH' "/ 2> /dev/null ) | head -n 1 | column -t | cut -d'"'"' '"'"' -f1" | getline misc_info;
							misc_width = 5;
							misc_ident = "=";
						}
					}

					printf "%s\t"misc_ident"%"misc_width"s\t%s\n", $'$FV_PATH', misc_info, $'$FV_INFO';
				}
			}
		}' "$FAVORITES"
	fi
fi
