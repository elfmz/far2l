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
	# safely source this script from user config dir if it copied there without changes
	export far2l_mounts_per_user_script_sourced=$((${far2l_mounts_per_user_script_sourced:-0}+0))
	if [ ${far2l_mounts_per_user_script_sourced} -lt 1 ]; then
		export far2l_mounts_per_user_script_sourced=$((${far2l_mounts_per_user_script_sourced}+1))
		if [ ${far2l_mounts_per_user_script_sourced} -eq 1 ]; then
			. ~/.config/far2l/mounts.sh
		fi
	fi
fi

##########################################################
# This optional file may contain per-user extra values added to "df" or "mount" output,
# absolute path or ~/dir1 or ${env_var1}/dir2 or $(program args)/dir3 can be used in path field
# anything that can be handled by shell in printf "example_path"
# its content must be looking like:
#
#path1<TAB>info1
#path2<TAB>info2
#-<TAB>separator_label
#path3<TAB>info3
#
FAVORITES=~/.config/far2l/favorites

##########################################################
script_debug_enabled="false"
script_debug_log="/dev/null"

if ! [ -z "${FAR2L_DEBUG_MOUNTS_LOG}" ]; then
	script_debug_enabled="true"

	# log file must exist already and be readable and writable
	if [ -r "${FAR2L_DEBUG_MOUNTS_LOG}" -a -w "${FAR2L_DEBUG_MOUNTS_LOG}" -a ! -x "${FAR2L_DEBUG_MOUNTS_LOG}" ]; then
		script_debug_log="${FAR2L_DEBUG_MOUNTS_LOG}"
	else
		script_debug_log="/tmp/far2l_mount_sh_debug.log"
	fi
fi

if [ ".${script_debug_enabled}" = ".true" ]; then
	# start with empty log file
	printf "%s" "" > "${script_debug_log}"
fi

##########################################################
if [ ."$1" = '.umount' -a -z "$2" ]; then
	if [ ."$3" = '.force' ]; then
		sudo umount -f "$2"
	else
		umount "$2"
	fi

##########################################################
else
	VAR_SCRIPT_AWK_DEBUG=''

	if [ ".${script_debug_enabled}" = ".true" ]; then
		VAR_SCRIPT_AWK_DEBUG='-v debug=1'
	fi

	RGX_SLASH='\'
	RGX_SLASH_DOUBLE='\\'
	RGX_TAB='\t'
	RGX_SPC=' '
	RGX_TAB_OR_SPC='['${RGX_TAB}''${RGX_SPC}']'
	RGX_NEWLINE='\n'

	sysname="$(uname||printf 'unknown')"
	timeout_coreutils="false"
	column_bsdmainutils="false"
	awk_busybox="false"

	if [ ".${sysname}" = ".Linux" ]; then
		timeout_coreutils="$(sh -c 'timeout --version > /dev/null 2>&1 && printf true || printf false')"
		awk_busybox="$([ .$(awk 2>&1 | head -n 1 | cut -d' ' -f1) = .BusyBox ] > /dev/null 2>&1 && printf true || printf false)"
	fi

	DF='( df -P )'
	USE_MOUNT_CMD='false'

	MNT_CMN_FILTER='cat'
	MNT_USR_FILTER='cat'
	DF_CMN_FILTER='cat'
	DF_USR_FILTER='cat'

	AWK_ARG_SRCF=''

##########################################################
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
			MNT_FILTER='grep -v -e " /proc\(/[\/a-z_]\+\)* " -e " /sys\(/[a-z_,]\+\)* " -e " /dev/\(pts\|hugepages\|mqueue\) " -e " /run/user/\([0-1]\+\)/doc " -e "/home/\([a-z0-9\._]\+\)/.cache/doc" -e " /tmp/\.\([a-z0-9\._]\+\)_\([a-zA-Z0-9\._]\+\) "'
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

##########################################################
	FV_PATH=1
	FV_INFO=2

	if [ -s "${FAVORITES}" ]; then

		# start script preparation ( parse "favorites" file contents )

		SCRIPT_AWK_FAVORITES='
BEGIN {
	if ("" debug "" == "1" || "" debug "" == "true") {
		debug_enabled = "true";
	}
	else {
		debug_enabled = "false";
	}

	if (debug_enabled == "true") {
		debug_log = "/tmp/script_1_favorites_debug.log";
		printf "" > debug_log;
		close(debug_log);

		output_log = "/tmp/script_1_favorites_output.log";
		printf "" > output_log;
		close(output_log);
	}

	# absolute path or ~/dir1 or ${env_var1}/dir2 or $(program args)/dir3 can be used in path field
	# anything that can be handled by shell in printf "example_path"
	# option_fav_nonabs_path should have value 1 to handle this
	# and value 0 to work with absolute path only
	option_fav_nonabs_path = 0;

	FS = "\\t";

	tab = "'${RGX_TAB}'";
	space = "'${RGX_SPC}'";
	tab_or_space = "'${RGX_TAB_OR_SPC}'";
	newline = "'${RGX_NEWLINE}'";
	single_quote = "\047";
}
debug_enabled == "true" {
	printf "====================" >> debug_log;
	print "" >> debug_log;
	printf "NR=%d, FNR=%d, NF=%d, $0=_%s_", NR, FNR, NF, $0 >> debug_log;
	print "" >> debug_log;

	for (n = 1; n <= NF; n++) {
		printf "%s","_ || $" n " = _" $n >> debug_log;
	}
	print "_ ||" >> debug_log;

}
/.*/ {
	if ($0 != "" && substr($0, 1, 1) != "#") {

		fav_path = $'${FV_PATH}';
		misc_desc = $'${FV_INFO}';

		if (fav_path == "-") {

			print $0;

		}
		else
		{
			misc_path = "";
			misc_info = "";
			misc_width = 9;
			misc_ident = "";

			if (option_fav_nonabs_path == 1) {

				snqt_fav_path = fav_path;
				gsub(/'"'"'/, "'"\'"'" "\"" "'"\'"'" "\"" "'"\'"'", snqt_fav_path);
				snqt_fav_path = sprintf(single_quote "%s" single_quote, snqt_fav_path);

				# example usage of quoted path with single quotes inside
				## misc_exec = "( printf " snqt_fav_path " 2> /dev/null ) | head -n 1 ";

				misc_exec = "( printf " fav_path " 2> /dev/null ) | head -n 1 ";
				"" misc_exec "" | getline misc_path;
				close(misc_exec);

				if (debug_enabled == "true") {
					print "[info]  misc_exec = _" misc_exec "_" >> debug_log;
				}
			}

			if (misc_path == "") {
				misc_path = fav_path;
			}

			if (misc_desc == "") {
				misc_desc = fav_path;
			}

			print_format = "%s\t" misc_ident " %" misc_width "s\t%s\n";

			printf print_format, misc_path, misc_info, misc_desc;

			if (debug_enabled == "true") {
				printf print_format, misc_path, misc_info, misc_desc >> output_log;
			}
		}
	}
}
END {
	if (debug_enabled == "true") {
		close(debug_log);
		close(output_log);
	}
}'

		# finish script preparation ( parse "favorites" file contents )

		if [ ".${script_debug_enabled}" = ".true" ]; then
			printf "%s\\n" "${SCRIPT_AWK_FAVORITES}" > "/tmp/script_1_favorites.awk"
			## echo "${SCRIPT_AWK_FAVORITES}" > "/tmp/script_2_favorites.awk"
		fi

		cat "${FAVORITES}" \
			| awk -F '\t' ${VAR_SCRIPT_AWK_DEBUG} ${AWK_ARG_SRCF} ' '"${SCRIPT_AWK_FAVORITES}"' ' \
			| cat
	fi
fi

##########################################################
# safely source this script from user config dir if it copied there without changes
exit 0
