#!/usr/bin/env vpython3
import sys
import os
import cffi

source = os.path.join(sys.argv[1], "staging")
target = os.path.join(sys.argv[1], "staging", "_pyfar.cpp")

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
#include "farwin.h"
#include "farplug-wide.h"
#include "farcolor.h"
#include "farkeys.h"
""",
    include_dirs=[
        os.path.join(source)
    ],
)
for fname in ("farwin.h", "farcolor.h", "farkeys.h", "farplug-wide.h"):
    fqname = os.path.join(source, fname)
    data = open(fqname, "rt", encoding="utf-8").read()
    ffi.cdef(data, packed=True)

if 0:
    ffi.compile(
        verbose=True,
        call_c_compiler=False
    )
else:
    ffi.emit_c_code(target)
