import os
import logging
import configparser

import debugpy

from far2l import _pluginmanager
from far2l.plugin import PluginBase
from far2l.farprogress import ProgressMessage
from far2l.fardialogbuilder import (
    TEXT,
    RADIOBUTTON,
    BUTTON,
    HLine,
    HSizer,
    VSizer,
    DialogBuilder,
)

log = logging.getLogger(__name__)


class Plugin(PluginBase):
    label = "Python"
    openFrom = ["PLUGINSMENU"]

    def Configure(self):
        @self.ffi.callback("FARWINDOWPROC")
        def DialogProc(hDlg, Msg, Param1, Param2):
            if Msg == self.ffic.DN_INITDIALOG:
                for name, option in self.prev.items():
                    ctrl = getattr(dlg, f'ID_{name}_{option}')
                    dlg.SetCheck(ctrl, self.ffic.BSTATE_CHECKED)
                self.Redraw(dlg)
            return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)

        elements = []
        iniFileName = os.path.join(self.USERHOME, "plugins.ini")
        self.prev = {}
        if os.path.isfile(iniFileName):
            with open(iniFileName, "rt") as fp:
                ini = configparser.ConfigParser()
                ini.read_file(fp)
                if ini.has_section('plugins'):
                    nmax = 0
                    for name in ini.options('plugins'):
                        nmax = max(len(name), nmax)
                    for name in ini.options('plugins'):
                        option = ini.get('autoload', name, fallback='register')
                        self.prev[name] = option
                        elements.append(
                            HSizer(
                                TEXT(None, ('%%-%d.%ds'%(nmax, nmax))%name),
                                RADIOBUTTON(name+"_register", "register", flags=self.ffic.DIF_GROUP),
                                RADIOBUTTON(name+"_load", "load"),
                                RADIOBUTTON(name+"_open", "open"),
                            ),
                        )

        if len(elements) == 0:
            self.error(['No plugins configured in:', iniFileName, 'OK'], 'Plugin manager')
            return
        elements.extend([
            HLine(),
            HSizer(
                BUTTON("vok", "OK", default=True, flags=self.ffic.DIF_CENTERGROUP),
                BUTTON("vcancel", "Cancel", flags=self.ffic.DIF_CENTERGROUP),
            ),
        ])
        b = DialogBuilder(
            self,
            DialogProc,
            "Python plugins",
            "helptopic",
            0,
            VSizer(*elements)
        )
        # debugpy.breakpoint()
        dlg = b.build(-1, -1)

        res = self.info.DialogRun(dlg.hDlg)
        if res == dlg.ID_vok:
            ini = configparser.ConfigParser()
            ini.add_section('autoload')
            ini.add_section('plugins')
            for name in sorted(self.prev.keys()):
                ini.set('plugins', name, '')
                for option in ('load', 'open'):
                    curr = dlg.GetCheck(getattr(dlg, f'ID_{name}_{option}'))
                    if curr:
                        ini.set('autoload', name, option)
                        break

            try:
                with open(iniFileName, 'w') as fp:
                    ini.write(fp)
            except:
                self.error(['I can\'t save the file:', iniFileName, 'OK'], 'Plugin manager')

        self.info.DialogFree(dlg.hDlg)

    def OpenPlugin(self, OpenFrom):
        self.Configure()
