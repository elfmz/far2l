import time
import logging
import threading
from far2l.plugin import PluginBase
from far2l.fardialogbuilder import (
    Spacer,
    TEXT,
    BUTTON,
    HLine,
    HSizer,
    VSizer,
    DialogBuilder,
)

log = logging.getLogger(__name__)


class ProgressThread(threading.Thread):
    def __init__(self):
        super().__init__()
        self.cond = threading.Lock()
        self.daemon = True
        self.sleep = 0.1
        self.vmin = 0
        self.vmax = 100
        self.done = False

    def cancel(self):
        if not self.done:
            self.vmin = -self.vmin

    def run(self):
        while self.vmin >= 0 and self.vmin < self.vmax:
            self.cond.acquire(True)
            self.vmin += 1
            self.cond.release()
            time.sleep(self.sleep)
        self.done = True

class Plugin(PluginBase):
    label = "Python Progress Dialog"
    openFrom = ["PLUGINSMENU"]

    def OnIdle(self, t, dlg):
        if not t.cond.acquire(False):
            return
        self._OnIdle(t, dlg)
        t.cond.release()

    def _OnIdle(self, t, dlg):
        n = t.vmin * 20 // t.vmax
        fprog = (('='*n)+' '*20)[:20]
        dlg.SetText(dlg.ID_fprog, '[{}]'.format(fprog))

    def OpenPlugin(self, OpenFrom):
        t = ProgressThread()

        @self.ffi.callback("FARWINDOWPROC")
        def DialogProc(hDlg, Msg, Param1, Param2):
            if Msg == self.ffic.DN_INITDIALOG:
                dlg.SetText(dlg.ID_fprog, "")
                # start thread
                t.start()
            elif Msg == self.ffic.DN_ENTERIDLE:
                if t.done:
                    dlg.Close(0)
                else:
                    self.OnIdle(t, dlg)
            return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)

        b = DialogBuilder(
            self,
            DialogProc,
            "Python Progress Demo",
            "helptopic",
            self.ffic.FDLG_REGULARIDLE,
            VSizer(
                TEXT("fprog", " "*(2+20)),
                HLine(),
                BUTTON("vcancel", "Cancel", flags=self.ffic.DIF_CENTERGROUP),
            ),
        )
        dlg = b.build(-1, -1)

        res = self.info.DialogRun(dlg.hDlg)

        t.cancel()
        v = t.vmin
        del t
        log.debug('t.vmin={} res={}'.format(v, res))

        self.info.DialogFree(dlg.hDlg)
