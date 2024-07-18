#!/bin/sh
# workaround for https://github.com/elfmz/far2l/issues/1658

script_path=$(dirname "$(readlink -f "$0")")

case "$1" in
get)
    powershell.exe -Command Get-Clipboard
;;
set)
    CONTENT=$(cat)
    echo -n "$CONTENT" | clip.exe
    echo -n "$CONTENT"
;;
"")
    (far2l --clipboard=$(readlink -f $0) >/dev/null 2>&1 &)
;;
esac
