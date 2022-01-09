import logging
from far2l.plugin import PluginBase
from far2l.fardialogbuilder import (
    Spacer,
    TEXT, EDIT, PASSWORD, MASKED, MEMOEDIT,
    BUTTON, CHECKBOX, RADIOBUTTON, COMBOBOX,
    LISTBOX, USERCONTROL,
    HLine,
    HSizer, VSizer,
    DialogBuilder
)

log = logging.getLogger(__name__)


class Plugin(PluginBase):
    label = "Python Dialog Demo"
    openFrom = ["PLUGINSMENU", "COMMANDLINE", "EDITOR", "VIEWER"]

    def OpenPlugin(self, OpenFrom):
        @self.ffi.callback("FARWINDOWPROC")
        def DialogProc(hDlg, Msg, Param1, Param2):
            if Msg == self.ffic.DN_INITDIALOG:
                log.debug('INITDIALOG')
                try:
                    dlg.SetText(dlg.ID_vapath, "vapath initial")
                    dlg.SetText(dlg.ID_vbpath, "vbpath initial")
                    dlg.Disable(dlg.ID_vbpath)
                    dlg.SetCheck(dlg.ID_vallow, 1)
                    dlg.SetFocus(dlg.ID_vseconds)
                except:
                    log.exception('bang')
                log.debug('/INITDIALOG')
            elif Msg == self.ffic.DN_BTNCLICK:
                pass
            elif Msg == self.ffic.DN_KEY:
                if Param2 == self.ffic.KEY_LEFT:
                    pass
                elif Param2 == self.ffic.KEY_UP:
                    pass
                elif Param2 == self.ffic.KEY_RIGHT:
                    pass
                elif Param2 == self.ffic.KEY_DOWN:
                    pass
                elif Param2 == self.ffic.KEY_ENTER:
                    pass
                elif Param2 == self.ffic.KEY_ESC:
                    pass
            elif Msg == self.ffic.DN_MOUSECLICK:
                pass
            return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)

        b = DialogBuilder(
            self,
            DialogProc,
            "Python dialog",
            "helptopic",
            0,
            VSizer(
                HSizer(TEXT("a path"), Spacer(), EDIT("vapath", 36, 40)),
                Spacer(),
                HSizer(TEXT("b path"), Spacer(), EDIT("vbpath", 20, 30)),
                HLine(),
                HSizer(
                    CHECKBOX('vallow', "Allow"),
                    Spacer(),
                    TEXT("Password"),
                    Spacer(),
                    PASSWORD("vuserpass", 8, 15),
                    Spacer(),
                    TEXT("Seconds"),
                    Spacer(),
                    MASKED("vseconds", "9999"),
                ),
                #MEMOEDIT("vmemo", 40, 5, 512),
                HLine(),
                HSizer(
                    RADIOBUTTON('vp1', "p1", True, flags=self.ffic.DIF_GROUP),
                    RADIOBUTTON('vp2', "p2"),
                    RADIOBUTTON('vp3', "p3"),
                ),
                HSizer(
                    RADIOBUTTON('vr1', "r1", True, flags=self.ffic.DIF_GROUP),
                    RADIOBUTTON('vr2', "r2"),
                    RADIOBUTTON('vr3', "r3"),
                ),
                HSizer(
                    LISTBOX("vlist", 1, "element A", "element B", "element C", "element D"),
                    Spacer(),
                    COMBOBOX("vcombo", 2, "element A", "element B", "element C", "element D"),
                    Spacer(),
                    COMBOBOX("vcombo1", 3, "element A", "element B", "element C", "element D"),
                ),
                HLine(),
                HSizer(
                    BUTTON('vok', "OK", True, flags=self.ffic.DIF_CENTERGROUP),
                    BUTTON('vcancel', "Cancel", flags=self.ffic.DIF_CENTERGROUP),
                ),
            ),
        )
        dlg = b.build(-1, -1)

        res = self.info.DialogRun(dlg.hDlg)
        self.info.DialogFree(dlg.hDlg)
