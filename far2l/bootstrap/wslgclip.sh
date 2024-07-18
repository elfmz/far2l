#!/bin/sh
# workaround for https://github.com/elfmz/far2l/issues/1658

script_path=$(dirname "$(readlink -f "$0")")

case "$1" in
get)
    if command -v cscript.exe >/dev/null 2>&1; then
        cscript.exe //Nologo $(wslpath -w "$script_path"/wslgclip.vbs)
    else
    	powershell.exe -Command "\$OutputEncoding = [System.Text.Encoding]::UTF8; Get-Clipboard -TextFormatType UnicodeText"
    fi
;;
set)
    CONTENT=$(cat)
    echo -n "$CONTENT" | iconv -f utf-8 -t utf-16le | clip.exe
    echo -n "$CONTENT"
;;
"")
    (far2l --clipboard=$(readlink -f $0) >/dev/null 2>&1 &)
;;
esac
