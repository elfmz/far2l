import os
import tempfile
import logging
from far2l.plugin import PluginBase

log = logging.getLogger(__name__)


class Plugin(PluginBase):
    label = "Python VTLog"
    openFrom = ["PLUGINSMENU", "EDITOR"]

    def Perform(self):
        eur = self.ffi.new("struct EditorUndoRedo *")
        eur.Command = self.ffic.EUR_BEGIN
        self.info.EditorControl(self.ffic.ECTL_UNDOREDO, eur)

        count = self.info.FSF.VTEnumBackground(self.ffi.NULL, 0)
        if count > 0:
            # why is it always equal to 0 ?
            log.debug(f'VTEnumBackground={count}')
            con_hnds = self.ffi.new("HANDLE []", count)
            vtssize = self.info.FSF.VTEnumBackground(con_hnds, count)
        else:
            con_hnds = self.ffi.new("HANDLE []", 1)
            con_hnds[0] = self.ffi.NULL

        block = None
        with tempfile.NamedTemporaryFile(delete=False) as fp:
            fp.close()
            os.unlink(fp.name)
            try:
                flags = self.ffic.VT_LOGEXPORT_COLORED
                flags = self.ffic.VT_LOGEXPORT_WITH_SCREENLINES
            except:
                flags = 0
            vtssize = self.info.FSF.VTLogExport(con_hnds[0], flags, self.s2f(fp.name))
            with open(fp.name, mode='rb') as f:
                block = f.read()
            os.unlink(fp.name)

        try:
            block = block.decode('utf-8')
        except:
            log.exception()
            return
        s = self.s2f(block)
        self.info.EditorControl(self.ffic.ECTL_INSERTTEXT, s)

        eur = self.ffi.new("struct EditorUndoRedo *")
        eur.Command = self.ffic.EUR_END
        self.info.EditorControl(self.ffic.ECTL_UNDOREDO, eur)
        log.debug('vtinsert end')

    def OpenPlugin(self, OpenFrom):
        # debugpy.breakpoint()
        self.Perform()
        return -1
