#!/bin/bash
PREPROCESSOR=$1
SRC=$2
DST=$3
${PREPROCESSOR} -E -P -xc \
-I"${SRC}/far2l/far2sdk" -I"${SRC}/WinPort" "${SRC}/python/src/far2lcffi.h" \
> "${DST}/Plugins/python/plug/far2l/far2lcffi.h"
