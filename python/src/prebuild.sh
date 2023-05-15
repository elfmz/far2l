#!/bin/bash
set -e
echo "XBUILD=$*"

SRC="$1"
DST="$2"
VIRTUAL_PYTHON="$3"
PYTHON="$4"
PREPROCESSOR="$5 -E -P -xc"

mkdir -p "$DST/incpy"

if [ "$VIRTUAL_PYTHON" == "0" ]; then
    if [ ! -f "$DST/python/.prepared" ]; then
        echo "Preparing python virtual env at $DST/python using $PYTHON"
        mkdir -p "$DST/python"
        $PYTHON -m venv --system-site-packages "$DST/python"
        "$DST/python/bin/python" -m pip install --upgrade pip || true
        "$DST/python/bin/python" -m pip install --ignore-installed debugpy pcpp adbutils
        "$DST/python/bin/python" -m pip install --force-reinstall --no-binary :all: cffi
        $PREPROCESSOR "$SRC/python/src/consts.gen" | sh > "${DST}/incpy/consts.h"
        echo "1" > "$DST/python/.prepared"
    fi
else
    if [ ! -f "$DST/.prepared" ]; then
        $PREPROCESSOR "$SRC/python/src/consts.gen" | sh > "${DST}/incpy/consts.h"
        echo "1" > "$DST/.prepared"
    fi
fi

###################

cp -f -R \
	"$SRC/python/configs/plug/far2l/"* \
	"$DST/incpy/"

if [ "$VIRTUAL_PYTHON" == "0" ]; then
    "$DST/python/bin/python" "$SRC/python/src/pythongen.py" "${SRC}" "${DST}/incpy"
else
    "$PYTHON" "$SRC/python/src/pythongen.py" "${SRC}" "${DST}/incpy"
fi
