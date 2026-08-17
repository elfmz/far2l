import logging
from far2l.plugin import PluginBase
from far2l.fardialogbuilder import (
    TEXT,
    EDIT,
    BUTTON,
    HLine,
    HSizer,
    VSizer,
    DialogBuilder,
    #
    Dialog,
)

log = logging.getLogger(__name__)


class Plugin(PluginBase):
    label = "Python Stealer"
    openFrom = ["PLUGINSMENU", "DIALOG"]

    def OpenPlugin(self, OpenFrom, Item):
        ptr = self.ffi.cast("struct OpenDlgPluginData *", Item)
        fDlg = Dialog(self)
        fDlg.hDlg = ptr.hDlg

        def DialogProc(hDlg, Msg, Param1, Param2):
            if Msg == self.ffic.DN_INITDIALOG:
                dlg.SetText(dlg.ID_vpostision, "")
                dlg.SetText(dlg.ID_vvalue, "")
            elif Msg == self.ffic.DN_BTNCLICK:
                if dlg.ID_vsteal == Param1:
                    lp = dlg.GetText(dlg.ID_vpostision)
                    try:
                        lp = int(lp)
                    except:
                        lp = 0
                    text = fDlg.GetText(lp)
                    if not isinstance(text, str):
                        text = str(text)
                    elif not text:
                        text = ""
                    dlg.SetText(dlg.ID_vvalue, text[:20])
                    dlg.SetFocus(dlg.ID_vpostision)
                    return True
                elif  Param1 not in (dlg.ID_vok, dlg.ID_vcancel):
                    return True
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
            "Python stealer",
            "helptopic",
            0,
            VSizer(
                HSizer(
                    TEXT(None, "Position:"),
                    EDIT("vpostision", 4, maxlength=4),
                ),
                HSizer(
                    TEXT(None, "Value   :"),
                    TEXT("vvalue", "*"*20),
                ),
                HLine(),
                BUTTON("vsteal", "Read", default=True, flags=self.ffic.DIF_CENTERGROUP),
                HLine(),
                HSizer(
                    BUTTON("vok", "OK", flags=self.ffic.DIF_CENTERGROUP),
                    BUTTON("vcancel", "Cancel", flags=self.ffic.DIF_CENTERGROUP),
                ),
            ),
        )
        dlg = b.build(-1, -1)

        res = self.info.DialogRun(dlg.hDlg)
        self.info.DialogFree(dlg.hDlg)
