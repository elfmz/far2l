import logging
from far2l.plugin import PluginBase
import far2l.fardialogbuilder as dlgb


log = logging.getLogger(__name__)


class Plugin(PluginBase):
    label = "Python ED Sort"
    openFrom = ["PLUGINSMENU", "EDITOR"]

    def Dialog(self):
        @self.ffi.callback("FARWINDOWPROC")
        def DialogProc(hDlg, Msg, Param1, Param2):
            if Msg == self.ffic.DN_INITDIALOG:
                dlg.SetText(dlg.ID_vcol, "")
            return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)

        b = dlgb.DialogBuilder(
            self,
            DialogProc,
            "Python uedsort",
            "helptopic",
            0,
            dlgb.VSizer(
                dlgb.HSizer(
                    dlgb.TEXT(None, "Column:"),
                    dlgb.Spacer(),
                    dlgb.EDIT("vcol", 4, maxlength=4),
                ),
                dlgb.HSizer(
                    dlgb.BUTTON("vok", "Ok", default=True, flags=self.ffic.DIF_CENTERGROUP),
                    dlgb.BUTTON("vclose", "Close", flags=self.ffic.DIF_CENTERGROUP),
                ),
            ),
        )
        dlg = b.build(-1, -1)

        res = self.info.DialogRun(dlg.hDlg)
        if res == dlg.ID_vok:
            vcol = int(dlg.GetText(dlg.ID_vcol) or "0", 10)
        else:
            vcol = None
        self.info.DialogFree(dlg.hDlg)
        return vcol

    def Perform(self):
        vcol = self.Dialog()
        if vcol is None:
            return

        ei = self.ffi.new("struct EditorInfo *")
        self.info.EditorControl(self.ffic.ECTL_GETINFO, ei)

        if ei.BlockType != self.ffic.BTYPE_STREAM:
            # working only if not vertical block selection
            return

        egs = self.ffi.new("struct EditorGetString *")
        egs.StringNumber = ei.BlockStartLine
        self.info.EditorControl(self.ffic.ECTL_GETSTRING, egs)
        if egs.SelEnd != -1:
            return

        eur = self.ffi.new("struct EditorUndoRedo *")
        eur.Command = self.ffic.EUR_BEGIN
        self.info.EditorControl(self.ffic.ECTL_UNDOREDO, eur)

        lines = []
        for lno in range(ei.BlockStartLine, ei.TotalLines):
            esp = self.ffi.new("struct EditorSetPosition *")
            esp.CurLine = lno
            esp.CurPos = esp.Overtype = 0
            esp.CurTabPos = esp.TopScreenLine = esp.LeftPos = -1
            self.info.EditorControl(self.ffic.ECTL_SETPOSITION, esp)

            egs = self.ffi.new("struct EditorGetString *")
            egs.StringNumber = -1
            self.info.EditorControl(self.ffic.ECTL_GETSTRING, egs)

            if egs.SelStart == -1 or egs.SelStart == egs.SelEnd:
                # Stop when reaching the end of the text selection
                break

            if egs.SelEnd != -1:
                # Extend selection to line end
                es = self.ffi.new("struct EditorSelect *")
                es.BlockType = ei.BlockType
                es.BlockStartLine = ei.BlockStartLine
                es.BlockStartPos = 0
                es.BlockWidth = 0
                es.BlockHeight = line - ei.BlockStartLine + 1
                self.info.EditorControl(self.ffic.ECTL_SELECT, es)

            lines.append(self.f2s(egs.StringText))

        self.info.EditorControl(self.ffic.ECTL_DELETEBLOCK, self.ffi.NULL)

        lines.sort(key=lambda x:x[vcol:])
        s = '\r'.join(lines)+'\r'
        s = self.s2f(s)
        self.info.EditorControl(self.ffic.ECTL_INSERTTEXT, s)

        eur = self.ffi.new("struct EditorUndoRedo *")
        eur.Command = self.ffic.EUR_END
        self.info.EditorControl(self.ffic.ECTL_UNDOREDO, eur)


    def OpenPlugin(self, OpenFrom):
        if OpenFrom == 5:
            # EDITOR
            self.Perform()
        return -1
