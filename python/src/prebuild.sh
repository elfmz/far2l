#!/bin/bash
set -e
#echo "XBUILD=$*"

SRC="$1"
DST="$2"
PYTHON="$3"
PREPROCESSOR="$4 -E -P -xc"

mkdir -p "$DST/incpy"

if [ ! -f "$DST/python/.prepared" ]; then
	echo "Preparing python virtual env at $DST/python using $PYTHON"
	mkdir -p "$DST/python"
	$PYTHON -m venv --system-site-packages "$DST/python"
	"$DST/python/bin/python" -m pip install --upgrade pip || true
	"$DST/python/bin/python" -m pip install --ignore-installed cffi debugpy pcpp adbutils
	$PREPROCESSOR "$SRC/python/src/consts.gen" | sh > "${DST}/incpy/consts.h"

	echo "1" > "$DST/python/.prepared"
fi

###################

cp -f -R \
	"$SRC/python/configs/plug/far2l/"* \
	"$DST/incpy/"

"$DST/python/bin/python" "$SRC/python/src/pythongen.py" "${SRC}" "${DST}/incpy"
