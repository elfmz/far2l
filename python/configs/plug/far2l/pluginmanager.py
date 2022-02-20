import os
import os.path
import sys
import types
import configparser
import logging
import logging.config

USERHOME = os.path.expanduser('~/.config/far2l/plugins/python')

sys.path.insert(1, USERHOME)
logging.basicConfig(level=logging.INFO)

def setup():
    fname = os.path.join(USERHOME, 'logger.ini')
    if os.path.isfile(fname):
        with open(fname, "rt") as fp:
            ini = configparser.ConfigParser()
            ini.read_file(fp)
            logging.config.fileConfig(ini)

setup()

log = logging.getLogger(__name__)
log.debug('%s start' % ('*'*20))
log.debug('sys.path={0}'.format(sys.path))
log.debug('cwd={0}'.format(os.getcwd()))

def handle_exception(exc_type, exc_value, exc_traceback):
    if issubclass(exc_type, KeyboardInterrupt):
        sys.__excepthook__(exc_type, exc_value, exc_traceback)
        return

    log.error("Uncaught exception", exc_info=(exc_type, exc_value, exc_traceback))

sys.excepthook = handle_exception

from .plugin import PluginBase

# commands in shell window:
#     py:load <python modulename>
#     py:unload <registered python module name>

from .far2lcffi import ffi
ffic = ffi.dlopen(None)

class PluginManager:
    Info = None

    def __init__(self):
        self.openplugins = {}
        self.plugins = []

    def SetStartupInfo(self, Info):
        log.debug("SetStartupInfo({0:08X})".format(Info))
        self.Info = ffi.cast("struct PluginStartupInfo *", Info)

    def pluginRemove(self, name):
        log.debug("remove plugin: {0}".format(name))
        for i in range(len(self.plugins)):
            if self.plugins[i].Plugin.name == name:
                del self.plugins[i]
                del sys.modules[name]
                for i in range(len(self.plugins)):
                    self.plugins[i].Plugin.number = i
                return
        log.error("install plugin: {0} - not installed".format(name))

    def pluginInstall(self, name):
        log.debug("install plugin: {0}".format(name))
        for i in range(len(self.plugins)):
            if self.plugins[i].Plugin.name == name:
                log.error("install plugin: {0} - allready installed".format(name))
                return
        #plugin = __import__(name, globals(), locals(), [], 0)
        plugin = __import__(name)
        cls = getattr(plugin, 'Plugin', None)
        if type(cls) == type(PluginBase) and issubclass(cls, PluginBase):
            # inject plugin name
            cls.name = name
            self.plugins.append(plugin)
            for i in range(len(self.plugins)):
                self.plugins[i].Plugin.number = i
        else:
            log.error("install plugin: {0} - not a far2l python plugin".format(name))
            del sys.modules[name]

    def pluginGet(self, hPlugin):
        v = self.openplugins.get(hPlugin, None)
        if v is None:

            class Nop:
                def __getattr__(self, name):
                    log.debug("Nop."+name)
                    return self

                def __call__(self, *args):
                    log.debug("Nop({0})".format(args))
                    return None

            v = Nop
            log.error("unhandled pluginGet{0}".format(hPlugin))
        return v

    def pluginGetFrom(self, OpenFrom, Item):
        id2name = {
            ffic.OPEN_DISKMENU: "DISKMENU",
            ffic.OPEN_PLUGINSMENU: "PLUGINSMENU",
            ffic.OPEN_FINDLIST: "FINDLIST",
            ffic.OPEN_SHORTCUT: "SHORTCUT",
            ffic.OPEN_COMMANDLINE: "COMMANDLINE",
            ffic.OPEN_EDITOR: "EDITOR",
            ffic.OPEN_VIEWER: "VIEWER",
            ffic.OPEN_FILEPANEL: "FILEPANEL",
        }
        name = id2name[OpenFrom]
        log.debug("pluginGetFrom({0}, {1})".format(OpenFrom, Item))
        for plugin in self.plugins:
            openFrom = plugin.Plugin.openFrom
            log.debug("pluginGetFrom({0}, {1} : {2})".format(name in openFrom, Item, plugin.Plugin.name))
            if name in openFrom:
                if not Item:
                    return plugin
                Item -= 1
        return None

    def GetPluginInfo(self, Info):
        # log.debug("GetPluginInfo({0:08X})".format(Info))
        Info = ffi.cast("struct PluginInfo *", Info)
        self._DiskItems = []
        self._MenuItems = []
        self._ConfigItems = []
        log.debug("GetPluginInfo")
        for plugin in self.plugins:
            openFrom = plugin.Plugin.openFrom
            if "DISKMENU" in openFrom:
                self._DiskItems.append(ffi.new("wchar_t []", plugin.Plugin.label))
            if "PLUGINSMENU" in openFrom:
                self._MenuItems.append(ffi.new("wchar_t []", plugin.Plugin.label))
            if plugin.Plugin.Configure is not None:
                self._ConfigItems.append(ffi.new("wchar_t []", plugin.Plugin.label))
        self.DiskItems = ffi.new("wchar_t *[]", self._DiskItems)
        self.MenuItems = ffi.new("wchar_t *[]", self._MenuItems)
        self.ConfigItems = ffi.new("wchar_t *[]", self._ConfigItems)
        Info.Flags = ffic.PF_EDITOR | ffic.PF_VIEWER | ffic.PF_DIALOG
        Info.DiskMenuStrings = self.DiskItems
        Info.DiskMenuStringsNumber = len(self._DiskItems)
        Info.PluginMenuStrings = self.MenuItems
        Info.PluginMenuStringsNumber = len(self._MenuItems)
        Info.PluginConfigStrings = self.ConfigItems
        Info.PluginConfigStringsNumber = len(self._ConfigItems)
        Info.CommandPrefix = ffi.new("wchar_t []", "py")

    def ClosePlugin(self, hPlugin):
        log.debug("ClosePlugin %08X" % hPlugin)
        plugin = self.openplugins.get(hPlugin, None)
        if plugin is not None:
            plugin.Close()
            del self.openplugins[hPlugin]

    def Compare(self, hPlugin, PanelItem1, PanelItem2, Mode):
        log.debug("Compare({0}, {1}, {2}, {3})".format(hPlugin, PanelItem1, PanelItem2, Mode))
        plugin = self.pluginGet(hPlugin)
        return plugin.Compare(PanelItem1, PanelItem2, Mode)

    def Configure(self, ItemNumber):
        log.debug("Configure({0})".format(ItemNumber))
        for plugin in self.plugins:
            if plugin.Plugin.Configure is not None:
                if ItemNumber == 0:
                    plugin = plugin.Plugin(self, self.Info, ffi, ffic)
                    plugin.Configure()
                    return
                ItemNumber -= 1

    def DeleteFiles(self, hPlugin, PanelItem, ItemsNumber, OpMode):
        log.debug("DeleteFiles({0}, {1}, {2})".format(hPlugin, PanelItem, ItemsNumber, OpMode))
        plugin = self.pluginGet(hPlugin)
        return plugin.DeleteFiles(PanelItem, ItemsNumber, OpMode)

    def ExitFAR(self):
        log.debug("ExitFAR()")

    def FreeFindData(self, hPlugin, PanelItem, ItemsNumber):
        log.debug("FreeFindData({0}, {1}, {2})".format(hPlugin, PanelItem, ItemsNumber))
        plugin = self.pluginGet(hPlugin)
        return plugin.FreeFindData(PanelItem, ItemsNumber)

    def FreeVirtualFindData(self, hPlugin, PanelItem, ItemsNumber):
        log.debug("FreeVirtualData({0}, {1}, {2})".format(hPlugin, PanelItem, ItemsNumber))
        plugin = self.pluginGet(hPlugin)
        return plugin.FreeVirtualFindData(PanelItem, ItemsNumber)

    def GetFiles(self, hPlugin, PanelItem, ItemsNumber, Move, DestPath, OpMode):
        log.debug("GetFiles({0}, {1}, {2}, {3}, {4}, {5})".format(hPlugin, PanelItem, ItemsNumber, Move, DestPath, OpMode))
        plugin = self.pluginGet(hPlugin)
        return plugin.GetFiles(PanelItem, ItemsNumber, Move, DestPath, OpMode)

    def GetFindData(self, hPlugin, PanelItem, ItemsNumber, OpMode):
        log.debug("GetFindData({0}, {1}, {2}, {3})".format(hPlugin, PanelItem, ItemsNumber, OpMode))
        plugin = self.pluginGet(hPlugin)
        return plugin.GetFindData(PanelItem, ItemsNumber, OpMode)

    def GetMinFarVersion(self):
        log.debug("GetMinFarVersion()")

    def GetOpenPluginInfo(self, hPlugin, OpenInfo):
        log.debug("GetOpenPluginInfo({0}, {1},)".format(hPlugin, OpenInfo))
        plugin = self.pluginGet(hPlugin)
        return plugin.GetOpenPluginInfo(OpenInfo)

    def GetVirtualFindData(self, hPlugin, PanelItem, ItemsNumber, Path):
        log.debug("GetVirtualFindData({0}, {1}, {2}, {3})".format(hPlugin, PanelItem, ItemsNumber, Path))
        plugin = self.pluginGet(hPlugin)
        return plugin.GetVirtualFindData(PanelItem, ItemsNumber, Path)

    def MakeDirectory(self, hPlugin, Name, OpMode):
        log.debug("MakeDirectory({0}, {1}, {2})".format(hPlugin, Name, OpMode))
        plugin = self.pluginGet(hPlugin)
        return plugin.MakeDirectory(Name, OpMode)

    def OpenFilePlugin(self, Name, Data, DataSize, OpMode):
        log.debug("OpenFilePlugin({0}, {1}, {2}, {3})".format(Name, Data, DataSize, OpMode))

    def OpenPlugin(self, OpenFrom, Item):
        log.debug("OpenPlugin({0}, {1})".format(OpenFrom, Item))
        if OpenFrom == ffic.OPEN_COMMANDLINE:
            line = ffi.string(ffi.cast("wchar_t *", Item))
            log.debug("cmd:{0}".format(line))
            line = line.lstrip()
            linesplit = line.split(' ', 1)
            if linesplit[0] == "unload":
                if len(linesplit) > 1:
                    self.pluginRemove(linesplit[1])
                else:
                    log.debug("missing plugin name in py:unload <plugin name>")
            elif linesplit[0] == "load":
                if len(linesplit) > 1:
                    self.pluginInstall(linesplit[1])
                else:
                    log.debug("missing plugin name in py:load <plugin name>")
            else:
                for plugin in self.plugins:
                    log.debug("{0} | {1}".format(plugin.Plugin.name, plugin.Plugin.label))
                    if plugin.Plugin.HandleCommandLine(linesplit[0]) is True:
                        plugin = plugin.Plugin(self, self.Info, ffi, ffic)
                        plugin.CommandLine(line)
                        break
            return
        plugin = self.pluginGetFrom(OpenFrom, Item)
        if plugin is not None:
            plugin = plugin.Plugin(self, self.Info, ffi, ffic)
            rc = plugin.OpenPlugin(OpenFrom)
            if rc not in (-1, None):
                rc = id(plugin)
                self.openplugins[rc] = plugin
        else:
            rc = None
        return rc

    def ProcessDialogEvent(self, Event, Param):
        # log.debug("ProcessDialogEvent({0}, {1}))".format(Event, Param))
        pass

    def ProcessEditorEvent(self, Event, Param):
        # log.debug("ProcessEditorEvent({0}, {1})".format(Event, Param))
        pass

    def ProcessEditorInput(self, Rec):
        log.debug("ProcessEditorInput({0})".format(Rec))

    def ProcessEvent(self, hPlugin, Event, Param):
        # log.debug("ProcessEvent({0}, {1}, {2})".format(hPlugin, Event, Param))
        plugin = self.pluginGet(hPlugin)
        return plugin.ProcessEvent(Event, Param)

    def ProcessHostFile(self, hPlugin, PanelItem, ItemsNumber, OpMode):
        log.debug("ProcessHostFile({0}, {1}, {2}, {3})".format(hPlugin, PanelItem, ItemsNumber, OpMode))
        plugin = self.pluginGet(hPlugin)
        return plugin.ProcessHostFile(PanelItem, ItemsNumber, OpMode)

    def ProcessKey(self, hPlugin, Key, ControlState):
        log.debug("ProcessKey({0}, {1}, {2})".format(hPlugin, Key, ControlState))
        plugin = self.pluginGet(hPlugin)
        return plugin.ProcessKey(Key, ControlState)

    def ProcessSynchroEvent(self, Event, Param):
        # log.debug("ProcessSynchroEvent({0}, {1})".format(Event, Param))
        pass

    def ProcessViewerEvent(self, Event, Param):
        # log.debug("ProcessViewerEvent({0}, {1})".format(Event, Param))
        pass

    def PutFiles(self, hPlugin, PanelItem, ItemsNumber, Move, SrcPath, OpMode):
        log.debug("PutFiles({0}, {1}, {2}, {3}, {4}, {5})".format(hPlugin, PanelItem, ItemsNumber, Move, SrcPath, OpMode))
        plugin = self.pluginGet(hPlugin)
        return plugin.PutFiles(PanelItem, ItemsNumber, Move, SrcPath, OpMode)

    def SetDirectory(self, hPlugin, Dir, OpMode):
        log.debug("SetDirectory({0}, {1}, {2})".format(hPlugin, Dir, OpMode))
        plugin = self.pluginGet(hPlugin)
        return plugin.SetDirectory(Dir, OpMode)

    def SetFindList(self, hPlugin, PanelItem, ItemsNumber):
        log.debug("SetFindList({0}, {1}, {2})".format(hPlugin, PanelItem, ItemsNumber))
        plugin = self.pluginGet(hPlugin)
        return plugin.SetFindList(PanelItem, ItemsNumber)
