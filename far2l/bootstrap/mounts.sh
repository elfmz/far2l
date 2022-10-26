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
	VAR_SCRIPT_GREP_DEBUG=''

	if [ ".${script_debug_enabled}" = ".true" ]; then
		VAR_SCRIPT_AWK_DEBUG='-v debug=1'
		VAR_SCRIPT_GREP_DEBUG='-x'
	fi

	RGX_SLASH='\'
	RGX_SLASH_DOUBLE='\\'
	RGX_TAB='\t'
	RGX_SPC=' '
	RGX_TAB_OR_SPC='['${RGX_TAB}''${RGX_SPC}']'
	RGX_NEWLINE='\n'

	sysname="$(uname||printf 'unknown')"
	timeout_coreutils="false"
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
	if [ "." = "." ]; then

		# parse "df -T" output with "type" info ( GNU / BSD / other ? )
		# or "df -P -k" output without "type" info ( POSIX / Darwin / other ? ) as input
		# regex for input validation - variant 4
		# ^(((\/([^\/\t ]|[\\])+?)+?)|([a-zA-Z0-9\.,\/\\:;\{\}~\!\@#\$%\^&\-\+_]+?))((([\t ]+)([a-zA-Z][a-zA-Z0-9\.,\/\\:;\{\}~\!\@#\$%\^&\-\+_]+?))?)([\t ]+)([0-9]+)([\t ]+)([0-9]+)([\t ]+)([0-9]+)([\t ]+)(((100|[1-9][0-9]|[0-9])\%)|([-]))([\t ]+)((\/)|(\/([^\/]|[\\])+?)+?)$

		RGX_DF_FS='(((\/([^\/\t ]|[\\])+?)+?)|([a-zA-Z][a-zA-Z0-9\.,\/\\:;\{\}~\!\@#\$%\^&\-\+_]+?))'
		RGX_DF_TYPE='([a-zA-Z][a-zA-Z0-9\.,\/\\:;\{\}~\!\@#\$%\^&\-\+_]+?)'
		RGX_DF_TYPE_COND='((([\t ]+)'${RGX_DF_TYPE}')?)'
		RGX_DF_TOTAL='([0-9]+)'
		RGX_DF_TOTAL_SPC='([\t ]+)'${RGX_DF_TOTAL}''
		RGX_DF_USED='([0-9]+)'
		RGX_DF_USED_SPC='([\t ]+)'${RGX_DF_USED}''
		RGX_DF_AVAIL='([0-9]+)'
		RGX_DF_AVAIL_SPC='([\t ]+)'${RGX_DF_AVAIL}''
		RGX_DF_USE_PRC='(((100|[1-9][0-9]|[0-9])\%)|([-]))'
		RGX_DF_USE_PRC_SPC='([\t ]+)'${RGX_DF_USE_PRC}''
		RGX_DF_PATH='((\/)|(\/([^\/]|[\\])+?)+?)'
		RGX_DF_PATH_SPC='([\t ]+)'${RGX_DF_PATH}''

		RGX_DF_ALL_COND='^'${RGX_DF_FS}''${RGX_DF_TYPE_COND}''${RGX_DF_TOTAL_SPC}''${RGX_DF_USED_SPC}''${RGX_DF_AVAIL_SPC}''${RGX_DF_USE_PRC_SPC}''${RGX_DF_PATH_SPC}'$'

		RC_RGX_DF_FS_PRE='^'
		RC_RGX_DF_FS_POST=''${RGX_DF_TYPE_COND}''${RGX_DF_TOTAL_SPC}''${RGX_DF_USED_SPC}''${RGX_DF_AVAIL_SPC}''${RGX_DF_USE_PRC_SPC}''${RGX_DF_PATH_SPC}'$'

		RC_RGX_DF_TYPE_PRE='^'${RGX_DF_FS}'([\t ]+)'
		RC_RGX_DF_TYPE_GNU_POST=''${RGX_DF_TOTAL_SPC}''${RGX_DF_USED_SPC}''${RGX_DF_AVAIL_SPC}''${RGX_DF_USE_PRC_SPC}''${RGX_DF_PATH_SPC}'$'
		RC_RGX_DF_TYPE_POSIX_POST=''${RGX_DF_TOTAL}''${RGX_DF_USED_SPC}''${RGX_DF_AVAIL_SPC}''${RGX_DF_USE_PRC_SPC}''${RGX_DF_PATH_SPC}'$'

		RC_RGX_DF_TOTAL_GNU_PRE='^'${RGX_DF_FS}''${RGX_DF_TYPE_COND}'([\t ]+)'
		# RC_RGX_DF_TOTAL_POSIX_PRE='^'${RGX_DF_FS}'([\t ]+)'
		RC_RGX_DF_TOTAL_POST=''${RGX_DF_USED_SPC}''${RGX_DF_AVAIL_SPC}''${RGX_DF_USE_PRC_SPC}''${RGX_DF_PATH_SPC}'$'

		RC_RGX_DF_USED_GNU_PRE='^'${RGX_DF_FS}''${RGX_DF_TYPE_COND}''${RGX_DF_TOTAL_SPC}'([\t ]+)'
		# RC_RGX_DF_USED_POSIX_PRE='^'${RGX_DF_FS}''${RGX_DF_TOTAL_SPC}'([\t ]+)'
		RC_RGX_DF_USED_POST=''${RGX_DF_AVAIL_SPC}''${RGX_DF_USE_PRC_SPC}''${RGX_DF_PATH_SPC}'$'

		RC_RGX_DF_AVAIL_GNU_PRE='^'${RGX_DF_FS}''${RGX_DF_TYPE_COND}''${RGX_DF_TOTAL_SPC}''${RGX_DF_USED_SPC}'([\t ]+)'
		# RC_RGX_DF_AVAIL_POSIX_PRE='^'${RGX_DF_FS}''${RGX_DF_TOTAL_SPC}''${RGX_DF_USED_SPC}'([\t ]+)'
		RC_RGX_DF_AVAIL_POST=''${RGX_DF_USE_PRC_SPC}''${RGX_DF_PATH_SPC}'$'

		RC_RGX_DF_USE_PRC_GNU_PRE='^'${RGX_DF_FS}''${RGX_DF_TYPE_COND}''${RGX_DF_TOTAL_SPC}''${RGX_DF_USED_SPC}''${RGX_DF_AVAIL_SPC}'([\t ]+)'
		# RC_RGX_DF_USE_PRC_POSIX_PRE='^'${RGX_DF_FS}''${RGX_DF_TOTAL_SPC}''${RGX_DF_USED_SPC}''${RGX_DF_AVAIL_SPC}'([\t ]+)'
		RC_RGX_DF_USE_PRC_POST=''${RGX_DF_PATH_SPC}'$'

		RC_RGX_DF_PATH_GNU_PRE='^'${RGX_DF_FS}''${RGX_DF_TYPE_COND}''${RGX_DF_TOTAL_SPC}''${RGX_DF_USED_SPC}''${RGX_DF_AVAIL_SPC}''${RGX_DF_USE_PRC_SPC}'([\t ]+)'
		# RC_RGX_DF_PATH_POSIX_PRE='^'${RGX_DF_FS}''${RGX_DF_TOTAL_SPC}''${RGX_DF_USED_SPC}''${RGX_DF_AVAIL_SPC}''${RGX_DF_USE_PRC_SPC}'([\t ]+)'
		RC_RGX_DF_PATH_POST='$'

		if [ ".${script_debug_enabled}" = ".true" ]; then
			printf "RGX_DF_FS = %s\\n" "'${RGX_DF_FS}'" >> "${script_debug_log}"
			printf "RGX_DF_TYPE_COND = %s\\n" "'${RGX_DF_TYPE_COND}'" >> "${script_debug_log}"
			printf "RGX_DF_TOTAL = %s\\n" "'${RGX_DF_TOTAL}'" >> "${script_debug_log}"
			printf "RGX_DF_USED = %s\\n" "'${RGX_DF_USED}'" >> "${script_debug_log}"
			printf "RGX_DF_AVAIL = %s\\n" "'${RGX_DF_AVAIL}'" >> "${script_debug_log}"
			printf "RGX_DF_USE_PRC = %s\\n" "'${RGX_DF_USE_PRC}'" >> "${script_debug_log}"
			printf "RGX_DF_PATH = %s\\n" "'${RGX_DF_PATH}'" >> "${script_debug_log}"
			printf "RGX_DF_ALL_COND = %s\\n" "'${RGX_DF_ALL_COND}'" >> "${script_debug_log}"
		fi
	fi

##########################################################
	if [ ".${sysname}" = ".Linux" ] || [ ".${sysname}" = ".FreeBSD" ] || [ ".${sysname}" = ".Darwin" ]; then

		USE_MOUNT_CMD='true'

		DF_ARGS='-T'

		# for 1024-byte blocks (POSIX and GNU ?) total blocks count should be divided by 1
		# for 512-byte blocks (non-POSIX ?) total blocks count should be divided by 2
		DF_DIVBY=1

		TIMEOUT=1

		FT_HEADER='Filesystem Type 1K-blocks Used Avail Use%% Mountpoint'

		if [ ".${sysname}" = ".Linux" ]; then
			AWK_ARG_SRCF='-e'
			DF_ARGS='-P -k -T'
			MNT_CMN_FILTER='grep -v -e " /proc\(/[\/a-z_]\+\)* " -e " /sys\(/[a-z_,]\+\)* " -e " /dev\(/\(pts\|hugepages\|mqueue\)\)\? " -e " /run/user/\([0-1]\+\)/[a-z][a-z0-9\-]* " -e " /home/\([a-z0-9\._]\+\)/.cache/doc " -e " /var/lib/lxcfs " -e " /tmp/\.\([a-z0-9\._]\+\)_\([a-zA-Z0-9\._]\+\) " -e " /run/snapd/ns\(\(/[a-z][a-z0-9\-]\+\.mnt\)\?\) " -e " /snap/\([a-z][a-z0-9\-]\+\)/\([0-9]\+\) " -e " /var/snap\(\(\/\)\|\(\/[^/]\+\?\)\+\?\) " -e " /run/credentials\(\(\/\)\|\(\/[^/]\+\?\)\+\?\) "'
			MNT_USR_FILTER='grep -v -e " /dev\(/\(shm\|foo\|bar\)\)\? " -e " /run\(/\(lock\|rpc_pipefs\|foo\|bar\)\)\? "'
			DF_CMN_FILTER='grep -v -e " /dev\(/\(foo\|bar\)\)\?$" -e " /run\(/\(foo\|bar\)\)\?$"'
			DF_USR_FILTER='grep -v -e " /opt\(/\(shm\|foo\|bar\)\)\?$" -e " /run\(/\(lock\|foo\|bar\)\)\?$" -e " /net$"'
			FT_HEADER='Filesystem Type 1K-blocks Used Available Use%% Mounted_on Mount_options'
		fi

		if [ ".${sysname}" = ".FreeBSD" ]; then
			AWK_ARG_SRCF=''
			DF_ARGS='-P -k'
			MNT_CMN_FILTER='grep -v -e " \(/[a-z]\+\)*/dev/foo "'
			MNT_USR_FILTER='grep -v -e " /opt\(/\(foo\|bar\)\)\? "'
			DF_CMN_FILTER='grep -v -e " /dev\(/\(foo\|bar\)\)\?$"'
			DF_USR_FILTER='grep -v -e " /opt\(/\(foo\|bar\)\)\?$" -e " /net$" -e " /home$"'
			FT_HEADER='Filesystem Type 1K-blocks Used Avail Capacity Mounted_on Mount_options'
		fi

		if [ ".${sysname}" = ".Darwin" ]; then
			DF_ARGS='-P -k'
			AWK_ARG_SRCF=''
			MNT_CMN_FILTER='grep -v -e " /dev " -e " /private/var/\(vm\|folders\)\(\(\/\)\|\(\/[^/]\+\)\+\) " -e " /System/Volumes\(\(\/\)\|\(\/[^/]\+\)\+\) "'
			MNT_USR_FILTER='grep -v -e " /opt\(/\(foo\|bar\)\)\? " -e " /net " -e " /home "'
			DF_CMN_FILTER='grep -v -e " /private/var/\(vm\|folders\)\(\(\/\)\|\(\/[^/]\+\)\+\)$" -e " /System/Volumes\(\(\/\)\|\(\/[^/]\+\)\+\)$"'
			DF_USR_FILTER='grep -v -e " /opt\(/\(foo\|bar\)\)\?$" -e " /net$" -e " /home$"'
			FT_HEADER='Filesystem 1024-blocks Used Available Capacity Mounted_on Mount_options'
		fi

		DF='( df '${DF_ARGS}' )'

		# script logic :
		# 1} printf ${FT_HEADER} and "mount" program output
		# 2) grep - use ${MNT_CMN_FILTER} for common filtering
		# 3) grep - use ${MNT_USR_FILTER} for user filtering
		# 4) awk - prepare mnt_* fields of "mount" output with mnt_options ( and maybe mnt_label ? )
		# 5) awk - prepare prefix variable ( = unique number and mnt_fs ) to eliminate duplicated block device names
		# 6) awk - uniq by prefix variable and run in subshell timeout controlled "df ${DF_ARGS} 'mnt_path'"
		# 7) awk - prepare df_* fields of "df" output with or without df_type

		# parse "mount" output with "type" info ( GNU / other ? )
		# or "mount" output without "type" info ( BSD / POSIX / Darwin / other ? ) as input
		# regex for input validation - variant 4

		# ^(((\/([^\/]|[\\])+?)+?)|([a-zA-Z][a-zA-Z0-9 \.,\/\\:;\{\}~\!\@#\$%\^&\-\+_]+?))([\t ](on)[\t ])((\/)|(\/([^\/]|[\\])+?)+?)((([\t ]type[\t ])([a-zA-Z][a-zA-Z0-9\.,\/\\:;\{\}~\!\@#\$%\^&\-\+_]+?))?)([\t ])([\(]([a-zA-Z][a-zA-Z0-9 \.,\/\\:;\{\}~\!\@#\$%\^&\*\-\+_=]+?)[\)])(([\t ](\[([\[\<\>\]a-zA-Z0-9\t \.,\/\\:;\{\}~\!\@#\$%\^&\*\(\)\-\+_=]+?)\]))?)$

		RGX_MNT_FS='(((\/([^\/]|[\\])+?)+?)|([a-zA-Z][a-zA-Z0-9 \.,\/\\:;\{\}~\!\@#\$%\^&\-\+_]+?))'
		RGX_MNT_ON_KEYWORD='on'
		RGX_MNT_ON_SPC='([\t ]('${RGX_MNT_ON_KEYWORD}')[\t ])'
		RGX_MNT_PATH='((\/)|(\/([^\/]|[\\])+?)+?)'
		RGX_MNT_TYPE_KEYWORD='type'
		RGX_MNT_TYPE='([a-zA-Z][a-zA-Z0-9\.,\/\\:;\{\}~\!\@#\$%\^&\-\+_]+?)'
		RGX_MNT_TYPE_COND='((([\t ]'${RGX_MNT_TYPE_KEYWORD}'[\t ])'${RGX_MNT_TYPE}')?)'
		RGX_MNT_OPTIONS='([\(]([a-zA-Z][a-zA-Z0-9 \.,\/\\:;\{\}~\!\@#\$%\^&\*\-\+_=]+?)[\)])'
		RGX_MNT_OPTIONS_SPC='([\t ])'${RGX_MNT_OPTIONS}''
		RGX_MNT_LABEL='(\[([\[\<\>\]a-zA-Z0-9\t \.,\/\\:;\{\}~\!\@#\$%\^&\*\(\)\-\+_=]+?)\])'
		RGX_MNT_LABEL_COND='(([\t ]'${RGX_MNT_LABEL}')?)'

		RGX_MNT_ALL_COND='^'${RGX_MNT_FS}''${RGX_MNT_ON_SPC}''${RGX_MNT_PATH}''${RGX_MNT_TYPE_COND}''${RGX_MNT_OPTIONS_SPC}''${RGX_MNT_LABEL_COND}'$'

		RC_RGX_MNT_FS_PRE=''
		RC_RGX_MNT_FS_POST=''${RGX_MNT_ON_SPC}''${RGX_MNT_PATH}''${RGX_MNT_TYPE_COND}''${RGX_MNT_OPTIONS_SPC}''${RGX_MNT_LABEL_COND}'$'

		RC_RGX_MNT_PATH_PRE='^'${RGX_MNT_FS}''${RGX_MNT_ON_SPC}''
		RC_RGX_MNT_PATH_POST=''${RGX_MNT_TYPE_COND}''${RGX_MNT_OPTIONS_SPC}''${RGX_MNT_LABEL_COND}'$'

		RC_RGX_MNT_TYPE_GNU_PRE='^'${RGX_MNT_FS}''${RGX_MNT_ON_SPC}''${RGX_MNT_PATH}''${RGX_TAB_OR_SPC}''${RGX_MNT_TYPE_KEYWORD}''${RGX_TAB_OR_SPC}''
		RC_RGX_MNT_TYPE_BSD_BR_PRE='^'${RGX_MNT_FS}''${RGX_MNT_ON_SPC}''${RGX_MNT_PATH}''${RGX_TAB_OR_SPC}'[(]'
		RC_RGX_MNT_TYPE_POST=''${RGX_MNT_OPTIONS_SPC}''${RGX_MNT_LABEL_COND}'$'

		RC_RGX_MNT_OPTIONS_BR_PRE='^'${RGX_MNT_FS}''${RGX_MNT_ON_SPC}''${RGX_MNT_PATH}''${RGX_MNT_TYPE_COND}''${RGX_TAB_OR_SPC}'[(]'
		RC_RGX_MNT_OPTIONS_POST=''${RGX_MNT_LABEL_COND}'$'

		if [ ".${script_debug_enabled}" = ".true" ]; then
			printf "RGX_MNT_FS = %s\\n" "'${RGX_MNT_FS}'" >> "${script_debug_log}"
			printf "RGX_MNT_PATH = %s\\n" "'${RGX_MNT_PATH}'" >> "${script_debug_log}"
			printf "RGX_MNT_TYPE_COND = %s\\n" "'${RGX_MNT_TYPE_COND}'" >> "${script_debug_log}"
			printf "RGX_MNT_OPTIONS = %s\\n" "'${RGX_MNT_OPTIONS}'" >> "${script_debug_log}"
			printf "RGX_MNT_LABEL_COND = %s\\n" "'${RGX_MNT_LABEL_COND}'" >> "${script_debug_log}"
			printf "RGX_MNT_ALL_COND = %s\\n" "'${RGX_MNT_ALL_COND}'" >> "${script_debug_log}"
		fi

##########################################################
		# start script preparation ( parse "mount" command output )

		SCRIPT_AWK_MNT_PREPARE='
BEGIN {
	if ("" debug "" == "1" || "" debug "" == "true") {
		debug_enabled = "true";
	}
	else
	{
		debug_enabled = "false";
	}

	if (debug_enabled == "true") {
		debug_log = "/tmp/script_1_mnt_prepare_debug.log";
		printf "" > debug_log;
		close(debug_log);

		output_log = "/tmp/script_1_mnt_prepare_output.log";
		printf "" > output_log;
		close(output_log);
	}

	# options values by default for compact output
	option_mnt_size_wide = 0;
	option_mnt_show_use_prc = 0;
	option_mnt_show_type = 0;
	option_mnt_show_fs = 1;

	# works only if mnt_options is not empty
	option_mnt_show_ro_rw = 1;

	if ("'${timeout_coreutils}'" == "true") {
		timeout_coreutils_available = "true";
	}
	else
	{
		timeout_coreutils_available = "false";
	}

	if (debug_enabled == "true") {
		print "[DEBUG] timeout_coreutils_available = [" timeout_coreutils_available "]" >> debug_log;
	}

	FS = " ";

	tab = "'${RGX_TAB}'";
	space = "'${RGX_SPC}'";
	tab_or_space = "'${RGX_TAB_OR_SPC}'";
	newline = "'${RGX_NEWLINE}'";
	single_quote = "\047";

	prev = "";
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
	if ( $0 !~ /'${RGX_MNT_ALL_COND}'/ ) {
		if (debug_enabled == "true") {
			print "" >> debug_log;
			print "[xxxxxxxxxxxxxxxx]" >> debug_log;
			print "[err] " $0 >> debug_log;
			print "[xxxxxxxxxxxxxxxx]" >> debug_log;
		}
	}
	else
	{
		if (debug_enabled == "true") {
			print "" >> debug_log;
			print "[================]" >> debug_log;
			print "[=]  " $0 >> debug_log;
			print "[================]" >> debug_log;
		}

		mnt_fs = $0;
		rc_mnt_fs_pre = sub(/'${RC_RGX_MNT_FS_PRE}'/, "", mnt_fs);
		rc_mnt_fs_post = sub(/'${RC_RGX_MNT_FS_POST}'/, "", mnt_fs);

		if (debug_enabled == "true") {
			print "[i]  mnt_fs = _" mnt_fs "_" >> debug_log;
		}

		mnt_path = $0;
		rc_mnt_path_pre = sub(/'${RC_RGX_MNT_PATH_PRE}'/, "", mnt_path);
		rc_mnt_path_post = sub(/'${RC_RGX_MNT_PATH_POST}'/, "", mnt_path);

		if (debug_enabled == "true") {
			print "[i]  mnt_path = _" mnt_path "_" >> debug_log;
		}

		mnt_type = $0;

		if (debug_enabled == "true") {
			print "[1st]  mnt_type = _" mnt_type "_" >> debug_log;
		}

		rc_mnt_type_pre = sub(/'${RC_RGX_MNT_TYPE_GNU_PRE}'/, "", mnt_type);

		if (debug_enabled == "true") {
			regex_mnt_type_pre = "'${RC_RGX_MNT_TYPE_GNU_PRE}'";
			print "[pre]  rc_mnt_type_pre = _" rc_mnt_type_pre "_ ; regex_mnt_type_pre = _" regex_mnt_type_pre "_" >> debug_log;
		}

		if (rc_mnt_type_pre == 0) {

			rc_mnt_type_pre = sub(/'${RC_RGX_MNT_TYPE_BSD_BR_PRE}'/, "", mnt_type);

			if (debug_enabled == "true") {
				regex_mnt_type_pre = "'${RC_RGX_MNT_TYPE_BSD_BR_PRE}'";
				print "[pre]  rc_mnt_type_pre = _" rc_mnt_type_pre "_ ; regex_mnt_type_pre = _" regex_mnt_type_pre "_" >> debug_log;
			}

			if (debug_enabled == "true") {
				print "[2nd]  mnt_type = _" mnt_type "_" >> debug_log;
			}

			mnt_type = " (" mnt_type;

			if (debug_enabled == "true") {
				print "[3rd]  mnt_type = _" mnt_type "_" >> debug_log;
			}
		}

		if (debug_enabled == "true") {
			print "[4th]  mnt_type = _" mnt_type "_" >> debug_log;
		}

		rc_mnt_type_post = sub(/'${RC_RGX_MNT_TYPE_POST}'/, "", mnt_type);

		if (debug_enabled == "true") {
			print "[i]  mnt_type = _" mnt_type "_" >> debug_log;
		}

		if (rc_mnt_type_pre == 1 && rc_mnt_type_post == 1 && mnt_type == "") {
			mnt_type = "[type]";
		}

		mnt_options = $0;

		if (debug_enabled == "true") {
			print "[1st]  mnt_options = _" mnt_options "_" >> debug_log;
		}

		rc_mnt_options_pre = sub(/'${RC_RGX_MNT_OPTIONS_BR_PRE}'/, "", mnt_options);

		if (debug_enabled == "true") {
			print "[2nd]  mnt_options = _" mnt_options "_" >> debug_log;
		}

		mnt_options = "(" mnt_options;

		if (debug_enabled == "true") {
			print "[3rd]  mnt_options = _" mnt_options "_" >> debug_log;
		}

		rc_mnt_options_post = sub(/'${RC_RGX_MNT_OPTIONS_POST}'/, "", mnt_options);

		if (debug_enabled == "true") {
			print "[i]  mnt_options = _" mnt_options "_" >> debug_log;
		}

		if (debug_enabled == "true") {
			print "_fs=" rc_mnt_fs_pre rc_mnt_fs_post \
				"_path=" rc_mnt_path_pre rc_mnt_path_post \
				"_type=" rc_mnt_type_pre rc_mnt_type_post \
				"_options=" rc_mnt_options_pre rc_mnt_options_post \
				>> debug_log;
		}

		if (rc_mnt_fs_pre == 1 && rc_mnt_fs_post == 1 \
			&& rc_mnt_path_pre == 1 && rc_mnt_path_post == 1 \
			&& rc_mnt_type_pre == 1 && rc_mnt_type_post == 1 \
			&& rc_mnt_options_pre == 1 && rc_mnt_options_post == 1) \
		{

			# number in "prefix"
			if (substr(mnt_fs, 1, 5) == "/dev/") {
				# number is zero for devices
				prefix = sprintf("%03.0f:%s $", 0, mnt_fs);
			}
			else
			{
				# number is record number for non-devices
				prefix = sprintf("%03.0f:%s $", NR, mnt_fs);
			}

			if (debug_enabled == "true") {
				# output "prefix"
				## printf "%03.0f:",NR >> output_log;
				## printf mnt_fs " $" tab >> output_log;

				printf "prefix = [ %s ]",prefix >> debug_log;
				printf newline >> debug_log;

				# emulate "df" command output separated by tab
				printf "%s",(mnt_fs tab mnt_type tab mnt_path tab) >> output_log;

				# append mnt_options to the end of "df" command output
				printf "" mnt_options >> output_log;
				printf newline >> output_log;
			}

			if (prev != prefix) {
				prev = prefix;

				var = "";

				snqt_mnt_path = mnt_path;
				gsub(/'"'"'/, "'"\'"'" "\"" "'"\'"'" "\"" "'"\'"'", snqt_mnt_path);
				snqt_mnt_path = sprintf(single_quote "%s" single_quote, snqt_mnt_path);

				if (timeout_coreutils_available == "true") {
					# work with timeout command from coreutils
					misc_exec = "( timeout --signal=TERM '${TIMEOUT}' df '${DF_ARGS}' " snqt_mnt_path " 2> /dev/null ) | head -n 2 | tail -n 1 ";
				}
				else
				{
					# work with sleep and kill command trick
					misc_exec = "( ( sleep '${TIMEOUT}' ; kill $$ 2> /dev/null ) & exec ; df '${DF_ARGS}' " snqt_mnt_path " 2> /dev/null ; echo ; echo ; ) | head -n 2 | tail -n 1 ";
				}

				"" misc_exec "" | getline var;
				close(misc_exec);

				if (debug_enabled == "true") {
					print "[info]  mnt_path = _" mnt_path "_" >> debug_log;
					print "[info]  snqt_mnt_path = _" snqt_mnt_path "_" >> debug_log;
					print "[info]  misc_exec = _" misc_exec "_" >> debug_log;
					print "[info]  var = _" var "_" >> debug_log;
				}

				df_output_parsed = "false";

				# parse "df" command output

				if (( var == "" ) || ( var !~ /'${RGX_DF_ALL_COND}'/ )) {

					if (debug_enabled == "true") {
						print "" >> debug_log;
						print "[xxxxxxxxxxxxxxxx]" >> debug_log;
						print "[err] " var >> debug_log;
						print "[xxxxxxxxxxxxxxxx]" >> debug_log;
					}

					df_fs = mnt_fs;
					df_type = mnt_type;
					df_total = "[size]";
					df_used = "[used]";
					df_avail = "[avail]";
					df_use_prc = "[useprc]";
					df_path = mnt_path;

					if (debug_enabled == "true") {
						# emulate "df" command output separated by tab
						# append mnt_options to the end of "df" command output
						print "[parsed]  " mnt_fs tab mnt_type tab "[size]" tab "[used]" tab "[avail]" tab "[useprc]" tab mnt_path tab mnt_options >> debug_log;
					}
				}
				else
				{
					if (debug_enabled == "true") {
						print "" >> debug_log;
						print "[================]" >> debug_log;
						print "[=]  " var >> debug_log;
						print "[================]" >> debug_log;
					}

					df_fs = var;

					if (debug_enabled == "true") {
						print "[1st]  df_fs = _" df_fs "_" >> debug_log;
					}

					rc_df_fs_pre = sub(/'${RC_RGX_DF_FS_PRE}'/, "", df_fs);

					if (debug_enabled == "true") {
						print "[2nd]  df_fs = _" df_fs "_" >> debug_log;
					}

					rc_df_fs_post = sub(/'${RC_RGX_DF_FS_POST}'/, "", df_fs);

					if (debug_enabled == "true") {
						print "[i]  df_fs = _" df_fs "_" >> debug_log;
					}

					df_type = var;
					rc_df_type_pre = sub(/'${RC_RGX_DF_TYPE_PRE}'/, "", df_type);

					if (debug_enabled == "true") {
						regex_df_type_pre = "'${RC_RGX_DF_TYPE_PRE}'";
						print "[pre]  rc_df_type_pre = _" rc_df_type_pre "_ ; regex_df_type_pre = _" regex_df_type_pre "_" >> debug_log;
					}

					if (debug_enabled == "true") {
						print "[1st]  df_type = _" df_type "_" >> debug_log;
					}

					rc_df_type_post = sub(/'${RC_RGX_DF_TYPE_GNU_POST}'/, "", df_type);

					if (debug_enabled == "true") {
						regex_df_type_post = "'${RC_RGX_DF_TYPE_GNU_POST}'";
						print "[post]  rc_df_type_post = _" rc_df_type_post "_ ; regex_df_type_post = _" regex_df_type_post "_" >> debug_log;
					}

					if (rc_df_type_post == 0) {

						if (debug_enabled == "true") {
							print "[2nd]  df_type = _" df_type "_" >> debug_log;
						}

						rc_df_type_post = sub(/'${RC_RGX_DF_TYPE_POSIX_POST}'/, "", df_type);

						if (debug_enabled == "true") {
							regex_df_type_post = "'${RC_RGX_DF_TYPE_POSIX_POST}'";
							print "[post]  rc_df_type_post = _" rc_df_type_post "_ ; regex_df_type_post = _" regex_df_type_post "_" >> debug_log;
						}
					}

					if (debug_enabled == "true") {
						print "[i]  df_type = _" df_type "_" >> debug_log;
					}

					if (rc_df_type_pre == 1 && rc_df_type_post == 1 && df_type == "") {
						df_type = "[type]";
					}

					df_total = var;
					rc_df_total_pre = sub(/'${RC_RGX_DF_TOTAL_GNU_PRE}'/, "", df_total);
					rc_df_total_post = sub(/'${RC_RGX_DF_TOTAL_POST}'/, "", df_total);

					if (debug_enabled == "true") {
						print "[i]  df_total = _" df_total "_" >> debug_log;
					}

					df_used = var;
					rc_df_used_pre = sub(/'${RC_RGX_DF_USED_GNU_PRE}'/, "", df_used);
					rc_df_used_post = sub(/'${RC_RGX_DF_USED_POST}'/, "", df_used);

					if (debug_enabled == "true") {
						print "[i]  df_used = _" df_used "_" >> debug_log;
					}

					df_avail = var;
					rc_df_avail_pre = sub(/'${RC_RGX_DF_AVAIL_GNU_PRE}'/, "", df_avail);
					rc_df_avail_post = sub(/'${RC_RGX_DF_AVAIL_POST}'/, "", df_avail);

					if (debug_enabled == "true") {
						print "[i]  df_avail = _" df_avail "_" >> debug_log;
					}

					df_use_prc = var;
					rc_df_use_prc_pre = sub(/'${RC_RGX_DF_USE_PRC_GNU_PRE}'/, "", df_use_prc);
					rc_df_use_prc_post = sub(/'${RC_RGX_DF_USE_PRC_POST}'/, "", df_use_prc);

					if (debug_enabled == "true") {
						print "[i]  df_use_prc = _" df_use_prc "_" >> debug_log;
					}

					df_path = var;
					rc_df_path_pre = sub(/'${RC_RGX_DF_PATH_GNU_PRE}'/, "", df_path);
					rc_df_path_post = sub(/'${RC_RGX_DF_PATH_POST}'/, "", df_path);

					if (debug_enabled == "true") {
						print "[i]  df_path = _" df_path "_" >> debug_log;
					}

					if (debug_enabled == "true") {
						print "_fs=" rc_df_fs_pre rc_df_fs_post \
							"_type=" rc_df_type_pre rc_df_type_post \
							"_total=" rc_df_total_pre rc_df_total_post \
							"_used=" rc_df_used_pre rc_df_used_post \
							"_avail=" rc_df_avail_pre rc_df_avail_post \
							"_use_prc=" rc_df_use_prc_pre rc_df_use_prc_post \
							"_path=" rc_df_path_pre rc_df_path_post \
							>> debug_log;
					}

					if (rc_df_fs_pre == 1 && rc_df_fs_post == 1 \
						&& rc_df_type_pre == 1 && rc_df_type_post == 1 \
						&& rc_df_total_pre == 1 && rc_df_total_post == 1 \
						&& rc_df_used_pre == 1 && rc_df_used_post == 1 \
						&& rc_df_avail_pre == 1 && rc_df_avail_post == 1 \
						&& rc_df_use_prc_pre == 1 && rc_df_use_prc_post == 1 \
						&& rc_df_path_pre == 1 && rc_df_path_post == 1) \
					{

						df_output_parsed = "true";

						if (debug_enabled == "true") {
							# emulate "df" command output separated by tab
							# append mnt_options to the end of "df" command output
							print "[parsed]  " df_fs tab df_type tab df_total tab df_used tab df_avail tab df_use_prc tab df_path tab mnt_options >> debug_log;
						}
					}
				}

				check_df_path = "false";

				# better filtering with "grep -v -e ..." will be done before this moment
				if (substr(df_path, 1, 1) == "/" && df_path != "/dev/foo") {

					check_df_path = "true";
				}

				if (check_df_path == "true") {

					avail = ( df_avail+0.0 ) / '${DF_DIVBY}';
					total = ( df_total+0.0 ) / '${DF_DIVBY}';

					num_df_avail = avail;
					num_df_total = total;

					total_units="K";
					if (total >= 1024*1024*1024) {
						total_units="T"; total/= 1024*1024*1024;
					}
					else if (total >= 1024*1024) {
						total_units="G"; total/= 1024*1024;
					}
					else if (total >= 1024) {
						total_units="M"; total/= 1024;
					}

					avail_units="K";
					if (avail >= 1024*1024*1024) {
						avail_units="T"; avail/= 1024*1024*1024;
					}
					else if (avail >= 1024*1024) {
						avail_units="G"; avail/= 1024*1024;
					}
					else if (avail >= 1024) {
						avail_units="M"; avail/= 1024;
					}

					if (option_mnt_size_wide == 1) {

						avail_ident = "";
						total_ident = "";

						avail_fraction_len = avail < 99.9 ? ( avail < 9.9 ? 3 : 2 ) : 1;
						total_fraction_len = total < 99.9 ? ( total < 9.9 ? 3 : 2 ) : 1;
					}
					else {

						avail_ident = avail < 99.9 ? ( avail < 9.9 ? "" : "_" ) : "";
						total_ident = total < 99.9 ? ( total < 9.9 ? "" : " " ) : "";

						avail_fraction_len = avail < 9.9 ? 1 : 0;
						total_fraction_len = total < 9.9 ? 1 : 0;
					}

					avail_str = avail "";
					avail_len = length(avail_str);
					avail_fraction_dot_pos = 0;

					if (avail_len > 0) {

						if ( avail_str ~ /'^[0-9]+[\.][0-9]+$'/ ) {

							avail_fraction_dot_pos = match(avail_str, "[.]");
						}
					}

					avail_fraction_pos = (avail_fraction_dot_pos > 0 && avail_fraction_dot_pos < avail_len) ? ( avail_fraction_dot_pos + 1 ) : avail_len;
					avail_fraction = (avail_fraction_pos == avail_len) ? "0" : substr((avail ""), avail_fraction_pos, avail_fraction_len);

					total_str = total "";
					total_len = length(total_str);
					total_fraction_dot_pos = 0;

					if (total_len > 0) {

						if ( total_str ~ /'^[0-9]+[\.][0-9]+$'/ ) {

							total_fraction_dot_pos = match(total_str, "[.]");
						}
					}

					total_fraction_pos = (total_fraction_dot_pos > 0 && total_fraction_dot_pos < total_len) ? ( total_fraction_dot_pos + 1 ) : total_len;
					total_fraction = (total_fraction_pos == total_len) ? "0" : substr((total ""), total_fraction_pos, total_fraction_len);

					if (option_mnt_size_wide == 1) {

						sep_avail = substr((avail_ident avail "." avail_fraction), 1, 5);
						sep_total = substr((total_ident total "." total_fraction), 1, 5);
					}
					else {

						sep_avail = substr((avail_ident avail "." avail_fraction), 1, 3);
						sep_total = substr((total_ident total "." total_fraction), 1, 3);
					}

					if (debug_enabled == "true") {
						printf "[round]  num_df_avail = [%s] ; avail = [%s] ; avail_str = [%s] ; avail_len = [%s] ; avail_units = [%s] ; avail_fraction_len = [%s] ; avail_fraction_pos = [%s] ; avail_fraction = [%s] ; avail_ident = [%s] ; sep_avail = [%s] ;", \
							num_df_avail, avail, avail_str, avail_len, avail_units, avail_fraction_len, avail_fraction_pos, avail_fraction, avail_ident, sep_avail \
							>> debug_log;

						printf newline >> debug_log;

						printf "[round]  num_df_total = [%s] ; total = [%s] ; total_str = [%s] ; total_len = [%s] ; total_units = [%s] ; total_fraction_len = [%s] ; total_fraction_pos = [%s] ; total_fraction = [%s] ; total_ident = [%s] ; sep_total = [%s] ;", \
							num_df_total, total, total_str, total_len, total_units, total_fraction_len, total_fraction_pos, total_fraction, total_ident, sep_total \
							>> debug_log;

						printf newline >> debug_log;
					}

					rw_mode_label = "";

					rc_rw_mode = match(mnt_options, "[( ,]rw[,]");

					rc_ro_mode = match(mnt_options, "[( ,]ro[,]");

					if (rc_rw_mode > 0) {
						rw_mode_label = "";
					}

					if (rc_ro_mode > 0) {
						rw_mode_label = "-";
					}

					separator_symbol = "/";

					if (option_mnt_show_use_prc == 1) {

						separator_symbol = " ";

						if (df_use_prc == "100%") {

							if (num_df_avail <= 1024) {

								sep_df_use_prc = separator_symbol "++%";
							}
							else
							{
								sep_df_use_prc = separator_symbol "99%";
							}
						}
						else
						{
							sep_df_use_prc = separator_symbol df_use_prc;
						}
					}
					else
					{
						sep_df_use_prc = "";
					}

					if (option_mnt_show_type == 1) {

						sep_df_type = df_type;
					}
					else
					{
						sep_df_type = "";
					}

					if (option_mnt_show_fs == 1) {

						if (option_mnt_show_type == 1) {

							sep_df_fs = " > " df_fs;
						}
						else
						{
							sep_df_fs = df_fs;
						}
					}
					else
					{
						sep_df_fs = "";
					}

					if (option_mnt_show_ro_rw == 1) {

						if (option_mnt_show_type == 1 || option_mnt_show_fs == 1) {

							sep_rw_mode_label = rw_mode_label " ";
						}
						else
						{
							sep_rw_mode_label = rw_mode_label;
						}
					}
					else
					{
						sep_rw_mode_label = "";
					}

					print_format = "%s\t" "%s%s" "%s" "%s%s" "%s\t" "%s%s%s";

					printf print_format, \
						df_path, sep_avail, avail_units, separator_symbol, \
						sep_total, total_units, sep_df_use_prc, \
						sep_rw_mode_label, sep_df_type, sep_df_fs;

					printf newline;

					if (debug_enabled == "true") {
						printf print_format, \
							df_path, sep_avail, avail_units, separator_symbol, \
							sep_total, total_units, sep_df_use_prc, \
							sep_rw_mode_label, sep_df_type, sep_df_fs >> output_log;

						printf newline >> debug_log;
					}
				}
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
		# finish script preparation ( parse "mount" command output )

		#TESTME: awk script for parsing of "mount" and "df" output with spaces and tabs

		if [ ".${script_debug_enabled}" = ".true" ]; then
			printf "%s\\n" "${SCRIPT_AWK_MNT_PREPARE}" > "/tmp/script_1_mnt_prepare.awk"
			## echo "${SCRIPT_AWK_MNT_PREPARE}" > "/tmp/script_2_mnt_prepare.awk"
		fi

##########################################################
	else
		USE_MOUNT_CMD='false'

		DF_ARGS='-P -k'

		# for 1024-byte blocks (POSIX and GNU ?) total blocks count should be divided by 1
		# for 512-byte blocks (non-POSIX ?) total blocks count should be divided by 2
		DF_DIVBY=1

		DF='( df '${DF_ARGS}' )'

		# DF_AVAIL=4
		# DF_TOTAL=2
		# DF_NAME=1
		# DF_USE=5
		# DF_MNT_ON=6

		DF_CMN_FILTER='grep -v -e " /dev\(/\(shm\|foo\|bar\)\)\?$" -e " /run\(/\(lock\|foo\|bar\)\)\?$" -e " /net$" -e " /home$" -e " /private/var/\(vm\|folders\)\(\(\/\)\|\(\/[^/]\+\)\+\)$" -e " /System/Volumes\(\(\/\)\|\(\/[^/]\+\)\+\)$"'
		DF_USR_FILTER='grep -v -e " /var\(/\(foo\|bar\)\)\?$" -e " /opt\(/\(foo\|bar\)\)\?$"'

		#FIXME: paths that contain repeated continuos spaces
		#TESTME: awk script parsing of "df" output with spaces and tabs

##########################################################
		# start script preparation ( parse "df" command output )

		SCRIPT_AWK_DF_PARSE='
BEGIN {
	if ("" debug "" == "1" || "" debug "" == "true") {
		debug_enabled = "true";
	}
	else {
		debug_enabled = "false";
	}

	if (debug_enabled == "true") {
		debug_log = "/tmp/script_1_df_parse_debug.log";
		printf "" > debug_log;
		close(debug_log);

		output_log = "/tmp/script_1_df_parse_output.log";
		printf "" > output_log;
		close(output_log);
	}

	# options values by default for compact output
	option_mnt_size_wide = 0;
	option_mnt_show_use_prc = 0;
	option_mnt_show_type = 0;
	option_mnt_show_fs = 1;

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
	var = $0;

	df_output_parsed = "false";

	# parse "df" command output

	if (( var == "" ) || ( var !~ /'${RGX_DF_ALL_COND}'/ )) {
		if (debug_enabled == "true") {
			print "" >> debug_log;
			print "[xxxxxxxxxxxxxxxx]" >> debug_log;
			print "[err] " var >> debug_log;
			print "[xxxxxxxxxxxxxxxx]" >> debug_log;
		}
	}
	else
	{
		if (debug_enabled == "true") {
			print "" >> debug_log;
			print "[================]" >> debug_log;
			print "[=]  " var >> debug_log;
			print "[================]" >> debug_log;
		}

		df_fs = var;

		if (debug_enabled == "true") {
			print "[1st]  df_fs = _" df_fs "_" >> debug_log;
		}

		rc_df_fs_pre = sub(/'${RC_RGX_DF_FS_PRE}'/, "", df_fs);

		if (debug_enabled == "true") {
			print "[2nd]  df_fs = _" df_fs "_" >> debug_log;
		}

		rc_df_fs_post = sub(/'${RC_RGX_DF_FS_POST}'/, "", df_fs);

		if (debug_enabled == "true") {
			print "[i]  df_fs = _" df_fs "_" >> debug_log;
		}

		df_type = var;
		rc_df_type_pre = sub(/'${RC_RGX_DF_TYPE_PRE}'/, "", df_type);

		if (debug_enabled == "true") {
			regex_df_type_pre = "'${RC_RGX_DF_TYPE_PRE}'";
			print "[pre]  rc_df_type_pre = _" rc_df_type_pre "_ ; regex_df_type_pre = _" regex_df_type_pre "_" >> debug_log;
		}

		if (debug_enabled == "true") {
			print "[1st]  df_type = _" df_type "_" >> debug_log;
		}

		rc_df_type_post = sub(/'${RC_RGX_DF_TYPE_GNU_POST}'/, "", df_type);

		if (debug_enabled == "true") {
			regex_df_type_post = "'${RC_RGX_DF_TYPE_GNU_POST}'";
			print "[post]  rc_df_type_post = _" rc_df_type_post "_ ; regex_df_type_post = _" regex_df_type_post "_" >> debug_log;
		}

		if (rc_df_type_post == 0 ) {

			if (debug_enabled == "true") {
				print "[2nd]  df_type = _" df_type "_" >> debug_log;
			}

			rc_df_type_post = sub(/'${RC_RGX_DF_TYPE_POSIX_POST}'/, "", df_type);

			if (debug_enabled == "true") {
				regex_df_type_post = "'${RC_RGX_DF_TYPE_POSIX_POST}'";
				print "[post]  rc_df_type_post = _" rc_df_type_post "_ ; regex_df_type_post = _" regex_df_type_post "_" >> debug_log;
			}
		}

		if (debug_enabled == "true") {
			print "[i]  df_type = _" df_type "_" >> debug_log;
		}

		if (rc_df_type_pre == 1 && rc_df_type_post == 1 && df_type == "") {
			df_type = "[type]";
		}

		df_total = var;
		rc_df_total_pre = sub(/'${RC_RGX_DF_TOTAL_GNU_PRE}'/, "", df_total);
		rc_df_total_post = sub(/'${RC_RGX_DF_TOTAL_POST}'/, "", df_total);

		if (debug_enabled == "true") {
			print "[i]  df_total = _" df_total "_" >> debug_log;
		}

		df_used = var;
		rc_df_used_pre = sub(/'${RC_RGX_DF_USED_GNU_PRE}'/, "", df_used);
		rc_df_used_post = sub(/'${RC_RGX_DF_USED_POST}'/, "", df_used);

		if (debug_enabled == "true") {
			print "[i]  df_used = _" df_used "_" >> debug_log;
		}

		df_avail = var;
		rc_df_avail_pre = sub(/'${RC_RGX_DF_AVAIL_GNU_PRE}'/, "", df_avail);
		rc_df_avail_post = sub(/'${RC_RGX_DF_AVAIL_POST}'/, "", df_avail);

		if (debug_enabled == "true") {
			print "[i]  df_avail = _" df_avail "_" >> debug_log;
		}

		df_use_prc = var;
		rc_df_use_prc_pre = sub(/'${RC_RGX_DF_USE_PRC_GNU_PRE}'/, "", df_use_prc);
		rc_df_use_prc_post = sub(/'${RC_RGX_DF_USE_PRC_POST}'/, "", df_use_prc);

		if (debug_enabled == "true") {
			print "[i]  df_use_prc = _" df_use_prc "_" >> debug_log;
		}

		df_path = var;
		rc_df_path_pre = sub(/'${RC_RGX_DF_PATH_GNU_PRE}'/, "", df_path);
		rc_df_path_post = sub(/'${RC_RGX_DF_PATH_POST}'/, "", df_path);

		if (debug_enabled == "true") {
			print "[i]  df_path = _" df_path "_" >> debug_log;
		}

		if (debug_enabled == "true") {
			print "_fs=" rc_df_fs_pre rc_df_fs_post \
				"_type=" rc_df_type_pre rc_df_type_post \
				"_total=" rc_df_total_pre rc_df_total_post \
				"_used=" rc_df_used_pre rc_df_used_post \
				"_avail=" rc_df_avail_pre rc_df_avail_post \
				"_use_prc=" rc_df_use_prc_pre rc_df_use_prc_post \
				"_path=" rc_df_path_pre rc_df_path_post \
				>> debug_log;
		}

		if (rc_df_fs_pre == 1 && rc_df_fs_post == 1 \
			&& rc_df_type_pre == 1 && rc_df_type_post == 1 \
			&& rc_df_total_pre == 1 && rc_df_total_post == 1 \
			&& rc_df_used_pre == 1 && rc_df_used_post == 1 \
			&& rc_df_avail_pre == 1 && rc_df_avail_post == 1 \
			&& rc_df_use_prc_pre == 1 && rc_df_use_prc_post == 1 \
			&& rc_df_path_pre == 1 && rc_df_path_post == 1) \
		{

			df_output_parsed = "true";

			if (debug_enabled == "true") {
				# emulate "df" command output separated by tab
				print "[parsed]  " df_fs tab df_type tab df_total tab df_used tab df_avail tab df_use_prc tab df_path >> debug_log;
			}
		}
	}

	check_df_path = "false";

	# better filtering with "grep -v -e ..." will be done before this moment
	if (substr(df_path, 1, 1) == "/" && df_path != "/dev/foo") {

		check_df_path = "true";
	}

	if (df_output_parsed == "true" && check_df_path == "true") {

		avail = ( df_avail+0.0 ) / '${DF_DIVBY}';
		total = ( df_total+0.0 ) / '${DF_DIVBY}';

		num_df_avail = avail;
		num_df_total = total;

		total_units="K";
		if (total >= 1024*1024*1024) {
			total_units="T"; total/= 1024*1024*1024;
		}
		else if (total >= 1024*1024) {
			total_units="G"; total/= 1024*1024;
		}
		else if (total >= 1024) {
			total_units="M"; total/= 1024;
		}

		avail_units="K";
		if (avail >= 1024*1024*1024) {
			avail_units="T"; avail/= 1024*1024*1024;
		}
		else if (avail >= 1024*1024) {
			avail_units="G"; avail/= 1024*1024;
		}
		else if (avail >= 1024) {
			avail_units="M"; avail/= 1024;
		}

		if (option_mnt_size_wide == 1) {

			avail_ident = "";
			total_ident = "";

			avail_fraction_len = avail < 99.9 ? ( avail < 9.9 ? 3 : 2 ) : 1;
			total_fraction_len = total < 99.9 ? ( total < 9.9 ? 3 : 2 ) : 1;
		}
		else {

			avail_ident = avail < 99.9 ? ( avail < 9.9 ? "" : "_" ) : "";
			total_ident = total < 99.9 ? ( total < 9.9 ? "" : " " ) : "";

			avail_fraction_len = avail < 9.9 ? 1 : 0;
			total_fraction_len = total < 9.9 ? 1 : 0;
		}

		avail_str = avail "";
		avail_len = length(avail_str);
		avail_fraction_dot_pos = 0;

		if (avail_len > 0) {

			if ( avail_str ~ /'^[0-9]+[\.][0-9]+$'/ ) {

				avail_fraction_dot_pos = match(avail_str, "[.]");
			}
		}

		avail_fraction_pos = (avail_fraction_dot_pos > 0 && avail_fraction_dot_pos < avail_len) ? ( avail_fraction_dot_pos + 1 ) : avail_len;
		avail_fraction = (avail_fraction_pos == avail_len) ? "0" : substr((avail ""), avail_fraction_pos, avail_fraction_len);

		total_str = total "";
		total_len = length(total_str);
		total_fraction_dot_pos = 0;

		if (total_len > 0) {

			if ( total_str ~ /'^[0-9]+[\.][0-9]+$'/ ) {

				total_fraction_dot_pos = match(total_str, "[.]");
			}
		}

		total_fraction_pos = (total_fraction_dot_pos > 0 && total_fraction_dot_pos < total_len) ? ( total_fraction_dot_pos + 1 ) : total_len;
		total_fraction = (total_fraction_pos == total_len) ? "0" : substr((total ""), total_fraction_pos, total_fraction_len);

		if (option_mnt_size_wide == 1) {

			sep_avail = substr((avail_ident avail "." avail_fraction), 1, 5);
			sep_total = substr((total_ident total "." total_fraction), 1, 5);
		}
		else {

			sep_avail = substr((avail_ident avail "." avail_fraction), 1, 3);
			sep_total = substr((total_ident total "." total_fraction), 1, 3);
		}

		if (debug_enabled == "true") {
			printf "[round]  num_df_avail = [%s] ; avail = [%s] ; avail_str = [%s] ; avail_len = [%s] ; avail_units = [%s] ; avail_fraction_len = [%s] ; avail_fraction_pos = [%s] ; avail_fraction = [%s] ; avail_ident = [%s] ; sep_avail = [%s] ;", \
				num_df_avail, avail, avail_str, avail_len, avail_units, avail_fraction_len, avail_fraction_pos, avail_fraction, avail_ident, sep_avail \
				>> debug_log;

			printf newline >> debug_log;

			printf "[round]  num_df_total = [%s] ; total = [%s] ; total_str = [%s] ; total_len = [%s] ; total_units = [%s] ; total_fraction_len = [%s] ; total_fraction_pos = [%s] ; total_fraction = [%s] ; total_ident = [%s] ; sep_total = [%s] ;", \
				num_df_total, total, total_str, total_len, total_units, total_fraction_len, total_fraction_pos, total_fraction, total_ident, sep_total \
				>> debug_log;

			printf newline >> debug_log;
		}

		separator_symbol = "/";

		if (option_mnt_show_use_prc == 1) {

			separator_symbol = " ";

			if (df_use_prc == "100%") {

				if (num_df_avail <= 1024) {

					sep_df_use_prc = separator_symbol "++%";
				}
				else
				{
					sep_df_use_prc = separator_symbol "99%";
				}
			}
			else
			{
				sep_df_use_prc = separator_symbol df_use_prc;
			}
		}
		else
		{
			sep_df_use_prc = "";
		}

		if (option_mnt_show_type == 1) {

			sep_df_type = df_type;
		}
		else
		{
			sep_df_type = "";
		}

		if (option_mnt_show_fs == 1) {

			if (option_mnt_show_type == 1) {

				sep_df_fs = " > " df_fs;
			}
			else
			{
				sep_df_fs = df_fs;
			}
		}
		else
		{
			sep_df_fs = "";
		}

		sep_rw_mode_label = "";

		print_format = "%s\t" "%s%s" "%s" "%s%s" "%s\t" "%s%s%s";

		printf print_format, \
			df_path, sep_avail, avail_units, separator_symbol, \
			sep_total, total_units, sep_df_use_prc, \
			sep_rw_mode_label, sep_df_type, sep_df_fs;

		printf newline;

		if (debug_enabled == "true") {
			printf print_format, \
				df_path, sep_avail, avail_units, separator_symbol, \
				sep_total, total_units, sep_df_use_prc, \
				sep_rw_mode_label, sep_df_type, sep_df_fs >> output_log;

			printf newline >> debug_log;
		}
	}
}
END {
	if (debug_enabled == "true") {
		close(debug_log);
		close(output_log);
	}
}'

		# finish script preparation ( parse "df" command output )

		if [ ".${script_debug_enabled}" = ".true" ]; then
			printf "%s\\n" "${SCRIPT_AWK_DF_PARSE}" > "/tmp/script_1_df_parse.awk"
			printf "%s" "${DF}" > /tmp/df_wrapper_1.sh
		fi
	fi

##########################################################
	if [ ".${USE_MOUNT_CMD}" = ".true" ]; then

		sh -c '( mount )' \
			| sh ${VAR_SCRIPT_GREP_DEBUG} -c "${MNT_CMN_FILTER}" \
			| sh ${VAR_SCRIPT_GREP_DEBUG} -c "${MNT_USR_FILTER}" \
			| awk ${VAR_SCRIPT_AWK_DEBUG} ${AWK_ARG_SRCF} ' '"${SCRIPT_AWK_MNT_PREPARE}"' ' \
			| cat

##########################################################
	else

		sh -c "${DF}" 2>/dev/null \
			| sh ${VAR_SCRIPT_GREP_DEBUG} -c "${DF_CMN_FILTER}" \
			| sh ${VAR_SCRIPT_GREP_DEBUG} -c "${DF_USR_FILTER}" \
			| awk ${VAR_SCRIPT_AWK_DEBUG} ${AWK_ARG_SRCF} ' '"${SCRIPT_AWK_DF_PARSE}"' ' \
			| cat
	fi

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
