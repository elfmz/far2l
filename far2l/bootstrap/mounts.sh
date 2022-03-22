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
# optional new parameter at 2nd position can have predefined values
# ':du' / ':mnt' / 'user_info_to_show'
# its content must be looking like:
#path1<TAB>:du<TAB>info1
#path2<TAB>:mnt<TAB>info2
#-<TAB>separator_label
#path3<TAB>info3
#path4<TAB><TAB>info4
#path5<TAB>some_info5<TAB>info5
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

			if ("'$awk_busybox'" == "true") {
				if (wide == 1) {

					avail2 = substr((avail_ident avail), 1, 5);
					total2 = substr((total_ident total), 1, 5);

				} else {

					avail2 = substr((avail_ident avail ".0"), 1, 3);
					total2 = substr((total_ident total ".0"), 1, 3);
				}

				print_format = "%s\t%s%s %s%s %s\t%s\n";
				printf print_format, path, avail2, avail_units, total2, total_units, $'$DF_USE', $'$DF_NAME';

			} else {

				print_format = "%s\t%s%.*f%s %s%.*f%s %3s\t%8s\n";
				printf print_format, path, avail_ident, avail_fraction, avail, avail_units, total_ident, total_fraction, total, total_units, $'$DF_USE', $'$DF_NAME';
			}
		}
	}'

	FV_PATH=1
	FV_INFO=2
	FV_MISC=3
	if [ -s "$FAVORITES" ]; then
		TIMEOUT=1
		FMT_COLUMN='cat'
		if [ "$sysname" = "Linux" ] && [ "$column_bsdmainutils" = "true" ]; then
			FMT_COLUMN='column -t'
		fi
		awk "-F " '{
			if ($0 != "" && substr($0, 1, 1) != "#") {
				if ($'$FV_PATH' == "-") {

					print $0;

				} else {

					misc_path = "";
					misc_info = "";
					misc_desc = "";
					misc_check = "false";
					misc_processed = 0;
					misc_width = 9;
					misc_ident = "";

					"printf " $'$FV_PATH' " " | getline misc_path;
					if (misc_path == "") {
						misc_path = $'$FV_PATH';
					}

					if (($'$FV_MISC' != "") && (misc_path != "")) {
						if ($'$FV_INFO' == ":mnt") {
							"[ -d " misc_path " ] && echo true || echo false" | getline misc_check;
							if (misc_check == "") {
								misc_check = "false";
							}
							if (misc_check == "true") {
								if ("'$sysname'" == "FreeBSD") {
									misc_exec = "mount | grep -e '"'"' on " misc_path " '"'"' | tail -n 1 ";
									misc_exec = misc_exec "| sed -e s:^\\\\\\([^\\\\\\ ]\\\\\\{1,\\\\\\}\\\\\\)\\\\\\(.\\\\\\{1,\\\\\\}\\\\\\):\\\\\\1:g ;";
									"" misc_exec "" | getline misc_info;

								} else {

									misc_exec = "mount | grep -e '"'"' on " misc_path " '"'"' | tail -n 1 ";
									misc_exec = misc_exec "| sed -e s:^\\\\\\([^\\\\\\ ]\\\\\\+\\\\\\)\\\\\\(.\\\\\\+\\\\\\):\\\\\\1:g ;";
									"" misc_exec "" | getline misc_info;
								}
								# printf "%s\n", misc_exec;
								if (misc_info == "") {
									misc_info = "+";
									misc_width = 2;
								}

							} else {

								misc_info = "-";
								misc_width = 2;
								misc_ident = "";
							}
							misc_processed = 1;
						}

						if ($'$FV_INFO' == ":du") {
							"[ -d " misc_path " ] && [ -r " misc_path " ] && echo true || echo false" | getline misc_check;
							if (misc_check == "") {
								misc_check = "false";
							}
							if (misc_check == "true") {
								misc_width = 5;
								misc_ident = "=";
								if (("'$sysname'" == "Linux") && ("'$timeout_coreutils'" == "true")) {
									misc_exec = "( timeout --signal=TERM " '$TIMEOUT' " du -sh " misc_path "/ 2> /dev/null ; ) ";
									misc_exec = misc_exec "| head -n 1 | '"$FMT_COLUMN"' ";
									misc_exec = misc_exec "| sed -e s:^\\\\\\([0-9a-zA-Z\\\\\\.,\\\\\\ ]\\\\\\+\\\\\\)\\\\\\(.\\\\\\+\\\\\\):\\\\\\1:g ;";
									"" misc_exec "" | getline misc_info;

								} else {

									misc_exec = "( sh -c '"'"'( sleep " '$TIMEOUT' " ; kill $$ 2> /dev/null ) & exec du -sh " misc_path "/ 2> /dev/null ;'"'"' ) ";
									misc_exec = misc_exec "| head -n 1 | '"$FMT_COLUMN"' ";
									misc_exec = misc_exec "| sed -e s:^\\\\\\([0-9a-zA-Z\\\\\\.,\\\\\\ ]\\\\\\{1,\\\\\\}\\\\\\)\\\\\\(.\\\\\\{1,\\\\\\}\\\\\\):\\\\\\1:g ;";
									"" misc_exec "" | getline misc_info;
								}
								# printf "%s\n", misc_exec;
								if (misc_info == "") {
									misc_info = "?";
									misc_width = 2;
								}

							} else {

								misc_info = "-";
								misc_width = 2;
								misc_ident = "=";
							}
							misc_processed = 1;
						}

						if (misc_processed == 0) {
							misc_info = $'$FV_INFO';
						}

						misc_desc = $'$FV_MISC';

					} else {

						misc_desc = $'$FV_INFO';
					}

					print_format = "%s\t" misc_ident " %" misc_width "s\t%s\n";
					printf print_format, misc_path, misc_info, misc_desc;
				}
			}
		}' "$FAVORITES"
	fi
fi
