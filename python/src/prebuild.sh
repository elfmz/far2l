#!/bin/bash
set -e

SRC="$1"
DST="$2"
PREPROCESSOR="$3 -E -P -xc"

echo XXX $PREPROCESSOR -I"$SRC/far2l/far2sdk" -I"$SRC/WinPort" "$SRC/python/src/far2lcffi.h" "$DST"
$PREPROCESSOR -I"$SRC/far2l/far2sdk" -I"$SRC/WinPort" "$SRC/python/src/far2lcffi.h" > "$DST"
cat "$SRC/python/src/far2lcffidefs.h" >> "$DST"
