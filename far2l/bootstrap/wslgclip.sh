#!/bin/sh
# workaround for https://github.com/elfmz/far2l/issues/1658

script_path=$(dirname "$(readlink -f "$0")")

case "$1" in
get)
    cscript.exe //Nologo //u $(wslpath -w "$script_path"/wslgclip.vbs) | iconv -f utf-16le -t utf-8

#   powershell method is slower, also it have unsolved charset problems. disabled for now

#    if command -v cscript.exe >/dev/null 2>&1; then
#        cscript.exe //Nologo //u $(wslpath -w "$script_path"/wslgclip.vbs) | iconv -f utf-16le -t utf-8
#    else
#        powershell.exe -Command Get-Clipboard
#    fi
;;
set)
    CONTENT="$(cat)"
    echo -n "$CONTENT" | iconv -f utf-8 -t utf-16le | clip.exe
    echo -n "$CONTENT"
;;
"")
    (far2l --clipboard=$(readlink -f $0) >/dev/null 2>&1 &)
;;
esac
