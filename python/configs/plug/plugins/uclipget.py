import os
import sys
import logging

import ctypes as ct
import far2lc
from far2l.plugin import PluginBase
from yfar import FarPlugin

log = logging.getLogger(__name__)

class Plugin(FarPlugin):
    label = "Python Clip GET"
    openFrom = ["PLUGINSMENU", 'FILEPANEL']

    def CopyFiles(self, data):
        pwd = self.get_panel().directory
        data = data.decode('utf8')
        log.debug(f'copyfiles: {data}')
        prefix = 'file://'
        for uri in data.split('\n'):
            if uri[:len(prefix)] != prefix:
                continue
            fqname = uri[len(prefix):]
            fname = fqname.split('/')[-1]
            dqname = os.path.join(pwd, fname)
            log.debug(f'CopyFile: {fqname} -> {dqname}')
            with open(dqname, 'wb') as fo:
                with open(fqname, 'rb') as fi:
                    rec = fi.read(4096)
                    if not rec:
                        break
                    fo.write(rec)

    def OpenPlugin(self, OpenFrom):
        winport = self.ffi.cast("struct WINPORTDECL *", far2lc.WINPORT())
        clipurifmt = winport.RegisterClipboardFormat("text/uri-list")
        if not clipurifmt:
            log.error('uclipset.ClipboardRegisterFormat.1')
            return
        clipgnofmt = winport.RegisterClipboardFormat("x-special/gnome-copied-files")
        if not clipgnofmt:
            log.error('uclipset.ClipboardRegisterFormat.2')
            return
        if not winport.OpenClipboard(self.ffi.NULL):
            log.error('uclipset.OpenClipboard')
            return
        try:
            data = winport.GetClipboardData(clipurifmt)
            if data is not None:
                nb = winport.ClipboardSize(data)
                result = self.ffi.buffer(data, nb-1)
                self.CopyFiles(bytes(result))
            else:
                data = winport.GetClipboardData(clipgnofmt)
                if data is not None:
                    nb = winport.ClipboardSize(data)
                    result = self.ffi.buffer(data, nb-1)
                    self.CopyFiles(bytes(result))
        except:
            log.exception('uclipset.GetClipboardData')
        finally:
            if not winport.CloseClipboard():
                log.error('uclipset.CloseClipboard')
                return
