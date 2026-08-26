import os
import zlib
import hashlib
import logging
from far2l.plugin import PluginBase
from far2l import far2leditor
from far2l.fardialogbuilder import (
    TEXT,
    EDIT,
    BUTTON,
    HLine,
    HSizer,
    VSizer,
    DialogBuilder,
)

log = logging.getLogger(__name__)
progs = {}
for name in sorted(hashlib.algorithms_available):
    progs[name] = hashlib.new(name)


class Plugin(PluginBase):
    label = "Python Hashes"
    openFrom = ["PLUGINSMENU", "COMMANDLINE", "EDITOR", "VIEWER"]

    def OpenPlugin(self, OpenFrom, Item):
        # 5=edit, 6=viewer, 1=panel
        if OpenFrom == 5:
            fqname = far2leditor.Editor(self).GetFileName()
        elif OpenFrom == 6:
            data = self.ffi.new("struct ViewerInfo *")
            data.StructSize = self.ffi.sizeof("struct ViewerInfo")
            self.info.ViewerControl(self.ffic.VCTL_GETINFO, data)
            fqname = self.f2s(data.FileName)
        elif OpenFrom == 1:
            pnli, pnlidata = self.panel.GetCurrentPanelItem()
            fname = self.f2s(pnli.FindData.lpwszFileName)
            fqname = os.path.join(self.panel.GetPanelDir(), fname)
        else:
            log.debug(f"unsupported open from {OpenFrom}")
            return
        try:
            self.Dialog(fqname)
        except Exception as ex:
            log.exception('run')

    def Calculate(self, fname):
        result = {}
        with open(fname, 'rb') as fp:
            ha = 1
            hc = 0
            b = fp.read(1024*1024)
            while b:
                ha = zlib.adler32(b, ha)
                hc = zlib.crc32(b, hc)
                for name, prog in progs.items():
                    prog.update(b)
                b = fp.read(1024)
            fp.close()

        result = {
            'adler32': (8, f'{ha:08x}'),
            'crc32': (8, f'{hc:08x}'),
        }
        for name in sorted(progs):
            prog = progs[name]
            if name[:5] == 'shake':
                s = prog.hexdigest(40)
            else:
                s = prog.hexdigest()
            result[name] = (len(s), s)
        return result

    def Dialog(self, fqname):
        hashes = self.Calculate(fqname)
        def DialogProc(hDlg, Msg, Param1, Param2):
            if Msg == self.ffic.DN_INITDIALOG:
                dlg.SetText(dlg.ID_fqname, fqname)
                for name in sorted(hashes):
                    hlen, hhash = hashes[name]
                    dlgid = getattr(dlg, f'ID_{name}')
                    dlg.SetText(dlgid, hhash)
            elif Msg == self.ffic.DN_KEY:
                if Param2 == self.ffic.KEY_F1:
                    fn = os.path.normpath(__file__)
                    dn = os.path.dirname(fn)
                    bn = os.path.splitext(os.path.basename(fn))[0]
                    nn = os.path.join(dn, 'help', bn, bn)+'.hlp'
                    self.info.ShowHelp(self.s2f(nn), self.ffi.NULL, 0)
                    return 1
            return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)

        @self.ffi.callback("FARWINDOWPROC")
        def _DialogProc(hDlg, Msg, Param1, Param2):
            try:
                return DialogProc(hDlg, Msg, Param1, Param2)
            except:
                log.exception('dialogproc')
                return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)

        body = [
            HSizer(
                TEXT(None, "File"),
                EDIT("fqname", 40),
            ),
        ]
        nlen = 0
        for name in sorted(hashes):
            nlen = max(nlen, len(name)+1)
        for name in sorted(hashes):
            hlen, hhash = hashes[name]
            body.append(
                HSizer(
                    TEXT(None, name.ljust(nlen)),
                    EDIT(name, 60),
                ),
            )
        body.extend([
            HLine(),
            HSizer(
                BUTTON("vok", "OK", default=True, flags=self.ffic.DIF_CENTERGROUP),
                BUTTON("vcancel", "Cancel", flags=self.ffic.DIF_CENTERGROUP),
            ),
        ])
        b = DialogBuilder(
            self,
            _DialogProc,
            "Python hashes",
            "helptopic",
            0,
            VSizer(*body),
        )
        dlg = b.build(-1, -1)

        res = self.info.DialogRun(dlg.hDlg)
        self.info.DialogFree(dlg.hDlg)
