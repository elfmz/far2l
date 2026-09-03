import os
import sys
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
        refs = {
            'Load':   self.s2f("Load  "),
            'Unload': self.s2f("Unload"),
        }
        @self.ffi.callback("FARWINDOWPROC")
        def DialogProc(hDlg, Msg, Param1, Param2):
            if Msg == self.ffic.DN_INITDIALOG:
                for name, value in prev.items():
                    ctrl = getattr(dlg, f'ID_{name}_{value.prevstate}')
                    dlg.SetCheck(ctrl, self.ffic.BSTATE_CHECKED)
                self.Redraw(dlg)
            elif Msg == self.ffic.DN_BTNCLICK and Param1 not in (dlg.ID_vcancel, dlg.ID_vok):
                for name, value in prev.items():
                    if value.bnload == Param1:
                        log.debug(f'OnButton: {Param1} {Param2}')
                        if name in sys.modules:
                            _pluginmanager.pluginRemove(name)
                            s = 'Load'
                        else:
                            autorun = dlg.GetCheck(getattr(dlg, f'ID_{name}_open')) != 0
                            _pluginmanager.pluginInstall(name, autorun)
                            s = 'Unload'
                        dlg.SetText(value.bnload, refs[s])
                        return True
                return True
            return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)

        cvt = {
            False: "Load  ",
            True:  "Unload",
        }

        class Prev:
            def __init__(self, name, prevstate):
                self.name = name
                self.prevstate = prevstate
                self.bnload = -1

        elements = []
        iniFileName = os.path.join(self.USERHOME, "plugins.ini")
        prev = {}
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
                        prev[name] = Prev(name, option)
                        elements.append(
                            HSizer(
                                TEXT(None, ('%%-%d.%ds'%(nmax, nmax))%name),
                                RADIOBUTTON(name+"_register", "register", flags=self.ffic.DIF_GROUP),
                                RADIOBUTTON(name+"_load", "load"),
                                RADIOBUTTON(name+"_open", "open"),
                                BUTTON(name+"_button", cvt[name in sys.modules]),
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

        for name in prev.keys():
            prev[name].bnload = getattr(dlg, f'ID_{name}_button')
        
        res = self.info.DialogRun(dlg.hDlg)
        if res == dlg.ID_vok:
            ini = configparser.ConfigParser()
            ini.add_section('autoload')
            ini.add_section('plugins')
            for name in sorted(prev.keys()):
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

    def OpenPlugin(self, OpenFrom, Item):
        self.Configure()
