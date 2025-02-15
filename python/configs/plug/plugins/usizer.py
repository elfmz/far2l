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
        msg2name = {}
        for name in dir(self.ffic):
            if name[:3] == 'DN_':
                msg2name[getattr(self.ffic, name)] = name
        SilentMsg = (
            self.ffic.DN_ENTERIDLE,
            self.ffic.DN_KILLFOCUS,
            self.ffic.DN_GOTFOCUS,
            self.ffic.DN_DRAWDIALOG,
            self.ffic.DN_CTLCOLORDIALOG,
            self.ffic.DN_DRAWDLGITEM,
            self.ffic.DN_DRAWDIALOGDONE,
            self.ffic.DN_CTLCOLORDLGITEM,
            self.ffic.DN_CTLCOLORDLGLIST
        )

        def DialogProc(hDlg, Msg, Param1, Param2):
            if Msg not in SilentMsg:
                log.debug(f'dlg: msg={Msg}/{msg2name.get(Msg)} Param1={Param1} Param2={Param2}')
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
                if Param1 == dlg.ID_vlist:
                    return 1
                elif Param1 == dlg.ID_vcombo:
                    return 1
            return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)

        @self.ffi.callback("FARWINDOWPROC")
        def _DialogProc(hDlg, Msg, Param1, Param2):
            try:
                return DialogProc(hDlg, Msg, Param1, Param2)
            except:
                log.exception('dialogproc')
                return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)

        b = DialogBuilder(
            self,
            _DialogProc,
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
                    RADIOBUTTON("vr1", "rb1", flags=self.ffic.DIF_GROUP),
                    RADIOBUTTON("vr2", "rb2"),
                    RADIOBUTTON("vr3", "rb3", flags=self.ffic.BSTATE_CHECKED),
                    #                    TEXT(None, "X"),
                ),
                HSizer(
                    LISTBOX(
                        "vlist", 1, "listbox A", "listbox B", "listbox C", "listbox D"
                    ),
                    COMBOBOX(
                        "vcombo", 2, "combo A", "combo B", "combo C", "combo D"
                    ),
                    VSizer(
                        CHECKBOX("vc1", "cb1"),
                        CHECKBOX("vc2", "cb2"),
                        CHECKBOX("vc3", "cb3", flags=self.ffic.BSTATE_CHECKED),
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
        dlg = b.build(-1, -1)

        res = self.info.DialogRun(dlg.hDlg)
        log.debug(
            """\
ok={}
a path=[{}] \
b path=[{}] \
allow={} \
pass=[{}] \
seconds=[{}] \
radio1={} radio2={} radio3={} \
checkbox1={} checkbox2={} checkbox3={} \
vlist={} \
vcombo={}
{}={} {}={} {}={} {}={} {}={} {}={} {}={} {}={} {}={} {}={} {}={} {}={} {}={} {}={} {}={}
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
                "dlg.ID_vapath", dlg.ID_vapath,
                "dlg.ID_vbpath", dlg.ID_vbpath,
                "dlg.ID_vallow", dlg.ID_vallow,
                "dlg.ID_vuserpass", dlg.ID_vuserpass,
                "dlg.ID_vseconds", dlg.ID_vseconds,
                "dlg.ID_vr1", dlg.ID_vr1,
                "dlg.ID_vr2", dlg.ID_vr2,
                "dlg.ID_vr3", dlg.ID_vr3,
                "dlg.ID_vc1", dlg.ID_vc1,
                "dlg.ID_vc2", dlg.ID_vc2,
                "dlg.ID_vc3", dlg.ID_vc3,
                "dlg.ID_vlist", dlg.ID_vlist,
                "dlg.ID_vcombo", dlg.ID_vcombo,
                "dlg.ID_vok", dlg.ID_vok,
                "dlg.ID_vcancel", dlg.ID_vcancel,
            )
        )
        self.info.DialogFree(dlg.hDlg)
