#!/bin/sh

script_path=$(dirname "$(readlink -f "$0")")

case "$1" in
get)
    if command -v cscript.exe >/dev/null 2>&1; then
        cscript.exe //Nologo "$script_path"\\wslgclip.vbs
    else
        powershell.exe -Command Get-Clipboard
    fi

;;
set)
    CONTENT=$(cat)
    echo "$CONTENT" | clip.exe
    echo "$CONTENT"
;;
"")
    (far2l --clipboard=$(readlink -f $0) >/dev/null 2>&1 &)
;;
esac

