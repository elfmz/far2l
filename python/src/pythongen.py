#!/usr/bin/env vpython3
import sys
import os
import re
import pcpp
import io
import cffi

source = sys.argv[1]
target = os.path.join(sys.argv[2], "far2lcffi.py")

cpp = pcpp.Preprocessor()
cpp.add_path(source)
cpp.add_path(os.path.join(source, "far2l", "far2sdk"))
cpp.add_path(sys.argv[2])  # to find consts.h
cpp.define("FAR_USE_INTERNALS 1")
# cpp.define('WINPORT_DIRECT')
cpp.define("WINPORT_REGISTRY")
cpp.define("UNICODE")
cpp.define("PROCPLUGINMACROFUNC")
cpp.define("__GNUC__")
cpp.define("_FAR_NO_NAMELESS_UNIONS")
cpp.define("_FILE_OFFSET_BITS 64")
cpp.define("FAR_PYTHON_GEN 1")
data = """\
#define uid_t uint32_t
#define gid_t uint32_t
#include "consts.h"
#include "farplug-wide.h"
#include "farcolor.h"
#include "farkeys.h"
"""

cpp.parse(data)
fp = io.StringIO()
cpp.write(fp)

data = fp.getvalue()
for s in re.findall("'.+'", data):
    t = str(ord(s[-2].encode("ascii")))
    data = data.replace(s, t)
data = data.replace("#line", "//#line")
data = data.replace("#pragma", "//#pragma")

ffi = cffi.FFI()
ffi.set_source('far2lcffi', None)
ffi.cdef(data, packed=True)
ffi.compile(verbose=True)
