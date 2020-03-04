class PluginBase:

    conf = False
    area = None

    def __init__(self, parent, info, ffi, ffic):
        self.parent = parent
        self.info = info
        self.ffi = ffi
        self.ffic = ffic

    def s2f(self, s):
        return self.ffi.new("wchar_t []", s)

    def f2s(self, s):
        return self.ffi.string(self.ffi.cast("wchar_t *", s))

    def HandleCommandLine(self, line):
        print("Plugin.HandleCommandLine(", line, ")")

    def OpenPlugin(self, OpenFrom):
        print("Plugin.OpenPlugin(", OpenFrom, ")")

    def Close(self):
        print("Plugin.Close()")


class PluginVFS(PluginBase):

    def GetOpenPluginInfo(self, OpenInfo):
        print("VFS.GetOpenPluginInfo(", OpenInfo, ")")

    def FreeFindData(self, PanelItem, ItemsNumber):
        print("VFS.FreeFindData(", PanelItem, ",", ItemsNumber, ")")

    def FreeVirtualFindData(self, PanelItem, ItemsNumber):
        print("VFS.FreeVirtualFindData(", PanelItem, ",", ItemsNumber, ")")

    def Compare(self, PanelItem1, PanelItem2, Mode):
        print("VFS.Compare(", PanelItem1, ",", PanelItem2, ",", Mode, ")")
        return -2

    def DeleteFiles(self, PanelItem, ItemsNumber, OpMode):
        print("VFS.DeleteFiles(", PanelItem, ",", ItemsNumber, ",", OpMode, ")")
        return 0

    def GetFiles(self, PanelItem, ItemsNumber, Move, DestPath, OpMode):
        print(
            "VFS.GetFiles(",
            PanelItem,
            ",",
            ItemsNumber,
            ",",
            Move,
            ",",
            DestPath,
            ",",
            OpMode,
            ")",
        )
        return 0

    def GetFindData(self, PanelItem, ItemsNumber, OpMode):
        print("VFS.GetFindData(", PanelItem, ",", ItemsNumber, ",", OpMode, ")")
        return 0

    def GetVirtualFindData(self, PanelItem, ItemsNumber, Path):
        print("VFS.GetVirtualFindData(", PanelItem, ",", ItemsNumber, ",", Path, ")")
        return 0

    def MakeDirectory(self, PanelItem, Name, OpMode):
        print("VFS.GetMakeDirectoryFindData(", PanelItem, ",", Name, ",", OpMode, ")")
        return 0

    def ProcessEvent(self, Event, Param):
        print("VFS.ProcessEvent(", Event, ",", Param, ")")
        return 0

    def ProcessHostFile(self, PanelItem, ItemsNumber, OpMode):
        print("VFS.ProcessHostFile(", PanelItem, ",", ItemsNumber, ",", OpMode, ")")
        return 0

    def ProcessKey(self, Key, ControlState):
        print("VFS.ProcessKey(", Key, ",", ControlState, ")")
        return 0

    def PutFiles(self, PanelItem, ItemsNumber, Move, SrcPath, OpMode):
        print(
            "VFS.PutFiles(",
            PanelItem,
            ",",
            ItemsNumber,
            ",",
            Move,
            ",",
            SrcPath,
            ",",
            OpMode,
            ")",
        )
        return 0

    def SetDirectory(self, Dir, OpMode):
        print("VFS.SetDirectory(", Dir, ",", OpMode, ")")
        return 0

    def SetFindList(self, PanelItem, ItemsNumber):
        print("VFS.SetFindList(", PanelItem, ",", ItemsNumber, ")")
        return 0
