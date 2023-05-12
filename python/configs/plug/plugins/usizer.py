if 0:
    import debugpy

    debugpy.listen(("127.0.0.1", 5678))
    debugpy.wait_for_client()

import logging
from far2l.plugin import PluginBase
from far2l.fardialogbuilder import (
    Spacer,
    TEXT,
    EDIT,
    PASSWORD,
    MASKED,
    MEMOEDIT,
    BUTTON,
    CHECKBOX,
    RADIOBUTTON,
    COMBOBOX,
    LISTBOX,
    USERCONTROL,
    HLine,
    HSizer,
    VSizer,
    DialogBuilder,
)

log = logging.getLogger(__name__)


class Plugin(PluginBase):
    label = "Python Dialog Demo"
    openFrom = ["PLUGINSMENU", "COMMANDLINE", "EDITOR", "VIEWER"]

    def OpenPlugin(self, OpenFrom):
        @self.ffi.callback("FARWINDOWPROC")
        def DialogProc(hDlg, Msg, Param1, Param2):
            if Msg == self.ffic.DN_INITDIALOG:
                try:
                    dlg.SetText(dlg.ID_vapath, "vapath initial")
                    dlg.SetText(dlg.ID_vbpath, "vbpath initial")
                    dlg.Disable(dlg.ID_vbpath)
                    dlg.SetCheck(dlg.ID_vallow, 1)
                    dlg.SetFocus(dlg.ID_vseconds)
                    # override value from constructor
                    dlg.SetCheck(dlg.ID_vr1, self.ffic.BSTATE_CHECKED)
                    dlg.SetCheck(dlg.ID_vc3, self.ffic.BSTATE_CHECKED)
                except:
                    log.exception("bang")
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
                HSizer(
                    TEXT(None, "a path"),
                    EDIT("vapath", 33, maxlength=40),
                    #                    TEXT(None, "X"),
                ),
                HSizer(
                    TEXT(None, "b path"),
                    EDIT("vbpath", 20),
                    #                    TEXT(None, "X"),
                ),
                HLine(),
                HSizer(
                    CHECKBOX("vallow", "Allow"),
                    TEXT(None, "Password"),
                    PASSWORD("vuserpass", 8, maxlength=15),
                    TEXT(None, "Seconds"),
                    MASKED("vseconds", "9999"),
                    #                    TEXT(None, "X"),
                ),
                # MEMOEDIT("vmemo", 40, 5, 512),
                HLine(),
                HSizer(
                    RADIOBUTTON("vr1", "p1", flags=self.ffic.DIF_GROUP),
                    RADIOBUTTON("vr2", "p2"),
                    RADIOBUTTON("vr3", "p3", flags=self.ffic.BSTATE_CHECKED),
                    #                    TEXT(None, "X"),
                ),
                HSizer(
                    LISTBOX(
                        "vlist", 1, "element A", "element B", "element C", "element D"
                    ),
                    COMBOBOX(
                        "vcombo", 2, "element A", "element B", "element C", "element D"
                    ),
                    VSizer(
                        CHECKBOX("vc1", "c1"),
                        CHECKBOX("vc2", "c2"),
                        CHECKBOX("vc3", "c3", flags=self.ffic.BSTATE_CHECKED),
                    ),
                    #                    TEXT(None, "X"),
                ),
                HLine(),
                HSizer(
                    BUTTON("vok", "OK", default=True, flags=self.ffic.DIF_CENTERGROUP),
                    BUTTON("vcancel", "Cancel", flags=self.ffic.DIF_CENTERGROUP),
                ),
            ),
        )
        # debugpy.breakpoint()
        dlg = b.build(-1, -1)

        res = self.info.DialogRun(dlg.hDlg)
        log.debug(
            """\
ok={} \
a path=[{}] \
b path=[{}] \
allow={} \
pass=[{}] \
seconds=[{}] \
radio1={} \
radio2={} \
radio3={} \
checkbox1={} \
checkbox2={} \
checkbox3={} \
vlist={} \
vcombo={} \
""".format(
                res == dlg.ID_vok,
                dlg.GetText(dlg.ID_vapath),
                dlg.GetText(dlg.ID_vbpath),
                dlg.GetCheck(dlg.ID_vallow),
                dlg.GetText(dlg.ID_vuserpass),
                dlg.GetText(dlg.ID_vseconds),
                dlg.GetCheck(dlg.ID_vr1),
                dlg.GetCheck(dlg.ID_vr2),
                dlg.GetCheck(dlg.ID_vr3),
                dlg.GetCheck(dlg.ID_vc1),
                dlg.GetCheck(dlg.ID_vc2),
                dlg.GetCheck(dlg.ID_vc3),
                dlg.GetCurPos(dlg.ID_vlist),
                dlg.GetCurPos(dlg.ID_vcombo),
            )
        )
        self.info.DialogFree(dlg.hDlg)
