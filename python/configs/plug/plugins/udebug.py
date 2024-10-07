import logging
import debugpy
from far2l.plugin import PluginBase
from far2l.farprogress import ProgressMessage
from far2l.fardialogbuilder import (
    TEXT,
    EDIT,
    BUTTON,
    MASKED,
    HLine,
    HSizer,
    VSizer,
    DialogBuilder,
)


log = logging.getLogger(__name__)


class Config:
    configured = False
    logto = "/tmp"
    host = "127.0.0.1"
    port = 5678


class Plugin(PluginBase):
    label = "Python udebug"
    openFrom = ["PLUGINSMENU", "COMMANDLINE", "EDITOR", "VIEWER", "FILEPANEL"]

    def debug(self):
        if Config.configured:
            self.parent.Message(("udebug can be configured only once.",))
            return

        t = ProgressMessage(self, self.label, "Waiting for VSCode", 100)
        t.show()
        try:
            Config.configured = True
            debugpy.log_to(Config.logto)
            # in vs code debuger select attach, port = 5678
            # commands in shell:
            #   py:debug
            # elsewhere in python code:
            #   import debugpy
            #   debugpy.breakpoint()
            debugpy.listen((Config.host, Config.port))
            debugpy.wait_for_client()
            # debugpy.breakpoint()
        except:
            log.exception()
        finally:
            t.close()

    def breakpoint(self):
        debugpy.breakpoint()

    @staticmethod
    def HandleCommandLine(line):
        return line in ("debug", "breakpoint")

    def CommandLine(self, line):
        getattr(self, line)()

    def Configure(self):
        if Config.configured:
            self.parent.Message(("udebug can be configured only once.",))
            return

        @self.ffi.callback("FARWINDOWPROC")
        def DialogProc(hDlg, Msg, Param1, Param2):
            if Msg == self.ffic.DN_INITDIALOG:
                try:
                    dlg.SetText(dlg.ID_logpath, Config.logto)
                    dlg.SetText(dlg.ID_ipaddress, Config.host)
                    dlg.SetText(dlg.ID_port, str(Config.port))
                except:
                    log.exception("bang")
            return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)

        b = DialogBuilder(
            self,
            DialogProc,
            "debugpy config",
            "helptopic",
            0,
            VSizer(
                HSizer(
                    TEXT(None, "log path:"),
                    EDIT("logpath", 33, maxlength=80),
                ),
                HSizer(
                    TEXT(None, "ip address:"),
                    EDIT("ipaddress", 20),
                ),
                HSizer(
                    TEXT(None, "ip port:"),
                    MASKED("port", "99999"),
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
        if res == dlg.ID_vok:
            Config.logto = dlg.GetText(dlg.ID_logpath)
            Config.host = dlg.GetText(dlg.ID_ipaddress)
            Config.port = int(dlg.GetText(dlg.ID_port), 10)
        self.info.DialogFree(dlg.hDlg)
        log.debug("end dialog")

    def OpenPlugin(self, OpenFrom):
        self.Configure()
