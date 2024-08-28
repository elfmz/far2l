#!/bin/bash
# workaround for https://github.com/elfmz/far2l/issues/1658

script_path=$(dirname "$(readlink -f "$0")")

case "$1" in
get)
    cscript.exe //Nologo //u $(wslpath -w "$script_path"/wslgclip.vbs) | iconv -f utf-16le -t utf-8

#   powershell method is slower, also it have unsolved charset problems. disabled for now
#   possible solution for charset problem:
#   https://github.com/microsoft/terminal/issues/280#issuecomment-1728298632

#    if command -v cscript.exe >/dev/null 2>&1; then
#        cscript.exe //Nologo //u $(wslpath -w "$script_path"/wslgclip.vbs) | iconv -f utf-16le -t utf-8
#    else
#        powershell.exe -Command Get-Clipboard
#    fi
;;
set)
    # shell removes tailing newlines. we should take care of it
    CONTENT=$(cat; echo -n .)
    CONTENT=${CONTENT%.}
    echo -n "$CONTENT" | iconv -f utf-8 -t utf-16le | clip.exe
    echo -n "$CONTENT"
;;
"")
    (far2l --clipboard=$(readlink -f $0) >/dev/null 2>&1 &)
;;
esac
