import logging
import time
import ctypes as ct
from far2l.plugin import PluginBase
import far2lc

import debugpy


log = logging.getLogger(__name__)


class Plugin(PluginBase):
    label = "Python INFO"
    openFrom = ["PLUGINSMENU", "EDITOR"]

    # @staticmethod
    # def HandleCommandLine(line):
    #    return line in ('edit',)

    def getEditorInfo(self):
        ei = self.ffi.new("struct EditorInfo *")
        self.info.EditorControl(self.ffic.ECTL_GETINFO, ei)
        result = """\
EditorID: {}
WindowSizeX: {}
WindowSizeY: {}
TotalLines: {}
CurLine: {}
CurPos: {}
CurTabPos: {}
TopScreenLine: {}
LeftPos: {}
Overtype: {}
BlockType: {}
BlockStartLine: {}
Options: {}
TabSize: {}
BookMarkCount: {}
CurState: {}
CodePage: {}
Reserved[5]: {}
""".format(
            ei.EditorID,
            ei.WindowSizeX,
            ei.WindowSizeY,
            ei.TotalLines,
            ei.CurLine,
            ei.CurPos,
            ei.CurTabPos,
            ei.TopScreenLine,
            ei.LeftPos,
            ei.Overtype,
            ei.BlockType,
            ei.BlockStartLine,
            ei.Options,
            ei.TabSize,
            ei.BookMarkCount,
            ei.CurState,
            ei.CodePage,
            ei.Reserved,
        )
        return result

    def Info(self):
        # debugpy.breakpoint()
        txt = self.getEditorInfo()
        #open("/tmp/uinfo", "wt").write(txt)
        log.debug('self.info.Editor')
        self.info.Editor(
            "/tmp/uinfo",
            "uinfo",
            #            0, 0, -1, -1,
            0,
            0,
            50,
            25,
            self.ffic.EF_NONMODAL|self.ffic.EF_DELETEONCLOSE|self.ffic.EF_IMMEDIATERETURN,
            0,
            0,
            0xFFFFFFFF,  # =-1=self.ffic.CP_AUTODETECT
        )
        log.debug('/self.info.Editor')
        try:
            for i in range(20):
                #pending = far2lc.CheckForInput(500)
                pending = False
                isesc = far2lc.CheckForEscape()
                log.debug('i={} pending={} escape={}'.format(i, pending, isesc))
                if isesc:
                    break
                time.sleep(1)
        except:
            log.exception('CheckForKeyPressed failure')
        # self.info.EditorControl(self.ffic.ECTL_INSERTTEXT, txt)

    def OpenPlugin(self, OpenFrom):
        # debugpy.breakpoint()
        if OpenFrom == 5:
            # EDITOR
            self.Info()
        return -1
