import logging
from .far2lpanel import Panel


log = logging.getLogger(__name__)


class PluginBase:
    # private
    name = ""
    number = 0
    # public
    USERHOME = ""  # ~/.config/far2l/plugins/python
    label = ""
    # openFrom = ["DISKMENU", "PLUGINSMENU", "FINDLIST", "SHORTCUT", "COMMANDLINE", "EDITOR", "VIEWER", "FILEPANEL"]
    openFrom = []
    Configure = None  # override with method when configuration dialog is needed

    def __init__(self, parent, info, ffi, ffic):
        self.parent = parent
        self.info = info
        self.ffi = ffi
        self.ffic = ffic
        self.hplugin = self.ffi.cast("void *", id(self))
        self.api = Panel(self, info, ffi, ffic)

    def s2f(self, s):
        return self.ffi.new("wchar_t []", s)

    def f2s(self, s):
        return self.ffi.string(self.ffi.cast("wchar_t *", s))

    def notice(self, body, title=None):
        return self._popup(body, title, 0)

    def error(self, body, title=None):
        return self._popup(body, title, self.ffic.FMSG_WARNING)

    def _popup(self, body, title, flags):
        items = [self.s2f(title)] if title else [self.s2f('')]
        if isinstance(body, str):
            if '\n' in body:
                body = body.split('\n')
            else:
                items.append(self.s2f(body))
        if not isinstance(body, str):
            items.extend(self.s2f(s) for s in body)
        # note: needs to be at least 2 items, otherwise message 
        # box is not shown
        citems = self.ffi.new('wchar_t *[]', items)
        if not flags:
            flags = self.ffic.FMSG_MB_OK
        return self.info.Message(
            self.info.ModuleNumber,  # GUID
            flags,  # Flags
            self.ffi.NULL,  # HelpTopic
            citems, len(citems), 1,  # ButtonsNumber
        )

    @staticmethod
    def HandleCommandLine(line):
        log.debug("Plugin.HandleCommandLine({})".format(line))
        return False

    def OpenPlugin(self, OpenFrom):
        log.debug("Plugin.OpenPlugin({}) #{}".format(OpenFrom, self.label))

    def Close(self):
        log.debug("Plugin.Close() #{}".format(self.label))

    def GetOpenPluginInfo(self, OpenInfo):
        log.debug("Plugin.GetOpenPluginInfo({}) #{}".format(OpenInfo, self.label))

    def ProcessKey(self, Key, ControlState):
        # log.debug("Plugin.ProcessKey({0}, {1})".format(Key, ControlState))
        return 0

    def MayExitFAR(self):
        # log.debug("Plugin.MayExitFAR()")
        return True

    def ExitFAR(self):
        #log.debug("Plugin.ExitFAR()")
        pass

    @staticmethod
    def OpenFilePlugin(parent, info, ffi, ffic, Name, Data, DataSize, OpMode):
        return False

    def ProcessDialogEvent(self, Event, Param):
        # log.debug("Plugin.ProcessDialogEvent({0}, {1})".format(Event, Param))
        return 0

    def ProcessEditorEvent(self, Event, Param):
        # log.debug("Plugin.ProcessEditorEvent({0}, {1})".format(Event, Param))
        return 0

    def ProcessEditorInput(self, Rec):
        # log.debug("Plugin.ProcessEditorInput({0})".format(Rec))
        return 0

    def ProcessSynchroEvent(self, Event, Param):
        # log.debug("Plugin.ProcessSynchroEvent({0}, {1})".format(Event, Param))
        return 0

    def ProcessViewerEvent(self, Event, Param):
        # log.debug("Plugin.ProcessViewerEvent({0}, {1})".format(Event, Param))
        return 0


class PluginVFS(PluginBase):
    def GetFindData(self, PanelItem, ItemsNumber, OpMode):
        log.debug(
            "VFS.GetFindData({0}, {1}, {2})".format(PanelItem, ItemsNumber, OpMode)
        )
        return 0

    def FreeFindData(self, PanelItem, ItemsNumber):
        log.debug("VFS.FreeFindData({0}, {1})".format(PanelItem, ItemsNumber))

    def GetVirtualFindData(self, PanelItem, ItemsNumber, Path):
        log.debug(
            "VFS.GetVirtualFindData({0}, {1}, {2})".format(PanelItem, ItemsNumber, Path)
        )
        return 0

    def FreeVirtualFindData(self, PanelItem, ItemsNumber):
        log.debug("VFS.FreeVirtualFindData({0}, {1})".format(PanelItem, ItemsNumber))

    def Compare(self, PanelItem1, PanelItem2, Mode):
        log.debug("VFS.Compare({0}, {1}, {2})".format(PanelItem1, PanelItem2, Mode))
        return -2

    def GetFiles(self, PanelItem, ItemsNumber, Move, DestPath, OpMode):
        log.debug(
            "VFS.GetFiles({0}, {1}, {2}, {3}, {4})".format(
                PanelItem,
                ItemsNumber,
                Move,
                DestPath,
                OpMode,
            )
        )
        return 0

    def PutFiles(self, PanelItem, ItemsNumber, Move, SrcPath, OpMode):
        log.debug(
            "VFS.PutFiles({0}, {1}, {2}, {3}, {4})".format(
                PanelItem,
                ItemsNumber,
                Move,
                SrcPath,
                OpMode,
            )
        )
        return 0

    def MakeDirectory(self, Name, OpMode):
        log.debug("VFS.MakeDirectory({0}, {1})".format(Name, OpMode))
        return 0

    def SetDirectory(self, Dir, OpMode):
        log.debug("VFS.SetDirectory({0}, {1})".format(Dir, OpMode))
        return 0

    def DeleteFiles(self, PanelItem, ItemsNumber, OpMode):
        log.debug(
            "VFS.DeleteFiles({0}, {1}, {2})".format(PanelItem, ItemsNumber, OpMode)
        )
        return 0

    def SetFindList(self, PanelItem, ItemsNumber):
        log.debug("VFS.SetFindList({0}, {1})".format(PanelItem, ItemsNumber))
        return 0

    def ProcessEvent(self, Event, Param):
        log.debug("VFS.ProcessEvent({0}, {1})".format(Event, Param))
        return 0

    def ProcessHostFile(self, PanelItem, ItemsNumber, OpMode):
        log.debug(
            "VFS.ProcessHostFile({0}, {1}, {2})".format(PanelItem, ItemsNumber, OpMode)
        )
        return 0
