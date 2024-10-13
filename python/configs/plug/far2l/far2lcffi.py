import os
import re

import cffi
ffi = cffi.FFI()

__all__ = ["ffi"]

def convert():
    with open(os.path.join(os.path.dirname(__file__), 'far2lcffi.h'), 'rt') as fp:
        data = fp.read()

    for s in re.findall("'.+'", data):
        i = str(ord(s[-2].encode("ascii")))
        data = data.replace(s, i)

    pack = None
    while True:
        i = data.find('#pragma pack')
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

convert()
