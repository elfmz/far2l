import logging
import debugpy
from far2l.plugin import PluginBase


log = logging.getLogger(__name__)

class Config:
    configured = False
    logto = '/tmp'
    host = '127.0.0.1'
    port = 5678

class Plugin(PluginBase):
    label = "Python udebug"
    openFrom = ["PLUGINSMENU", "COMMANDLINE", "EDITOR", "VIEWER", "FILEPANEL"]

    def debug(self):
        if Config.configured:
            log.debug('udebug can be configured only once')
            return

        if not Config.configured:
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
        #debugpy.breakpoint()

    def breakpoint(self):
        debugpy.breakpoint()

    @staticmethod
    def HandleCommandLine(line):
        return line in ('debug', 'breakpoint')

    def CommandLine(self, line):
        getattr(self, line)()

    def Configure(self):
        @self.ffi.callback("FARWINDOWPROC")
        def KeyDialogProc(hDlg, Msg, Param1, Param2):
            #if Msg == self.ffic.DN_KEY:
            #    if Param2 == 27:
            #        return 0
            return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)

        fdis = [
            (self.ffic.DI_DOUBLEBOX,  2, 1, 32, 7, 0, {'Selected':0}, 0, 0, self.s2f("Python Debug Config"), 0),
            (self.ffic.DI_TEXT,       4, 2, 11, 0, 0, {'Selected':0}, 0, 0, self.s2f("Log path"), 0),
            (self.ffic.DI_EDIT,      13, 2, 30, 0, 0, {'Selected':0}, 0, 0, self.s2f(Config.logto), 32),
            (self.ffic.DI_TEXT,       4, 3,  8, 0, 0, {'Selected':0}, 0, 0, self.s2f("Host"), 0),
            (self.ffic.DI_EDIT,      13, 3, 30, 0, 0, {'Selected':0}, 0, 0, self.s2f(Config.host), 32),
            (self.ffic.DI_TEXT,       4, 4,  8, 0, 0, {'Selected':0}, 0, 0, self.s2f("Port"), 0),
            (self.ffic.DI_EDIT,      13, 4, 30, 0, 0, {'Selected':0}, 0, 0, self.s2f(str(Config.port)), 5),
            (self.ffic.DI_TEXT,       2, 5,  0, 0, 0, {'Selected':0}, self.ffic.DIF_SEPARATOR, 0, self.ffi.NULL, 0),
            (self.ffic.DI_BUTTON,     6, 6,  0, 0, 0, {'Selected':0}, 0, self.ffic.DIF_DEFAULT, self.s2f("OK"), 0),
            (self.ffic.DI_BUTTON,    19, 6,  0, 0, 0, {'Selected':0}, 0, 0, self.s2f("Cancel"), 0),
        ]
        fdi = self.ffi.new("struct FarDialogItem []", fdis)
        hDlg = self.info.DialogInit(self.info.ModuleNumber,
            -1, -1, fdis[0][3]+3, fdis[0][4]+2,
            self.s2f("Debug Config"),
            fdi, len(fdi), 0, 0, KeyDialogProc, 0)
        res = self.info.DialogRun(hDlg)
        log.debug('dialog result: {0}'.format(res))
        if res == 8:
            result = []
            for i in (2, 4, 6):
                sptr = self.info.SendDlgMessage(hDlg, self.ffic.DM_GETCONSTTEXTPTR, i, 0)
                result.append(self.f2s(sptr))
            log.debug('result: {0}'.format(result))
            Config.logto = result[0]
            Config.host = result[1]
            Config.port = int(result[2])
        self.info.DialogFree(hDlg)
        log.debug('end dialog')

    def OpenPlugin(self, OpenFrom):
        self.Configure()
