# This script is compactized when sent:
# All comments and empty lines are discarded.
# Tokens started by SHELLVAR_ and SHELLFCN_ are renamed to shorter names.

SAVED_PS1=$PS1;SAVED_PS2=$PS2;SAVED_PS2=$PS3;SAVED_PS4=$PS4;SAVED_PC=$PROMPT_COMMAND;export PS1=;export PS2=;export PS3=;export PS4=;export PROMPT_COMMAND=;
#SAVED_STTY=`stty -g`
#stty -opost -echo

if [ "$0" = "bash" ] || [ "$0" = "-bash" ]; then bind 'set enable-bracketed-paste off'; fi
export FISH=far2l
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
  $SHELLVAR_READ_FN STR || exit
  echo "resync.rpl: $STR" >>$SHELLVAR_LOG
  [ "$STR" = "SHELL_RESYNCHRONIZATION_ERROR:$1:$ERRID" ] && break
 done
}

SHELLFCN_GET_INFO_INNER() {
# $1 - path
# $2 - nonempty if follow symlink
# $3 - stat format
# $4 - find format
# $5 - optional ls filter
 if [ -n "$SHELLVAR_STAT" ]; then
  if [ -n "$2" ]; then
   stat -L --format="$3" "$1"
  else
   stat --format="$3" "$1"
  fi

 elif [ -n "$SHELLVAR_FIND" ]; then
  if [ -n "$2" ]; then
   find -H "$1" -mindepth 0 -maxdepth 0 -printf "$4"
  else
   find "$1" -mindepth 0 -maxdepth 0 -printf "$4"
  fi

 else
  SHELLVAR_SELECTED_LS_ARGS=$SHELLVAR_LS_ARGS
  [ -n "$2" ] && SHELLVAR_SELECTED_LS_ARGS=$SHELLVAR_LS_ARGS_FOLLOW
  if [ -n "$5" ]; then
   ls -d $SHELLVAR_SELECTED_LS_ARGS "$1" | grep $SHELLVAR_GREP_ARGS '^[^cbt]' | ( $SHELLVAR_READ_FN $5; echo $y )
  else
   ls -d $SHELLVAR_SELECTED_LS_ARGS "$1" | grep $SHELLVAR_GREP_ARGS '^[^cbt]'
  fi
 fi
}

SHELLFCN_GET_SIZE() {
 SHELLVAR_INFO=`SHELLFCN_GET_INFO_INNER "$1" '1' "$SHELLVAR_STAT_FMT_SIZE" "$SHELLVAR_FIND_FMT_SIZE" 'n n n n y n' 2>>$SHELLVAR_LOG`
 [ -n "$SHELLVAR_INFO" ] || SHELLVAR_INFO=0
 echo "$SHELLVAR_INFO"
}

SHELLFCN_GET_INFO() {
 SHELLVAR_OUT=`SHELLFCN_GET_INFO_INNER "$@" 2>>$SHELLVAR_LOG`
 if [ -n "$SHELLVAR_OUT" ]; then
  echo "+OK:$SHELLVAR_OUT"
 else
  SHELLVAR_OUT=`SHELLFCN_GET_INFO_INNER "$@" 2>&1`
  echo "+ERROR:$SHELLVAR_OUT"
 fi
}

SHELLFCN_CMD_ENUM() {
 if [ -n "$SHELLVAR_STAT" ]; then
  stat --format="$SHELLVAR_STAT_FMT" "$SHELLVAR_ARG"/* "$SHELLVAR_ARG"/.* 2>>$SHELLVAR_LOG
 elif [ -n "$SHELLVAR_FIND" ]; then
  find -H "$SHELLVAR_ARG" -mindepth 1 -maxdepth 1 -printf "$SHELLVAR_FIND_FMT" 2>>$SHELLVAR_LOG
 else
  ls $SHELLVAR_LS_ARGS_FOLLOW "$SHELLVAR_ARG" 2>>$SHELLVAR_LOG | grep $SHELLVAR_GREP_ARGS '^[^cbt]'
 fi
}

SHELLFCN_CMD_INFO_SINGLE() {
 SHELLFCN_GET_INFO "$SHELLVAR_ARG" "$1" "$2" "$3" "$4"
}

SHELLFCN_CMD_INFO_MULTI() {
# !!! This is a directory with symlinks listing bottleneck !!!
# TODO: Rewrite so instead of querying files one-by-one do grouped queries with one stat per several files
 while [ $SHELLVAR_ARG -gt 0 ]; do
  $SHELLVAR_READ_FN SHELLVAR_PATH1 SHELLVAR_PATH2 SHELLVAR_PATH3 SHELLVAR_PATH4 SHELLVAR_PATH5 SHELLVAR_PATH6 SHELLVAR_PATH7 SHELLVAR_PATH8 || exit
  [ $SHELLVAR_ARG -ge 1 ] && SHELLFCN_GET_INFO "$SHELLVAR_PATH1" "$1" "$2" "$3" "$4"
  [ $SHELLVAR_ARG -ge 2 ] && SHELLFCN_GET_INFO "$SHELLVAR_PATH2" "$1" "$2" "$3" "$4"
  [ $SHELLVAR_ARG -ge 3 ] && SHELLFCN_GET_INFO "$SHELLVAR_PATH3" "$1" "$2" "$3" "$4"
  [ $SHELLVAR_ARG -ge 4 ] && SHELLFCN_GET_INFO "$SHELLVAR_PATH4" "$1" "$2" "$3" "$4"
  [ $SHELLVAR_ARG -ge 5 ] && SHELLFCN_GET_INFO "$SHELLVAR_PATH5" "$1" "$2" "$3" "$4"
  [ $SHELLVAR_ARG -ge 6 ] && SHELLFCN_GET_INFO "$SHELLVAR_PATH6" "$1" "$2" "$3" "$4"
  [ $SHELLVAR_ARG -ge 7 ] && SHELLFCN_GET_INFO "$SHELLVAR_PATH7" "$1" "$2" "$3" "$4"
  [ $SHELLVAR_ARG -ge 8 ] && SHELLFCN_GET_INFO "$SHELLVAR_PATH8" "$1" "$2" "$3" "$4"
  SHELLVAR_ARG=`expr $SHELLVAR_ARG - 8`
 done
}

SHELLFCN_CHOOSE_BLOCK() {
# $1 - size want to write
# $2 - position want to seek (so chosen block size will be its divider)
   SHELLVAR_BLOCK=1
   [ $1 -ge 16 ] && [ `expr $2 % 16` -eq 0 ] && SHELLVAR_BLOCK=16
   [ $1 -ge 64 ] && [ `expr $2 % 64` -eq 0 ] && SHELLVAR_BLOCK=64
   [ $1 -ge 512 ] && [ `expr $2 % 512` -eq 0 ] && SHELLVAR_BLOCK=512
   [ $1 -ge 4096 ] && [ `expr $2 % 4096` -eq 0 ] && SHELLVAR_BLOCK=4096
   [ $1 -ge 65536 ] && [ `expr $2 % 65536` -eq 0 ] && SHELLVAR_BLOCK=65536
}

SHELLFCN_CMD_READ() {
 $SHELLVAR_READ_FN SHELLVAR_OFFSET || exit
 SHELLVAR_SIZE=`SHELLFCN_GET_SIZE "$SHELLVAR_ARG"`
 if [ ! -n "$SHELLVAR_SIZE" ]; then
    echo '+FAIL'
    return
 fi
 SHELLVAR_STATE=
 if [ -n "$SHELLVAR_DD" ]; then
  while true; do
   SHELLVAR_REMAIN=`expr $SHELLVAR_SIZE - $SHELLVAR_OFFSET`
   $SHELLVAR_READ_FN SHELLVAR_STATE || exit
   if [ "$SHELLVAR_STATE" = 'abort' ]; then
    echo '+ABORTED'
    $SHELLVAR_READ_FN SHELLVAR_STATE || exit
    break
   fi
   if [ $SHELLVAR_REMAIN -le 0 ]; then
    echo '+DONE'
    $SHELLVAR_READ_FN SHELLVAR_STATE || exit
    break
   fi
   SHELLFCN_CHOOSE_BLOCK $SHELLVAR_REMAIN $SHELLVAR_OFFSET
   SHELLVAR_PIECE=$SHELLVAR_REMAIN
   [ $SHELLVAR_PIECE -gt 8388608 ] && SHELLVAR_PIECE=8388608
   CNT=`expr $SHELLVAR_PIECE / $SHELLVAR_BLOCK`
   SHELLVAR_PIECE=`expr $CNT '*' $SHELLVAR_BLOCK`
   echo '+NEXT:'$SHELLVAR_PIECE
   dd iflag=fullblock skip=`expr $SHELLVAR_OFFSET / $SHELLVAR_BLOCK` count=$CNT bs=$SHELLVAR_BLOCK if="${SHELLVAR_ARG}" 2>>$SHELLVAR_LOG
   ERR=$?
   if [ $ERR -ne 0 ]; then
    # its unknown how much data was actually read, so send $SHELLVAR_PIECE of zeroes followed with ERROR statement
    # unless its client aborted transfer by sending CtrlC
    [ "$SHELLVAR_STATE" = 'abort' ] || dd iflag=fullblock count=$CNT bs=$SHELLVAR_BLOCK if=/dev/zero 2>>$SHELLVAR_LOG
    SHELLFCN_SEND_ERROR_AND_RESYNC "$ERR"
    break
   fi
   SHELLVAR_OFFSET=`expr $SHELLVAR_OFFSET + $SHELLVAR_PIECE`
  done

 elif [ $SHELLVAR_OFFSET -eq 0 ]; then
  # no dd? fallback to cat, if can
  $SHELLVAR_READ_FN SHELLVAR_STATE || exit
  echo '+NEXT:'$SHELLVAR_SIZE
  cat "$SHELLVAR_ARG" 2>>$SHELLVAR_LOG
  echo '+DONE'
  $SHELLVAR_READ_FN SHELLVAR_STATE || exit

 else
  echo '+FAIL'
 fi
}

SHELLFCN_WRITE_BY_DD() {
# $1 - size
# $2 - offset
# $3 - file
 SHELLFCN_CHOOSE_BLOCK $1 $2
 SHELLVAR_DDCNT=`expr $1 / $SHELLVAR_BLOCK`
 SHELLVAR_DDPIECE=`expr $SHELLVAR_DDCNT '*' $SHELLVAR_BLOCK`
 SHELLVAR_DDSEEK=`expr $2 '/' $SHELLVAR_BLOCK`
 dd iflag=fullblock seek=$SHELLVAR_DDSEEK count=$SHELLVAR_DDCNT bs=$SHELLVAR_BLOCK of="$3" 2>>$SHELLVAR_LOG
 RV=$?
 if [ $RV -eq 0 ] && [ $SHELLVAR_DDPIECE -ne $1 ]; then
  SHELLFCN_WRITE_BY_DD `expr $1 - $SHELLVAR_DDPIECE` `expr $2 + $SHELLVAR_DDPIECE` "$3"
  RV=$?
 fi
 return $RV
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
  if [ $2 -eq 0 ]; then
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
 $SHELLVAR_READ_FN B64LN
 if [ $2 -eq 0 ]; then
   echo "$B64LN" | base64 -d >"$3" 2>>$SHELLVAR_LOG
 else
   echo "$B64LN" | base64 -d >>"$3" 2>>$SHELLVAR_LOG
 fi
}

SHELLFCN_CMD_WRITE() {
 $SHELLVAR_READ_FN SHELLVAR_OFFSET || exit
 if [ -n "$SHELLVAR_DD" ] || [ -n "$SHELLVAR_HEAD" ] || [ -n "$SHELLVAR_BASE64" ]; then
  SHELLFCN_WRITE=SHELLFCN_WRITE_BY_BASE64
  [ -n "$SHELLVAR_HEAD" ] && SHELLFCN_WRITE=SHELLFCN_WRITE_BY_HEAD
  [ -n "$SHELLVAR_DD" ] && SHELLFCN_WRITE=SHELLFCN_WRITE_BY_DD
  if ! [ -n "$SHELLVAR_DD" ] && [ $SHELLVAR_OFFSET -ne 0 ] && ! truncate --size="$SHELLVAR_OFFSET" "$SHELLVAR_ARG" >>$SHELLVAR_LOG 2>&1 ; then
   SHELLFCN_SEND_ERROR_AND_RESYNC "$?"
   # avoid further writings
   SHELLVAR_ARG=/dev/null
  else
    echo '+OK'
  fi
  NSEQ=1
  while true; do
   while true; do
    $SHELLVAR_READ_FN SEQ SHELLVAR_SIZE || exit
    [ "$SHELLVAR_SIZE" = '' ] || break
   done
   if [ "$SEQ" = '.' ]; then
    # have to create/truncate file if there was no data written
    [ $NSEQ -eq 1 ] && ( touch "$SHELLVAR_ARG"; truncate --size="$SHELLVAR_OFFSET" "$SHELLVAR_ARG" ) >>$SHELLVAR_LOG 2>&1
    break
   fi
   if [ $NSEQ -eq $SEQ ] && $SHELLFCN_WRITE $SHELLVAR_SIZE $SHELLVAR_OFFSET "$SHELLVAR_ARG"; then
    echo '+OK'
    SHELLVAR_OFFSET=`expr $SHELLVAR_OFFSET + $SHELLVAR_SIZE`
    NSEQ=`expr $NSEQ + 1`
   else
    SHELLFCN_SEND_ERROR_AND_RESYNC "SEQ=$SEQ NSEQ=$NSEQ $?"
    # avoid further writings
    SHELLVAR_ARG=/dev/null
   fi
  done
 else
  SHELLFCN_SEND_ERROR_AND_RESYNC "$?"
 fi
}

SHELLFCN_CMD_REMOVE_FILE() {
 SHELLVAR_OUT=`unlink "$SHELLVAR_ARG" 2>&1 || rm -f "$SHELLVAR_ARG" 2>&1`
 RV=$?
 [ $RV -ne 0 ] && echo "+ERROR:$SHELLVAR_OUT"
}

SHELLFCN_CMD_REMOVE_DIR() {
 SHELLVAR_OUT=`rmdir "$SHELLVAR_ARG" 2>&1 || rm -f "$SHELLVAR_ARG" 2>&1`
 RV=$?
 [ $RV -ne 0 ] && echo "+ERROR:$SHELLVAR_OUT"
}

SHELLFCN_CMD_CREATE_DIR() {
 $SHELLVAR_READ_FN SHELLVAR_ARG_MODE || exit
 SHELLVAR_OUT=`mkdir -m "$SHELLVAR_ARG_MODE" "$SHELLVAR_ARG" 2>&1`
 RV=$?
 [ $RV -ne 0 ] && echo "+ERROR:$SHELLVAR_OUT"
}

SHELLFCN_CMD_RENAME() {
 $SHELLVAR_READ_FN SHELLVAR_ARG_DEST || exit
 SHELLVAR_OUT=`mv "$SHELLVAR_ARG" "$SHELLVAR_ARG_DEST" 2>&1`
 RV=$?
 [ $RV -ne 0 ] && echo "+ERROR:$SHELLVAR_OUT"
}

SHELLFCN_READLINK_BY_LS() {
# lrwxrwxrwx 1 root root 7 Feb 11  2023 /bin -> usr/bin
 ls -d $SHELLVAR_LS_ARGS "$1" 2>/dev/null | ( $SHELLVAR_READ_FN W1 W2 W3 W4 W5 W6 W7 W8 W9 W10 W11 W12 W13 W14 W15
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
 SHELLVAR_OUT=`$SHELLVAR_READLINK_FN "$SHELLVAR_ARG" 2>>$SHELLVAR_LOG`
 RV=$?
 if [ $RV -eq 0 ]; then
  echo "+OK:$SHELLVAR_OUT"
 else
  SHELLVAR_OUT=`$SHELLVAR_READLINK_FN "$SHELLVAR_ARG" 2>&1`
  echo "+ERROR:$SHELLVAR_OUT"
 fi
}

SHELLFCN_CMD_MAKE_SYMLINK() {
 $SHELLVAR_READ_FN SHELLVAR_ARG_DEST || exit
 SHELLVAR_OUT=`ln -s "$SHELLVAR_ARG_DEST" "$SHELLVAR_ARG" 2>&1`
 RV=$?
 [ $RV -ne 0 ] && echo "+ERROR:$SHELLVAR_OUT"
}

SHELLFCN_CMD_SET_MODE() {
 $SHELLVAR_READ_FN SHELLVAR_ARG_MODE || exit
 SHELLVAR_OUT=`chmod "$SHELLVAR_ARG_MODE" "$SHELLVAR_ARG" 2>&1`
 RV=$?
 [ $RV -ne 0 ] && echo "+ERROR:$SHELLVAR_OUT"
}

SHELLFCN_CMD_EXECUTE() {
 $SHELLVAR_READ_FN SHELLVAR_ARG_TOKEN SHELLVAR_ARG_WD || exit
 if cd "$SHELLVAR_ARG_WD"; then
  eval $SHELLVAR_ARG
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
SHELLVAR_READ_FN=read

if echo XXX | read -s XXX >/dev/null 2>&1; then
 SHELLVAR_READ_FN='read -s'
fi

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
elif ls -d -L . >>$SHELLVAR_LOG 2>&1; then
 SHELLVAR_LS_ARGS_FOLLOW="$SHELLVAR_LS_ARGS -L"
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
 # peer know about this so it will pad each unaligned send with \n
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

SHELLVAR_FEATS=
if [ -n "$SHELLVAR_STAT" ]; then
 SHELLVAR_FEATS="${SHELLVAR_FEATS}STAT "
elif [ -n "$SHELLVAR_FIND" ]; then
 SHELLVAR_FEATS="${SHELLVAR_FEATS}FIND "
else
 SHELLVAR_FEATS="${SHELLVAR_FEATS}LS "
fi

if [ -n "$SHELLVAR_DD" ]; then
 SHELLVAR_FEATS="${SHELLVAR_FEATS}READ_RESUME WRITE_RESUME "
elif [ -n "$SHELLVAR_HEAD" ]; then
 SHELLVAR_FEATS="${SHELLVAR_FEATS}READ WRITE_RESUME "
 [ -n "$SHELLVAR_WRITE_BLOCK" ] && SHELLVAR_FEATS="${SHELLVAR_FEATS}WRITE_BLOCK=$SHELLVAR_WRITE_BLOCK "
elif [ -n "$SHELLVAR_BASE64" ]; then
 SHELLVAR_FEATS="${SHELLVAR_FEATS}READ WRITE_RESUME WRITE_BASE64 "
else
 SHELLVAR_FEATS="${SHELLVAR_FEATS}READ "
fi

echo;echo;echo;
SHELLVAR_NOPROMPT=
while true; do
 [ ! -n "$SHELLVAR_NOPROMPT" ] && echo && echo '>(((^>'
 SHELLVAR_NOPROMPT=
 $SHELLVAR_READ_FN SHELLVAR_CMD SHELLVAR_ARG || exit
 case "$SHELLVAR_CMD" in
  feats ) echo "FEATS ${SHELLVAR_FEATS} SHELL.FAR2L";;
  enum ) SHELLFCN_CMD_ENUM;;
  linfo ) SHELLFCN_CMD_INFO_SINGLE '0' "$SHELLVAR_STAT_FMT_INFO" "$SHELLVAR_FIND_FMT_INFO" '';;
  info ) SHELLFCN_CMD_INFO_SINGLE '1' "$SHELLVAR_STAT_FMT_INFO" "$SHELLVAR_FIND_FMT_INFO" '';;
  lsize ) SHELLFCN_CMD_INFO_SINGLE '0' "$SHELLVAR_STAT_FMT_SIZE" "$SHELLVAR_FIND_FMT_SIZE" 'n n n n y n';;
  size ) SHELLFCN_CMD_INFO_SINGLE '1' "$SHELLVAR_STAT_FMT_SIZE" "$SHELLVAR_FIND_FMT_SIZE" 'n n n n y n';;
  lmode ) SHELLFCN_CMD_INFO_SINGLE '0' "$SHELLVAR_STAT_FMT_MODE" "$SHELLVAR_FIND_FMT_MODE" 'y n n n n n';;
  mode ) SHELLFCN_CMD_INFO_SINGLE '1' "$SHELLVAR_STAT_FMT_MODE" "$SHELLVAR_FIND_FMT_MODE" 'y n n n n n';;
  lmodes ) SHELLFCN_CMD_INFO_MULTI '0' "$SHELLVAR_STAT_FMT_MODE" "$SHELLVAR_FIND_FMT_MODE" 'y n n n n n';;
  modes ) SHELLFCN_CMD_INFO_MULTI '1' "$SHELLVAR_STAT_FMT_MODE" "$SHELLVAR_FIND_FMT_MODE" 'y n n n n n';;
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
# Special cases: casual 'abort' or 'cont' could be from cancelled read operation, so silently skip them
  abort ) echo 'Odd abort' >>$SHELLVAR_LOG; SHELLVAR_NOPROMPT=Y;;
  cont ) echo 'Odd cont' >>$SHELLVAR_LOG; SHELLVAR_NOPROMPT=Y;;
  noop ) ;;
  exit ) echo '73!'; exit 0; break;;
# Another special case - if its part of initial sequence supposed to be sent to shell
# - lets mimic shell's response so negotiation sequence will continue.
# sleep 3 ensures 'fishy' prompt will be printed at the right time moment
  echo ) echo; echo 'far2l is ready for fishing'; sleep 3;;
  * ) echo "Bad CMD='$SHELLVAR_CMD'" >>$SHELLVAR_LOG; echo "??? '$SHELLVAR_CMD'";;
 esac
done
