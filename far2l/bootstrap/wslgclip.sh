#!/bin/sh
# workaround for https://github.com/elfmz/far2l/issues/1658

script_path=$(dirname "$(readlink -f "$0")")

case "$1" in
get)
	powershell.exe -Command "\$OutputEncoding = [System.Text.Encoding]::UTF8; Get-Clipboard"
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
