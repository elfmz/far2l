import logging
import time
import re
from far2l.plugin import PluginBase
from far2l.farprogress import ProgressMessage
from far2l.fardialogbuilder import (
    TEXT,
    EDIT,
    BUTTON,
    CHECKBOX,
    HLine,
    HSizer,
    VSizer,
    DialogBuilder,
)


log = logging.getLogger(__name__)


class Worker:
    def __init__(self, parent, curline, curpos, nlines, pat, repl):
        self.parent = parent
        self.curline = curline
        self.curpos = curpos
        self.nlines = nlines
        self.pat = pat
        self.repl = repl
        self.prog = re.compile(pat)

    def info(self, line):
        print('***** curline=', self.curline, 'curpos=', self.curpos, '**', line)

    def replace(self, spart, m):
        repl = self.prog.sub(self.repl, spart[m.start():m.end()], count=1)
        rc = self.parent.ask(m, spart[m.start():m.end()], repl)
        if rc == 0:
            spart = spart[:m.start()] + repl + spart[m.end():]
        return rc, spart

    def forward(self):
        while self.curline < self.nlines:
            line = self.parent.getline(self.curline)
            self.info(line)
            while True:
                spart = line[self.curpos:]
                m = self.prog.search(spart)
                if not m:
                    break
                rc, repl = self.replace(spart, m)
                if rc == 3:
                    return
                line = line[:self.curpos] + repl
                self.curpos += len(repl)-(len(spart)-m.end())
                self.parent.setline(self.curline, line)
            self.curline += 1
            self.curpos = 0

    def backward(self):
        while self.curline >= 0:
            line = self.parent.getline(self.curline)
            self.info(line)
            while True:
                spart = line[:self.curpos]
                ml = list(self.prog.finditer(spart))
                if not ml:
                    break
                rc, repl = self.replace(spart, ml[-1])
                if rc == 3:
                    return
                line = repl+line[self.curpos:]
                self.curpos = ml[-1].start()
                self.parent.setline(self.curline, line)
            self.curline -= 1
            self.curpos = -1

class Plugin(PluginBase):
    label = "Python ED Replace"
    openFrom = []
    f_search = ''
    f_replace = ''
    f_case = 0
    f_whole = 0
    f_rev = 0
    f_rx = 0
    f_select = 0
    f_is_search = False

    def ProcessEditorInput(self, Rec):
        rec = self.ffi.cast("INPUT_RECORD *", Rec)

        if (
            rec.EventType != self.ffic.KEY_EVENT
            or not rec.Event.KeyEvent.bKeyDown
            or (rec.Event.KeyEvent.wVirtualKeyCode != self.ffic.VK_F3)
        ):
            return 0
        ks = rec.Event.KeyEvent.dwControlKeyState
        fctrl = ks & (self.ffic.RIGHT_CTRL_PRESSED | self.ffic.LEFT_CTRL_PRESSED) != 0
        falt = ks & (self.ffic.RIGHT_ALT_PRESSED | self.ffic.LEFT_ALT_PRESSED) != 0
        fshift = ks & (self.ffic.SHIFT_PRESSED) != 0
        log.debug(f'ED Replace ctrl:{fctrl} alt:{falt} shift:{fshift}')
        self.Perform(fctrl, falt, fshift)
        return 1

    def ConfigDialog(self, fctrl):
        self.f_is_search = not fctrl

        @self.ffi.callback("FARWINDOWPROC")
        def DialogProc(hDlg, Msg, Param1, Param2):
            if Msg == self.ffic.DN_INITDIALOG:
                try:
                    dlg.SetCheck(dlg.ID_f_case, self.f_case)
                    dlg.SetCheck(dlg.ID_f_whole, self.f_whole)
                    dlg.SetCheck(dlg.ID_f_rev, self.f_rev)
                    dlg.SetCheck(dlg.ID_f_rx, self.f_rx)
                    dlg.SetCheck(dlg.ID_f_select, self.f_select)
                except:
                    log.exception("bang")
            return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)

        title = "Search" if self.f_is_search else "Replace"
        dlgitems = [
            HSizer(
                TEXT(None, "Search &for  :"),
                EDIT("f_search", 33, maxlength=80, history='f_search', flags=self.ffic.DIF_USELASTHISTORY|self.ffic.DIF_HISTORY),
            )
        ]
        if not self.f_is_search:
            dlgitems.append(
                HSizer(
                    TEXT(None, "Replace &with:"),
                    EDIT("f_replace", 33, maxlength=80, history='f_replace', flags=self.ffic.DIF_USELASTHISTORY|self.ffic.DIF_HISTORY),
                )
            )
        dlgitems.extend([
            HLine(),
            HSizer(
                VSizer(
                    CHECKBOX("f_case", "&Case sensitive"),
                    CHECKBOX("f_whole", "W&hole words"),
                    CHECKBOX("f_rev", "Re&verse search"),
                ),
                VSizer(
                    CHECKBOX("f_rx", "Regular e&xpressions"),
                    CHECKBOX("f_select", "Se&lect found"),
                )
            ),
            HLine(),
            HSizer(
                BUTTON("vreplace", "&"+title, default=True, flags=self.ffic.DIF_CENTERGROUP),
                BUTTON("vcancel", "Cancel", flags=self.ffic.DIF_CENTERGROUP),
            ),
        ])
        b = DialogBuilder(
            self,
            DialogProc,
            title,
            "uedreplace",
            0,
            VSizer(*list(dlgitems)),
        )
        dlg = b.build(-1, -1)

        res = self.info.DialogRun(dlg.hDlg)
        if res == dlg.ID_vreplace:
            self.f_search = dlg.GetText(dlg.ID_f_search)
            if self.f_is_search:
                self.f_replace = ''
            else:
                self.f_replace = dlg.GetText(dlg.ID_f_replace)
            self.f_case = dlg.GetCheck(dlg.ID_f_case)
            self.f_whole = dlg.GetCheck(dlg.ID_f_whole)
            self.f_rev = dlg.GetCheck(dlg.ID_f_rev)
            self.f_rx = dlg.GetCheck(dlg.ID_f_rx)
            self.f_select = dlg.GetCheck(dlg.ID_f_select)
        self.info.DialogFree(dlg.hDlg)
        log.debug(f'ED Replace cfg: res={res} rep={dlg.ID_vreplace}')
        return res == dlg.ID_vreplace

    def Ask(self, sold, snew):
        @self.ffi.callback("FARWINDOWPROC")
        def DialogProc(hDlg, Msg, Param1, Param2):
            return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)
        b = DialogBuilder(
            self,
            DialogProc,
            "Replace",
            "uedreplace",
            0,
            VSizer(
                TEXT(None, "Replace:"+sold.strip()),
                TEXT(None, "With   :"+snew.strip()),
                HLine(),
                HSizer(
                    BUTTON("vreplace", "&Replace", default=True, flags=self.ffic.DIF_CENTERGROUP),
                    BUTTON("vall", "&All", flags=self.ffic.DIF_CENTERGROUP),
                    BUTTON("vskip", "&Skip", flags=self.ffic.DIF_CENTERGROUP),
                    BUTTON("vcancel", "&Cancel", flags=self.ffic.DIF_CENTERGROUP),
                ),
            ),
        )
        dlg = b.build(-1, -1)

        res = self.info.DialogRun(dlg.hDlg)
        self.info.DialogFree(dlg.hDlg)
        if res == dlg.ID_vreplace:
            return 0
        elif res == dlg.ID_vall:
            return 1
        elif res == dlg.ID_vskip:
            return 2
        elif res == dlg.ID_vcancel:
            return 3
        return 3

    def Perform(self, fctrl, falt, fshift):
        if not self.f_search or not (falt or fshift):
            if not self.ConfigDialog(fctrl):
                return
            if not self.f_search:
                return
        if falt:
            self.DoPrev()
        elif fshift:
            self.DoNext()
        else:
            if self.f_rev:
                self.DoPrev()
            else:
                self.DoNext()

    def DoNext(self):
        f_search = self.f_search
        f_replace = self.f_replace
        if not self.f_rx and self.f_whole:
            f_search = r'\b'+f_search+r'\b'
        f_flags = re.IGNORECASE if not self.f_case else 0
        prog = re.compile(f_search, flags=f_flags)

        ei = self.ffi.new("struct EditorInfo *")
        self.info.EditorControl(self.ffic.ECTL_GETINFO, ei)

        self.fask = True
        self.found = False

        egs = self.ffi.new("struct EditorGetString *")
        esp = self.ffi.new("struct EditorSetPosition *")
        esp.CurTabPos = -1
        esp.TopScreenLine = -1
        esp.LeftPos = -1
        esp.Overtype = -1
        es = self.ffi.new("struct EditorSelect *")

        class IO:
            lno = None
            line = None

            def getline(me, lno):
                if lno == me.lno:
                    return me.line
                egs.StringNumber = lno
                self.info.EditorControl(self.ffic.ECTL_GETSTRING, egs)
                me.line = self.f2s(egs.StringText)
                return me.line

            def setline(me, lno, line):
                # select whole line
                es.BlockType = self.ffic.BTYPE_COLUMN
                es.BlockStartLine = lno
                es.BlockStartPos = 0
                es.BlockWidth = len(me.line)
                es.BlockHeight = 1
                self.info.EditorControl(self.ffic.ECTL_SELECT, es)
                # delete selected line
                self.info.EditorControl(self.ffic.ECTL_DELETEBLOCK, self.ffi.NULL)
                # insert new line
                self.info.EditorControl(self.ffic.ECTL_INSERTTEXT, self.s2f(line))
                me.line = line

            def ask(me, m, found, replace):
                self.found = True
                if self.f_is_search or self.Ask:
                    # set editor position
                    esp.CurLine = worker.curline
                    esp.CurPos = worker.curpos+m.start()
                    self.info.EditorControl(self.ffic.ECTL_SETPOSITION, esp)
                    if self.f_select or self.Ask:
                        # select found item
                        es.BlockType = self.ffic.BTYPE_COLUMN
                        es.BlockStartLine = worker.curline
                        es.BlockStartPos = worker.curpos+m.start()
                        es.BlockWidth = m.end()-m.start()
                        es.BlockHeight = 1
                        self.info.EditorControl(self.ffic.ECTL_SELECT, es)
                    # refresh
                    self.info.EditorControl(self.ffic.ECTL_REDRAW, self.ffi.NULL)
                if self.f_is_search:
                    # cancel=search only
                    return 3
                if self.fask:
                    # ask for decision
                    rc = self.Ask(found, replace)
                else:
                    rc = 0
                if rc == 1:
                    # all
                    self.fask = False
                    rc = 0
                return rc

        if not self.f_is_search:
            eur = self.ffi.new("struct EditorUndoRedo *")
            eur.Command = self.ffic.EUR_BEGIN
            self.info.EditorControl(self.ffic.ECTL_UNDOREDO, eur)

        worker = Worker(IO(), ei.CurLine, ei.CurPos, ei.TotalLines, prog, f_replace)
        worker.forward()

        if not self.f_is_search:
            eur = self.ffi.new("struct EditorUndoRedo *")
            eur.Command = self.ffic.EUR_END
            self.info.EditorControl(self.ffic.ECTL_UNDOREDO, eur)

        if not self.found:
            self.notice("Could not find the string:"+f_search)

        self.info.EditorControl(self.ffic.ECTL_REDRAW, self.ffi.NULL)

    def DoPrev(self):
        f_search = self.f_search
        f_replace = self.f_replace
        if not self.f_rx and self.f_whole:
            f_search = r'\b'+f_search+r'\b'
        f_flags = re.IGNORECASE if not self.f_case else 0
        prog = re.compile(f_search, flags=f_flags)

        ei = self.ffi.new("struct EditorInfo *")
        self.info.EditorControl(self.ffic.ECTL_GETINFO, ei)

        self.fask = True
        self.found = False

        egs = self.ffi.new("struct EditorGetString *")
        esp = self.ffi.new("struct EditorSetPosition *")
        esp.CurTabPos = -1
        esp.TopScreenLine = -1
        esp.LeftPos = -1
        esp.Overtype = -1
        es = self.ffi.new("struct EditorSelect *")

        class IO:
            lno = None
            line = None

            def getline(me, lno):
                if lno == me.lno:
                    return me.line
                egs.StringNumber = lno
                self.info.EditorControl(self.ffic.ECTL_GETSTRING, egs)
                me.line = self.f2s(egs.StringText)
                return me.line

            def setline(me, lno, line):
                # select whole line
                es.BlockType = self.ffic.BTYPE_COLUMN
                es.BlockStartLine = lno
                es.BlockStartPos = 0
                es.BlockWidth = len(me.line)
                es.BlockHeight = 1
                self.info.EditorControl(self.ffic.ECTL_SELECT, es)
                # delete selected line
                self.info.EditorControl(self.ffic.ECTL_DELETEBLOCK, self.ffi.NULL)
                # insert new line
                self.info.EditorControl(self.ffic.ECTL_INSERTTEXT, self.s2f(line))
                me.line = line

            def ask(me, m, found, replace):
                self.found = True
                if self.f_is_search or self.Ask:
                    # set editor position
                    esp.CurLine = worker.curline
                    esp.CurPos = m.start()
                    self.info.EditorControl(self.ffic.ECTL_SETPOSITION, esp)
                    if self.f_select or self.Ask:
                        # select found item
                        es.BlockType = self.ffic.BTYPE_COLUMN
                        es.BlockStartLine = worker.curline
                        es.BlockStartPos = m.start()
                        es.BlockWidth = m.end()-m.start()
                        es.BlockHeight = 1
                        self.info.EditorControl(self.ffic.ECTL_SELECT, es)
                    # refresh
                    self.info.EditorControl(self.ffic.ECTL_REDRAW, self.ffi.NULL)
                if self.f_is_search:
                    # cancel=search only
                    return 3
                if self.fask:
                    # ask for decision
                    rc = self.Ask(found, replace)
                else:
                    rc = 0
                if rc == 1:
                    # all
                    self.fask = False
                    rc = 0
                return rc

        if not self.f_is_search:
            eur = self.ffi.new("struct EditorUndoRedo *")
            eur.Command = self.ffic.EUR_BEGIN
            self.info.EditorControl(self.ffic.ECTL_UNDOREDO, eur)

        worker = Worker(IO(), ei.CurLine, ei.CurPos, ei.TotalLines, prog, f_replace)
        worker.backward()

        if not self.f_is_search:
            eur = self.ffi.new("struct EditorUndoRedo *")
            eur.Command = self.ffic.EUR_END
            self.info.EditorControl(self.ffic.ECTL_UNDOREDO, eur)

        if not self.found:
            self.notice("Could not find the string:"+f_search)

        self.info.EditorControl(self.ffic.ECTL_REDRAW, self.ffi.NULL)
