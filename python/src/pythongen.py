#!/usr/bin/env vpython3
import sys
import os
import re
import pcpp
import io
import cffi
import ctypes

source = sys.argv[1]
target = os.path.join(sys.argv[2], "far2lcffi.py")

cpp = pcpp.Preprocessor()
cpp.add_path('/usr/include')
data = """\
#include <limits.h>
TARGET_PATH_MAX=PATH_MAX
TARGET_NAME_MAX=NAME_MAX
"""
cpp.parse(data)
fp = io.StringIO()
cpp.write(fp)
data = fp.getvalue()
PATH_MAX=re.findall('TARGET_PATH_MAX\=(\d+)', data)[0]
NAME_MAX=re.findall('TARGET_NAME_MAX\=(\d+)', data)[0]

cpp = pcpp.Preprocessor()
cpp.add_path(source)
cpp.add_path(os.path.join(source, "far2l", "far2sdk"))
cpp.define("FAR_USE_INTERNALS 1")
# cpp.define('WINPORT_DIRECT')
cpp.define("WINPORT_REGISTRY")
cpp.define("UNICODE")
cpp.define("PROCPLUGINMACROFUNC")
cpp.define("__GNUC__")
if ctypes.sizeof(ctypes.c_int) == ctypes.sizeof(ctypes.c_void_p):
    # 32 bit platform
    pass
else:
    # 64 bit platform
    cpp.define("__LP64__")
cpp.define("_FAR_NO_NAMELESS_UNIONS")
cpp.define("_FILE_OFFSET_BITS 64")
cpp.define("FAR_PYTHON_GEN 1")
data = """\
#define PATH_MAX %s
#define NAME_MAX %s
#define uid_t uint32_t
#define gid_t uint32_t
#include "farplug-wide.h"
#include "farcolor.h"
#include "farkeys.h"
""" %(PATH_MAX, NAME_MAX)
cpp.parse(data)
fp = io.StringIO()
cpp.write(fp)

data = fp.getvalue()
data = data.replace("'['", "0x5B")
data = data.replace("']'", "0x5D")
data = data.replace("','", "0x2C")
data = data.replace("'\"'", "0x22")
data = data.replace("'.'", "0x2E")
data = data.replace("'/'", "0x2F")
data = data.replace("':'", "0x3A")
data = data.replace("';'", "0x3B")
data = data.replace("'\\\\'", "0x5C")
data = data.replace("#line", "//#line")
data = data.replace("#pragma", "//#pragma")

# if preprocessor fails then far2l.py is not generated
pydata = (
    '''\
## This file is autogenerated

far2l = """
'''
    + data
    + '''\
"""
import cffi

ffi = cffi.FFI()
ffi.cdef(far2l, packed=True)
del far2l
'''
)
open(target, "wt").write(pydata)

# import generated file as real plugin does
try:
    sys.path.insert(1, sys.argv[2])
    from far2lcffi import ffi, cffi
except:
    os.unlink(target)
    raise
