#!/usr/bin/env vpython3
import sys
import os
import cffi

source = sys.argv[1]
target = os.path.join(sys.argv[2], "_pyfar.cpp")

far2l = os.path.join(source, '..')
far2lp = os.path.join(source, 'configs', 'plug', 'far2l')
ffi = cffi.FFI()

xpre = """
//#define FAR_DONT_USE_INTERNALS
#define FAR_USE_INTERNALS
#define PROCPLUGINMACROFUNC 1

#define UNICODE
#define WINPORT_DIRECT
#define WINPORT_REGISTRY
//#define WXUSINGDLL
#define _FILE_OFFSET_BITS 64
//#define __WXGTK__

"""

ffi.set_source("_pyfar",  # name of the output C extension
    #xpre + 
"""
#include "plugin.hpp"
#include "farcolor.hpp"
#include "farkeys.hpp"
""",
    include_dirs=[
        os.path.join(far2l),
        os.path.join(far2l, 'far2l', 'fpsdk'),
        os.path.join(far2l, 'WinPort'),
    ],
)
for fname in ("farwin.h", "farcolor.hpp", "farkeys.hpp", "plugin.hpp"):
    fqname = os.path.join(far2lp, fname)
    data = open(fqname, "rt", encoding="utf-8").read()
    ffi.cdef(data, packed=True)

if 0:
    ffi.compile(
        verbose=True,
        call_c_compiler=False
    )
else:
    ffi.emit_c_code(target)
