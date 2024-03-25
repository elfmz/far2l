import logging
import time
import ctypes as ct
from far2l.plugin import PluginBase
import far2lc

import debugpy


log = logging.getLogger(__name__)


class Plugin(PluginBase):
    label = "Python ED Uniq"
    openFrom = ["PLUGINSMENU", "EDITOR"]

    def Perform(self):
        ei = self.ffi.new("struct EditorInfo *")
        self.info.EditorControl(self.ffic.ECTL_GETINFO, ei)

        if ei.BlockType != self.ffic.BTYPE_STREAM:
            # working only if not vertical block selection
            return 0;

        egs = self.ffi.new("struct EditorGetString *")
        egs.StringNumber = ei.BlockStartLine
        self.info.EditorControl(self.ffic.ECTL_GETSTRING, egs)
        if egs.SelEnd != -1:
            return 0

        eur = self.ffi.new("struct EditorUndoRedo *")
        eur.Command = self.ffic.EUR_BEGIN
        self.info.EditorControl(self.ffic.ECTL_UNDOREDO, eur)

        lines = []
        result = []
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

            s = self.f2s(egs.StringText)
            if s not in lines:
                lines.append(s)

        self.info.EditorControl(self.ffic.ECTL_DELETEBLOCK, self.ffi.NULL)

        s = '\r'.join(lines)+'\r'
        s = self.s2f(s)
        self.info.EditorControl(self.ffic.ECTL_INSERTTEXT, s)

        eur = self.ffi.new("struct EditorUndoRedo *")
        eur.Command = self.ffic.EUR_END
        self.info.EditorControl(self.ffic.ECTL_UNDOREDO, eur)


    def OpenPlugin(self, OpenFrom):
        # debugpy.breakpoint()
        if OpenFrom == 5:
            # EDITOR
            self.Perform()
        return -1
