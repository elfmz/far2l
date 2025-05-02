import os
import logging
from far2l.plugin import PluginVFS


log = logging.getLogger(__name__)


class Screen:
    def __init__(self, parent):
        self.parent = parent
        self.hScreen = None

    def __enter__(self):
        self.hScreen = self.parent.info.SaveScreen(0, 0, -1, -1)

    def __exit__(self, *args):
        self.parent.info.RestoreScreen(self.hScreen)


class Plugin(PluginVFS):
    label = "Python upanel"
    openFrom = ["PLUGINSMENU", "DISKMENU"]

    def OpenPlugin(self, OpenFrom):
        self.Root = "PythonPanel"
        self.names = []
        self.Items = []
        return True

    def GetOpenPluginInfo(self, OpenInfo):
        Info = self.ffi.cast("struct OpenPluginInfo *", OpenInfo)
        Info.Flags = (
            self.ffic.OPIF_USEFILTER
            | self.ffic.OPIF_USESORTGROUPS
            | self.ffic.OPIF_USEHIGHLIGHTING
            | self.ffic.OPIF_ADDDOTS
            | self.ffic.OPIF_SHOWNAMESONLY
        )
        Info.HostFile = self.ffi.NULL
        Info.CurDir = self.s2f(self.Root)
        Info.Format = self.s2f(self.label)
        Info.PanelTitle = self.s2f(self.label)

    def GetFindData(self, PanelItem, ItemsNumber, OpMode):
        log.debug(
            "VFS.GetFindData({0}, {1}, {2})".format(PanelItem, ItemsNumber, OpMode)
        )
        PanelItem = self.ffi.cast("struct PluginPanelItem **", PanelItem)
        ItemsNumber = self.ffi.cast("int *", ItemsNumber)
        n = len(self.Items)
        p = self.ffi.NULL if n == 0 else self.Items
        PanelItem[0] = p
        ItemsNumber[0] = n
        return 1

    def SetDirectory(self, Dir, OpMode):
        if OpMode & self.ffic.OPM_FIND:
            return 0
        if self.f2s(Dir) == "":
            self.info.Control(self.hplugin, self.ffic.FCTL_CLOSEPLUGIN, 0, 0)
        else:
            self.info.Control(self.hplugin, self.ffic.FCTL_CLOSEPLUGIN, 0, Dir)
        return 1

    def PutFiles(self, PanelItem, ItemsNumber, Move, SrcPath, OpMode):
        PanelItem = self.ffi.cast("struct PluginPanelItem *", PanelItem)
        SrcPath = self.f2s(SrcPath)
        for i in range(ItemsNumber):
            self.PutFile(PanelItem[i], Move, SrcPath, OpMode)
        return 1

    def PutFile(self, PanelItem, Move, SrcPath, OpMode):
        fqname = os.path.join(SrcPath, self.f2s(PanelItem.FindData.lpwszFileName))
        # increase C array
        items = self.ffi.new("struct PluginPanelItem []", len(self.Items) + 1)
        for i in range(len(self.Items)):
            items[i] = self.Items[i]
        self.Items = items
        # append element
        self.names.append(self.s2f(fqname))
        i = len(self.Items) - 1
        items[i].FindData = PanelItem.FindData
        items[i].FindData.lpwszFileName = self.names[i]

    def DeleteFiles(self, PanelItem, ItemsNumber, OpMode):
        item = self.ffi.cast("struct PluginPanelItem *", PanelItem)
        snames = []
        for i in range(ItemsNumber):
            snames.append(self.f2s(item[i].FindData.lpwszFileName))
        found = []
        for i in range(len(self.names)):
            if self.f2s(self.names[i]) in snames:
                found.append(i)
        if len(found) == 0:
            return 0
        # new array
        items = self.ffi.new("struct PluginPanelItem []", len(self.Items) - len(found))
        j = 0
        for i in range(len(self.Items)):
            if i not in found:
                items[j] = self.Items[i]
                j += 1
        # delete
        for i in sorted(found, reverse=True):
            del self.names[i]
        self.Items = items
        self.info.Control(self.hplugin, self.ffic.FCTL_UPDATEPANEL, 0, 0)
        self.info.Control(self.hplugin, self.ffic.FCTL_REDRAWPANEL, 0, 0)
        return 0

    def GetFiles(self, PanelItem, ItemsNumber, Move, DestPath, OpMode):
        if ItemsNumber == 0 or Move:
            return 0
        item = self.ffi.cast("struct PluginPanelItem *", PanelItem)
        DestPath = self.ffi.cast("wchar_t **", DestPath)
        dpath = self.ffi.string(DestPath[0])
        for i in range(ItemsNumber):
            sqname = self.f2s(item[i].FindData.lpwszFileName)
            dqname = os.path.join(dpath, sqname.split("/")[-1])
            # log.debug('GetFiles OpMode={} source={} destination={}'.format(OpMode, sqname, dqname))
            # just copy file, local filesystem only
            data = open(sqname, "rb").read()
            open(dqname, "wb").write(data)
        return 1
