# This script is compactized when sent:
# All comments and empty lines are discared.
# Tokens started by SHELLVAR_ and SHELLFCN_ are renamed to shorter names.

export PS1=;export PS2=;export PS3=;export PS4=;export PROMPT_COMMAND=;
if [ "$0" = "bash" ] || [ "$0" = "-bash" ]; then bind 'set enable-bracketed-paste off'; fi
export LANG=C
export LC_TIME=C
export LS_COLORS=
export SHELLVAR_LOG=/dev/null
#export SHELLVAR_LOG=/tmp/nr-shell.log

SHELLFCN_SEND_ERROR_AND_RESYNC() {
 SHELLVAR_ERRCNT=`expr $SHELLVAR_ERRCNT + 1`
 ERRID="$$-$(date)-$SHELLVAR_ERRCNT"
 echo "resync.req: $1:$ERRID" >>$SHELLVAR_LOG
 echo;echo "+ERROR:$1:$ERRID"
 while true; do
  read STR || exit
  echo "resync.rpl: $STR" >>$SHELLVAR_LOG
  [ "$STR" = "SHELL_RESYNCHRONIZATION_ERROR:$1:$ERRID" ] && break
 done
}

SHELLFCN_GET_INFO() {
# $1 - path
# $2 - nonempty if follow symlink
# $3 - stat format
# $4 - find format
# $5 - optional ls filter

 if [ -n "$SHELLVAR_STAT" ]; then
  if [ -n "$2" ]; then
   stat -L --format="$3" "$1" 2>>$SHELLVAR_LOG || echo +ERROR:$?
  else
   stat --format="$3" "$1" 2>>$SHELLVAR_LOG || echo +ERROR:$?
  fi

 elif [ -n "$SHELLVAR_FIND" ]; then
  if [ -n "$2" ]; then
   find -H "$1" -mindepth 0 -maxdepth 0 -printf "$4" 2>>$SHELLVAR_LOG || echo +ERROR:$?
  else
   find "$1" -mindepth 0 -maxdepth 0 -printf "$4" 2>>$SHELLVAR_LOG || echo +ERROR:$?
  fi

 else
  SHELLVAR_SELECTED_LS_ARGS=$SHELLVAR_LS_ARGS
  if [ -n "$2" ]; then
   SHELLVAR_SELECTED_LS_ARGS=$SHELLVAR_LS_ARGS_FOLLOW
  fi
  if [ -n "$5" ]; then
   ( ( ls -d $SHELLVAR_SELECTED_LS_ARGS "$1" 2>>$SHELLVAR_LOG | grep $SHELLVAR_GREP_ARGS '^[^cbt]' || echo +ERROR:$? 1>&2 ) | ( read $5; echo $y ) ) 2>&1
  else
   ls -d $SHELLVAR_SELECTED_LS_ARGS "$1" 2>>$SHELLVAR_LOG | grep $SHELLVAR_GREP_ARGS '^[^cbt]' || echo +ERROR:$?
  fi
 fi
}

SHELLFCN_GET_SIZE() {
 SHELLFCN_GET_INFO "$1" '1' "$SHELLVAR_STAT_FMT_SIZE" "$SHELLVAR_FIND_FMT_SIZE" 'n n n n y n'
}

SHELLFCN_CMD_ENUM() {
 read SHELLVAR_ARG_PATH || exit
 if [ -n "$SHELLVAR_STAT" ]; then
  stat --format="$SHELLVAR_STAT_FMT" "$SHELLVAR_ARG_PATH"/* "$SHELLVAR_ARG_PATH"/.* 2>>$SHELLVAR_LOG
 elif [ -n "$SHELLVAR_FIND" ]; then
  find -H "$SHELLVAR_ARG_PATH" -mindepth 1 -maxdepth 1 -printf "$SHELLVAR_FIND_FMT" 2>>$SHELLVAR_LOG
 else
  ls -H $SHELLVAR_LS_ARGS "$SHELLVAR_ARG_PATH" 2>>$SHELLVAR_LOG | grep $SHELLVAR_GREP_ARGS '^[^cbt]'
 fi
}

SHELLFCN_CMD_INFO() {
# !!! This is a directory with symlinks listing bottleneck !!!
# TODO: Rewrite so instead of querying files one-by-one do grouped queries with one stat per several files
 while true; do
  read SHELLVAR_ARG_PATH || exit
  [ ! -n "$SHELLVAR_ARG_PATH" ] && break
  SHELLFCN_GET_INFO "$SHELLVAR_ARG_PATH" "$1" "$2" "$3" "$4"
 done
}

SHELLFCN_CHOOSE_BLOCK() {
# $1 - size want to write
# $2 - position want to seek (so chosen block size will be its divider)
   BLOCK=1
   [ $1 -ge 16 ] && [ `expr '(' $2 / 16 ')' '*' 16` = "$2" ] && BLOCK=16
   [ $1 -ge 64 ] && [ `expr '(' $2 / 64 ')' '*' 64` = "$2" ] && BLOCK=64
   [ $1 -ge 512 ] && [ `expr '(' $2 / 512 ')' '*' 512` = "$2" ] && BLOCK=512
   [ $1 -ge 4096 ] && [ `expr '(' $2 / 4096 ')' '*' 4096` = "$2" ] && BLOCK=4096
   [ $1 -ge 65536 ] && [ `expr '(' $2 / 65536 ')' '*' 65536` = "$2" ] && BLOCK=65536
}

SHELLFCN_CMD_READ() {
 read OFFSET SHELLVAR_ARG_PATH || exit
 SIZE=`SHELLFCN_GET_SIZE "$SHELLVAR_ARG_PATH"`
 if [ ! -n "$SIZE" ]; then
    echo '+FAIL'
    return
 fi
 SHELLVAR_STATE=
 if [ -n "$SHELLVAR_DD" ]; then
  while true; do
   REMAIN=`expr $SIZE - $OFFSET`
   read SHELLVAR_STATE || exit
   if [ "$SHELLVAR_STATE" = 'abort' ]; then
    echo '+ABORTED'
    read SHELLVAR_STATE || exit
    break
   fi
   if [ $REMAIN -le 0 ]; then
    echo '+DONE'
    read SHELLVAR_STATE || exit
    break
   fi
   SHELLFCN_CHOOSE_BLOCK $REMAIN $OFFSET
   PIECE=$REMAIN
   [ $PIECE -gt 8388608 ] && PIECE=8388608
   CNT=`expr $PIECE / $BLOCK`
   PIECE=`expr $CNT '*' $BLOCK`
   echo '+NEXT:'$PIECE
   dd iflag=fullblock skip=`expr $OFFSET / $BLOCK` count=$CNT bs=$BLOCK if="${SHELLVAR_ARG_PATH}" 2>>$SHELLVAR_LOG
   ERR=$?
   if [ $ERR -ne 0 ]; then
    # its unknown how much data was actually read, so send $PIECE of zeroes followed with ERROR statement
    # unless its client aborted transfer by sending CtrlC
    [ "$SHELLVAR_STATE" = 'abort' ] || dd iflag=fullblock count=$CNT bs=$BLOCK if=/dev/zero 2>>$SHELLVAR_LOG
    SHELLFCN_SEND_ERROR_AND_RESYNC "$ERR"
    break
   fi
   OFFSET=`expr $OFFSET + $PIECE`
  done

 elif [ "$OFFSET" = '0' ]; then
  # no dd? fallback to cat, if can
  read SHELLVAR_STATE || exit
  echo '+NEXT:'$SIZE
  cat "$SHELLVAR_ARG_PATH" 2>>$SHELLVAR_LOG
  echo '+DONE'
  read SHELLVAR_STATE || exit

 else
  echo '+FAIL'
 fi
}

SHELLFCN_WRITE_BY_DD() {
# $1 - size
# $2 - offset
# $3 - file
 SHELLFCN_CHOOSE_BLOCK $1 $2
 DDCNT=`expr $1 / $BLOCK`
 DDPIECE=`expr $DDCNT '*' $BLOCK`
 DDSEEK=`expr $2 '/' $BLOCK`
 dd iflag=fullblock seek=$DDSEEK count=$DDCNT bs=$BLOCK of="$3" 2>>$SHELLVAR_LOG
 rv=$?
 if [ $rv -eq 0 ] && [ $DDPIECE -ne $1 ]; then
  SHELLFCN_WRITE_BY_DD `expr $1 - $DDPIECE` `expr $2 + $DDPIECE` "$3"
  rv=$?
 fi
 return $rv
}

SHELLFCN_WRITE_BY_HEAD() {
# $1 - size
# $2 - offset
# $3 - file
 SHELLVAR_WBH_SIZE_EXPECTED=`expr $1 + $2`
 SHELLVAR_WBH_SIZE_REMAIN=$1
#  echo "HEADING $1 bytes at $2" >>$SHELLVAR_LOG
#  echo "HEADED $1 bytes at $2 size before=$(SIZE_BEFORE) after=$(SHELLFCN_GET_SIZE "$3") " >>$SHELLVAR_LOG
 while true; do
  if [ "$2" = '0' ]; then
   head -c $SHELLVAR_WBH_SIZE_REMAIN >"$3" 2>>$SHELLVAR_LOG
   SHELLVAR_WBH_RV=$?
  else
   head -c $SHELLVAR_WBH_SIZE_REMAIN >>"$3" 2>>$SHELLVAR_LOG
   SHELLVAR_WBH_RV=$?
  fi
  SHELLVAR_WBH_SIZE=$(SHELLFCN_GET_SIZE "$3")
  [ $SHELLVAR_WBH_RV -ne 0 ] || return $SHELLVAR_WBH_RV
  [ $SHELLVAR_WBH_SIZE -ge $SHELLVAR_WBH_SIZE_EXPECTED ] && return 0
  SHELLVAR_WBH_SIZE_REMAIN=`expr $SHELLVAR_WBH_SIZE_EXPECTED - $SHELLVAR_WBH_SIZE`
 done
}

SHELLFCN_WRITE_BY_BASE64() {
# $1 - size (ignored as whole line will be read)
# $2 - offset
# $3 - file
 read B64LN
 if [ "$2" = '0' ]; then
   echo "$B64LN" | base64 -d >"$3" 2>>$SHELLVAR_LOG
 else
   echo "$B64LN" | base64 -d >>"$3" 2>>$SHELLVAR_LOG
 fi
}

SHELLFCN_CMD_WRITE() {
 read OFFSET SHELLVAR_ARG_PATH || exit
 if [ -n "$SHELLVAR_DD" ] || [ -n "$SHELLVAR_HEAD" ] || [ -n "$SHELLVAR_BASE64" ]; then
  SHELLFCN_WRITE=SHELLFCN_WRITE_BY_BASE64
  [ -n "$SHELLVAR_HEAD" ] && SHELLFCN_WRITE=SHELLFCN_WRITE_BY_HEAD
  [ -n "$SHELLVAR_DD" ] && SHELLFCN_WRITE=SHELLFCN_WRITE_BY_DD
  if ! [ -n "$SHELLVAR_DD" ] && [ "$OFFSET" != '0' ] && ! truncate --size="$OFFSET" "$SHELLVAR_ARG_PATH" >>$SHELLVAR_LOG 2>&1 ; then
   SHELLFCN_SEND_ERROR_AND_RESYNC "$?"
   # avoid futher writings
   SHELLVAR_ARG_PATH=/dev/null
  else
    echo '+OK'
  fi
  NSEQ=1
  while true; do
   while true; do
    read SEQ SIZE || exit
    [ "$SIZE" = '' ] || break
   done
   [ "$SEQ" = '.' ] && break
   if [ $NSEQ -eq $SEQ ] && $SHELLFCN_WRITE $SIZE $OFFSET "$SHELLVAR_ARG_PATH"; then
    echo '+OK'
    OFFSET=`expr $OFFSET + $SIZE`
    NSEQ=`expr $NSEQ + 1`
   else
    SHELLFCN_SEND_ERROR_AND_RESYNC "SEQ=$SEQ NSEQ=$NSEQ $?"
    # avoid futher writings
    SHELLVAR_ARG_PATH=/dev/null
   fi
  done
 else
  SHELLFCN_SEND_ERROR_AND_RESYNC "$?"
 fi
}

SHELLFCN_CMD_REMOVE_FILE() {
 read SHELLVAR_ARG_PATH || exit
 unlink "$SHELLVAR_ARG_PATH" >>$SHELLVAR_LOG 2>&1 || rm -f "$SHELLVAR_ARG_PATH" >>$SHELLVAR_LOG 2>&1
 RV=$?
 if [ "$RV" != "0" ]; then
  echo "+ERROR:$RV"
 fi
}

SHELLFCN_CMD_REMOVE_DIR() {
 read SHELLVAR_ARG_PATH || exit
 rmdir "$SHELLVAR_ARG_PATH" >>$SHELLVAR_LOG 2>&1 || rm -f "$SHELLVAR_ARG_PATH" >>$SHELLVAR_LOG 2>&1
 RV=$?
 if [ "$RV" != "0" ]; then
  echo "+ERROR:$RV"
 fi
}

SHELLFCN_CMD_CREATE_DIR() {
 read SHELLVAR_ARG_PATH || exit
 read SHELLVAR_ARG_MODE || exit
 mkdir -m "$SHELLVAR_ARG_MODE" "$SHELLVAR_ARG_PATH" >>$SHELLVAR_LOG 2>&1
 RV=$?
 if [ "$RV" != "0" ]; then
  echo "+ERROR:$RV"
 fi
}

SHELLFCN_CMD_RENAME() {
 read SHELLVAR_ARG_PATH1 || exit
 read SHELLVAR_ARG_PATH2 || exit
 mv "$SHELLVAR_ARG_PATH1" "$SHELLVAR_ARG_PATH2" >>$SHELLVAR_LOG 2>&1
 RV=$?
 if [ "$RV" != "0" ]; then
  echo "+ERROR:$RV"
 fi
}

SHELLFCN_READLINK_BY_LS() {
# lrwxrwxrwx 1 root root 7 Feb 11  2023 /bin -> usr/bin
 ls -d $SHELLVAR_LS_ARGS "$1" 2>/dev/null | ( read W1 W2 W3 W4 W5 W6 W7 W8 W9 W10 W11 W12 W13 W14 W15
  RV=$?
  if [ $RV -eq 0 ]; then
   [ "$W14" = '->' ] && echo "$W15"
   [ "$W13" = '->' ] && echo "$W14"
   [ "$W12" = '->' ] && echo "$W13"
   [ "$W11" = '->' ] && echo "$W12"
   [ "$W10" = '->' ] && echo "$W11"
   [ "$W9" = '->' ] && echo "$W10"
   [ "$W8" = '->' ] && echo "$W9"
   [ "$W7" = '->' ] && echo "$W8"
  fi
  return $RV
 )
}

SHELLFCN_CMD_READ_SYMLINK() {
 read SHELLVAR_ARG_PATH_LINK || exit
 LNK=`$SHELLVAR_READLINK_FN "$SHELLVAR_ARG_PATH_LINK" 2>>$SHELLVAR_LOG`
 RV=$?
 if [ $RV -eq 0 ]; then
  echo "+OK:$LNK"
 else
  echo "+ERROR:$RV"
 fi
}

SHELLFCN_CMD_MAKE_SYMLINK() {
 read SHELLVAR_ARG_PATH_LINK || exit
 read SHELLVAR_ARG_PATH_FILE || exit
 ln -s "$SHELLVAR_ARG_PATH_FILE" "$SHELLVAR_ARG_PATH_LINK" >>$SHELLVAR_LOG 2>&1
 RV=$?
 if [ "$RV" != "0" ]; then
  echo "+ERROR:$RV"
 fi
}

SHELLFCN_CMD_SET_MODE() {
 read SHELLVAR_ARG_PATH || exit
 read SHELLVAR_ARG_MODE || exit
 chmod "$SHELLVAR_ARG_MODE" "$SHELLVAR_ARG_PATH" >>$SHELLVAR_LOG 2>&1
 RV=$?
 if [ "$RV" != "0" ]; then
  echo "+ERROR:$RV"
 fi
}

SHELLFCN_CMD_EXECUTE() {
 read SHELLVAR_ARG_CMD || exit
 read SHELLVAR_ARG_WD || exit
 read SHELLVAR_ARG_TOKEN || exit
 if cd "$SHELLVAR_ARG_WD"; then
  $SHELLVAR_ARG_CMD
  RV=$?
  echo;echo "$SHELLVAR_ARG_TOKEN:$RV"
 else
  echo;echo "$SHELLVAR_ARG_TOKEN:-1"
 fi
}

###

# NAME:MODE:SIZE:ACCESS:MODIFY:STATUS:USER:GROUP

SHELLVAR_STAT_FMT='%n %f %s %X %Y %Z %U %G'
SHELLVAR_FIND_FMT='%f %M %s %A@ %T@ %C@ %u %g\n'
SHELLVAR_STAT_FMT_INFO='%f %s %X %Y %Z'
SHELLVAR_FIND_FMT_INFO='%M %s %A@ %T@ %C@\n'
SHELLVAR_STAT_FMT_SIZE='%s'
SHELLVAR_FIND_FMT_SIZE='%s\n'
SHELLVAR_STAT_FMT_MODE='%f'
SHELLVAR_FIND_FMT_MODE='%M\n'

SHELLVAR_READLINK_FN=SHELLFCN_READLINK_BY_LS
SHELLVAR_FIND=
SHELLVAR_STAT=
SHELLVAR_LS_ARGS='-l -A'
SHELLVAR_DD=
SHELLVAR_HEAD=
SHELLVAR_WRITE_BLOCK=
SHELLVAR_BASE64=
SHELLVAR_ERRCNT=0
SHELLVAR_GREP_ARGS=


if which readlink >>$SHELLVAR_LOG 2>&1; then
 SHELLVAR_READLINK_FN=readlink
fi

if echo foo | grep --color=never foo >>$SHELLVAR_LOG 2>&1; then
 SHELLVAR_GREP_ARGS='--color=never'
fi

if find -H . -mindepth 0 -maxdepth 0 -printf "$SHELLVAR_FIND_FMT" >>$SHELLVAR_LOG 2>&1; then
 SHELLVAR_FIND=Y
fi

if stat -L --format="$SHELLVAR_STAT_FMT" . >>$SHELLVAR_LOG 2>&1; then
 SHELLVAR_STAT=Y
fi

if ls -f $SHELLVAR_LS_ARGS . >>$SHELLVAR_LOG 2>&1; then
 SHELLVAR_LS_ARGS="-f $SHELLVAR_LS_ARGS"
fi

if ls -d -H . >>$SHELLVAR_LOG 2>&1; then
 SHELLVAR_LS_ARGS_FOLLOW="$SHELLVAR_LS_ARGS -H"
else
 SHELLVAR_LS_ARGS_FOLLOW="$SHELLVAR_LS_ARGS"
fi

# using dd requires also expr command
if dd iflag=fullblock skip=16 count=16 bs=16 if=/dev/zero 2>>$SHELLVAR_LOG >/dev/null && [ `expr 1 + 2` = '3' ] ; then
 SHELLVAR_DD=Y
fi
if [ "`echo 'helloworld' | ( head -c 5 > /dev/null 2>>$SHELLVAR_LOG; cat )`" = world ]; then
 SHELLVAR_HEAD=Y
elif [ "`echo 'helloworld' | head -c 5 2>>$SHELLVAR_LOG`" = hello ]; then
 # head is here but it reads stdin with buffering, so let
 # peer know about this so it will pad each unaligned send with \r
 SHELLVAR_WRITE_BLOCK=`head -c 131072 /dev/zero | ( head -c 1 > /dev/null; cat ) | wc -c`
 SHELLVAR_WRITE_BLOCK=`expr 131072 - $SHELLVAR_WRITE_BLOCK`
 [ $SHELLVAR_WRITE_BLOCK -le 65536 ] && [ $SHELLVAR_WRITE_BLOCK -gt 1 ] && SHELLVAR_HEAD=Y
 SHELLVAR_WRITE_BLOCK="$SHELLVAR_WRITE_BLOCK*`expr 2097152 / $SHELLVAR_WRITE_BLOCK`"
fi

[ "`echo aGVsbG8K | base64 -d 2>>$SHELLVAR_LOG`" = hello ] && SHELLVAR_BASE64=Y

#debug
#SHELLVAR_STAT=
#SHELLVAR_FIND=
#SHELLVAR_DD=
#SHELLVAR_HEAD=
#SHELLVAR_BASE64=
#echo "SHELLVAR_LS_ARGS=$SHELLVAR_LS_ARGS"

INFO=
if [ -n "$SHELLVAR_STAT" ]; then
 INFO="${INFO}STAT "
elif [ -n "$SHELLVAR_FIND" ]; then
 INFO="${INFO}FIND "
else
 INFO="${INFO}LS "
fi

if [ -n "$SHELLVAR_DD" ]; then
 INFO="${INFO}READ_RESUME WRITE_RESUME "
elif [ -n "$SHELLVAR_HEAD" ]; then
 INFO="${INFO}READ WRITE_RESUME "
 [ -n "$SHELLVAR_WRITE_BLOCK" ] && INFO="${INFO}WRITE_BLOCK=$SHELLVAR_WRITE_BLOCK "
elif [ -n "$SHELLVAR_BASE64" ]; then
 INFO="${INFO}READ WRITE_RESUME WRITE_BASE64 "
else
 INFO="${INFO}READ "
fi

echo " ${INFO}SHELL.FAR2L"
SHELLVAR_NOPROMPT=
while true; do
 [ ! -n "$SHELLVAR_NOPROMPT" ] && echo && echo '>(((^>'
 SHELLVAR_NOPROMPT=
 read CMD || exit
 case "$CMD" in
  enum ) SHELLFCN_CMD_ENUM;;
  linfo ) SHELLFCN_CMD_INFO '0' "$SHELLVAR_STAT_FMT_INFO" "$SHELLVAR_FIND_FMT_INFO" '';;
  info ) SHELLFCN_CMD_INFO '1' "$SHELLVAR_STAT_FMT_INFO" "$SHELLVAR_FIND_FMT_INFO" '';;
  lmode ) SHELLFCN_CMD_INFO '0' "$SHELLVAR_STAT_FMT_MODE" "$SHELLVAR_FIND_FMT_MODE" 'y n n n n n';;
  mode ) SHELLFCN_CMD_INFO '1' "$SHELLVAR_STAT_FMT_MODE" "$SHELLVAR_FIND_FMT_MODE" 'y n n n n n';;
  lsize ) SHELLFCN_CMD_INFO '0' "$SHELLVAR_STAT_FMT_SIZE" "$SHELLVAR_FIND_FMT_SIZE" 'n n n n y n';;
  size ) SHELLFCN_CMD_INFO '1' "$SHELLVAR_STAT_FMT_SIZE" "$SHELLVAR_FIND_FMT_SIZE" 'n n n n y n';;
  read ) SHELLFCN_CMD_READ;;
  write ) SHELLFCN_CMD_WRITE;;
  rmfile ) SHELLFCN_CMD_REMOVE_FILE;;
  rmdir ) SHELLFCN_CMD_REMOVE_DIR;;
  mkdir ) SHELLFCN_CMD_CREATE_DIR;;
  rename ) SHELLFCN_CMD_RENAME;;
  chmod ) SHELLFCN_CMD_SET_MODE;;
  rdsym ) SHELLFCN_CMD_READ_SYMLINK;;
  mksym ) SHELLFCN_CMD_MAKE_SYMLINK;;
  exec ) SHELLFCN_CMD_EXECUTE;;
# special cases: casual 'abort' or 'cont' could be from cancelled read operation, so silently skip them
  abort ) echo 'Odd abort' >>$SHELLVAR_LOG; SHELLVAR_NOPROMPT=Y;;
  cont ) echo 'Odd cont' >>$SHELLVAR_LOG; SHELLVAR_NOPROMPT=Y;;
  noop ) ;;
  exit ) echo '73!'; exit 0;;
  * ) echo "Bad CMD='$CMD'" >>$SHELLVAR_LOG; echo "??? '$CMD'";;
 esac
done
