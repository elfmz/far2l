import logging
from far2l.plugin import PluginBase


log = logging.getLogger(__name__)


class Plugin(PluginBase):
    label = "Python udialog"
    openFrom = ["PLUGINSMENU"]

    @staticmethod
    def HandleCommandLine(line):
        return line in ("dialog", "exec")

    def CommandLine(self, line):
        if line == "dialog":
            log.debug("dialog")

            @self.ffi.callback("FARWINDOWPROC")
            def KeyDialogProc(hDlg, Msg, Param1, Param2):
                # if Msg == self.ffic.DN_KEY:
                #    if Param2 == 27:
                #        return 0
                return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)

            fdis = [
                (
                    self.ffic.DI_DOUBLEBOX,
                    2,
                    1,
                    31,
                    4,
                    0,
                    {"Selected": 0},
                    0,
                    0,
                    self.s2f("Python"),
                    0,
                ),
                (
                    self.ffic.DI_TEXT,
                    -1,
                    2,
                    0,
                    2,
                    0,
                    {"Selected": 0},
                    0,
                    0,
                    self.s2f("Plugin path"),
                    0,
                ),
                (
                    self.ffic.DI_EDIT,
                    5,
                    3,
                    28,
                    3,
                    0,
                    {"Selected": 0},
                    0,
                    0,
                    self.ffi.NULL,
                    0,
                ),
            ]
            fdi = self.ffi.new("struct FarDialogItem []", fdis)
            hDlg = self.info.DialogInit(
                self.info.ModuleNumber,
                -1,
                -1,
                34,
                6,
                self.s2f("Set Python Path"),
                fdi,
                len(fdi),
                0,
                0,
                KeyDialogProc,
                0,
            )
            res = self.info.DialogRun(hDlg)
            if res != -1:
                sptr = self.info.SendDlgMessage(
                    hDlg, self.ffic.DM_GETCONSTTEXTPTR, 2, 0
                )
                path = self.f2s(sptr)
                log.debug("path: {0}".format(path))
            self.info.DialogFree(hDlg)
            log.debug("end dialog")
        else:
            line = line.split(" ", 1)[1]
            exec(line, globals(), locals())

    def OpenPlugin(self, OpenFrom):
        _MsgItems = [
            self.s2f("Python"),
            self.s2f(""),
            self.s2f("python.cpp: compiling..."),
            self.s2f("   0 error(s), 0 warning(s) :-)"),
            self.s2f(""),
            self.s2f("\x01"),
            self.s2f("&Ok"),
        ]
        MsgItems = self.ffi.new("wchar_t *[]", _MsgItems)
        self.info.Message(
            self.info.ModuleNumber,  # GUID
            self.ffic.FMSG_WARNING | self.ffic.FMSG_LEFTALIGN,  # Flags
            self.s2f("Contents"),  # HelpTopic
            MsgItems,  # Items
            len(MsgItems),  # ItemsNumber
            1,  # ButtonsNumber
        )
        return -1
