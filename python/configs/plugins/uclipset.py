import os
import logging

import ctypes as ct
import far2lc
from far2l.plugin import PluginBase

log = logging.getLogger(__name__)

CAH_ALLOCATED_MAGIC = 0x0610ba10A110CED0
CAH_FREED_MAGIC     = 0x0610ba10F4EED000
class ClipboardAllocHeader(ct.Structure):
    _fields_ = [
		('size', ct.c_uint32),
		('padding', ct.c_uint32),
		('magic', ct.c_uint64),
    ]

class Plugin(PluginBase):
    label = "Python Clip SET"
    openFrom = ["PLUGINSMENU", 'FILEPANEL']

    def SetClipboard(self, winport, fmt, fqname):
        fqname = fqname.encode('utf8')
        #log.debug(f'clip.{fmt} set: {fqname} {len(fqname)}')
        ptr = winport.ClipboardAlloc(len(fqname))
        if ptr is None:
            log.debug('clipboard alloc failed')
        else:
            if 0:
                m = int(self.ffi.cast('DWORD64', ptr))
                log.debug(f'ptr={ptr} m={m:x}')
                m -= ct.sizeof(ClipboardAllocHeader)
                log.debug(f'm={m:x}')
                mp = ClipboardAllocHeader.from_address(m)
                log.debug(f'mp.size={mp.size} mp.padding={mp.padding} mp.magic={mp.magic:x} CAH_ALLOCATED_MAGIC={CAH_ALLOCATED_MAGIC:x}')
            self.ffi.memmove(ptr, fqname, len(fqname)) 
            res = winport.SetClipboardData(fmt, ptr)

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
            files = []
            pwd = self.panel.GetPanelDir()
            for i in range(self.panel.GetPanelInfo().SelectedItemsNumber):
                pnli, pnlidata = self.panel.GetSelectedPanelItem(i)
                fqname = self.f2s(pnli.FindData.lpwszFileName)
                files.append("file://"+os.path.join(pwd, fqname))
            #self.SetClipboard(winport, 1, fqname)
            self.SetClipboard(winport, clipurifmt, "\n".join(files))
            self.SetClipboard(winport, clipgnofmt, "copy\n"+"\n".join(files))
        except:
            log.exception('uclipset.SetClipboard')
        finally:
            if not winport.CloseClipboard():
                log.error('uclipset.CloseClipboard')
                return
        # typedef BOOL (*WINPORT_IsClipboardFormatAvailable) (UINT format);
        # typedef PVOID (*WINPORT_GetClipboardData) (UINT format);
        # typedef SIZE_T (*WINPORT_ClipboardSize) (PVOID mem);
