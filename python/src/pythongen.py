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

with open(target, 'wt') as fp:
    fp.write('''\
data = """\\
''')
    fp.write('''\
#define RIGHT_ALT_PRESSED     0x0001 // the right alt key is pressed.
#define LEFT_ALT_PRESSED      0x0002 // the left alt key is pressed.
#define RIGHT_CTRL_PRESSED    0x0004 // the right ctrl key is pressed.
#define LEFT_CTRL_PRESSED     0x0008 // the left ctrl key is pressed.
#define SHIFT_PRESSED         0x0010 // the shift key is pressed.
#define NUMLOCK_ON            0x0020 // the numlock light is on.
#define SCROLLLOCK_ON         0x0040 // the scrolllock light is on.
#define CAPSLOCK_ON           0x0080 // the capslock light is on.
#define ENHANCED_KEY          0x0100 // the key is enhanced.

#define FROM_LEFT_1ST_BUTTON_PRESSED    0x0001
#define RIGHTMOST_BUTTON_PRESSED        0x0002
#define FROM_LEFT_2ND_BUTTON_PRESSED    0x0004
#define FROM_LEFT_3RD_BUTTON_PRESSED    0x0008
#define FROM_LEFT_4TH_BUTTON_PRESSED    0x0010

#define MOUSE_MOVED   0x0001
#define DOUBLE_CLICK  0x0002
#define MOUSE_WHEELED 0x0004
#define MOUSE_HWHEELED 0x0008

#define KEY_EVENT         0x0001 // Event contains key event record
#define MOUSE_EVENT       0x0002 // Event contains mouse event record
#define WINDOW_BUFFER_SIZE_EVENT 0x0004 // Event contains window change event record
#define MENU_EVENT 0x0008 // Event contains menu event record
#define FOCUS_EVENT 0x0010 // event contains focus change
#define BRACKETED_PASTE_EVENT 0x0020 // event contains bracketed paste state change
#define CALLBACK_EVENT 0x0040 // callback to be invoked when its record dequeued, its translated into NOOP_EVENT when invoked
#define NOOP_EVENT 0x0080 // nothing interesting, typically injected to kick events dispatcher
''')
    fp.write(data)
    fp.write('''\
"""
''')
    fp.write('''\
import cffi
ffi = cffi.FFI()
pack = None
while True:
    i = data.find('//#pragma pack')
    if i < 0:
        ffi.cdef(data, pack=pack)
        break
    ndata = data[:i]
    ffi.cdef(ndata, pack=pack)
    data = data[i:]
    i = data.find(')')
    s = data[:i+1]
    if s.find('()') > 0:
        pack = None
    else:
        pack = 2
    data = data[i+1:]
del pack, data, i, ndata, s
''')
