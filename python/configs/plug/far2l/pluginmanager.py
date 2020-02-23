import os
import cffi
from . import rpdb

def installffi(ffi):
    print('installffi(', __file__, ')')
    dname = '/'.join(__file__.split('/')[:-1])
    for fname in (
        'farwin.h',
        'farcolor.hpp',
        'farkeys.hpp',
        'plugin.hpp'
    ):
        data = open(os.path.join(dname, fname), 'rt').read()
        ffi.cdef(data, packed=True)

ffi = cffi.FFI()
installffi(ffi)
#ffi.cdef(open(__file__+'.h', 'rt').read(), packed=True)
ffic = ffi.dlopen('c')

from . import (
    udialog,
    ucharmap,
)

class PluginManager:
    Info = None

    def __init__(self):
        self.plugins = []
        self.menu2plugin = {}
        self.conf2plugin = {}
        self.cmd2plugin = {}
        self.openplugins = {}

        self.AddPlugin(udialog)
        self.AddPlugin(ucharmap)

    def AddPlugin(self, plugin):
        n = len(self.plugins)
        self.plugins.append(plugin)
        self.menu2plugin[n] = plugin
        self.conf2plugin[n] = plugin
        self.cmd2plugin[n] = plugin

    def debugger(self):
        # in another window type: nc 127.0.0.1 7654
        rpdb.RemotePdb(host='127.0.0.1', port=7654).set_trace()

    def SetStartupInfo(self, Info):
        #print('SetStartupInfo %08X' % Info)
        self.Info = ffi.cast("struct PluginStartupInfo *", Info)

    def GetPluginInfo(self, Info):
        #print('GetPluginInfo %08X' % Info)
        Info = ffi.cast("struct PluginInfo *", Info)
        self._MenuItems = []
        self._ConfigItems = []
        for plugin in self.plugins:
            if plugin.Plugin.menu:
                self._MenuItems.append(ffi.new("wchar_t []", plugin.Plugin.menu))
            if plugin.Plugin.conf:
                self._ConfigItems.append(ffi.new("wchar_t []", plugin.Plugin.conf))
        self.MenuItems = ffi.new("wchar_t *[]", self._MenuItems)
        self.ConfigItems = ffi.new("wchar_t *[]", self._ConfigItems)
        Info.Flags = ffic.PF_EDITOR |ffic.PF_VIEWER |ffic.PF_DIALOG
        Info.DiskMenuStringsNumber = 0
        Info.PluginMenuStrings = self.MenuItems
        Info.PluginMenuStringsNumber = len(self._MenuItems)
        Info.PluginConfigStrings = self.ConfigItems
        Info.PluginConfigStringsNumber = len(self._ConfigItems)
        Info.CommandPrefix = ffi.new("wchar_t []", "py")

    def ClosePlugin(self, hPlugin):
        print('ClosePlugin %08X' % hPlugin)

    def Compare(self, hPlugin, PanelItem1, PanelItem2, Mode):
        print('Compare', hPlugin, PanelItem1, PanelItem2, Mode)

    def Configure(self, ItemNumber):
        print('Configure', ItemNumber)

    def DeleteFiles(self, hPlugin, PanelItem, ItemsNumber, OpMode):
        print('DeleteFiles', hPlugin, PanelItem, ItemsNumber, OpMode)

    def ExitFAR(self):
        print('ExitFAR')

    def FreeFindData(self, hPlugin, PanelItem, ItemsNumber):
        print('FreeFindData', hPlugin, PanelItem, ItemsNumber)

    def FreeVirtualFindData(self, hPlugin, PanelItem, ItemsNumber):
        print('FreeVirtualData', hPlugin, PanelItem, ItemsNumber)

    def GetFiles(self, hPlugin, PanelItem, ItemsNumber, Move, DestPath, OpMode):
        print('GetFiles', hPlugin, PanelItem, ItemsNumber, Move, DestPath, OpMode)

    def GetFindData(self, hPlugin, PanelItem, ItemsNumber, OpMode):
        print('GetFindData', hPlugin, PanelItem, ItemsNumber, OpMode)

    def GetMinFarVersion(self):
        print('GetMinFarVersion')

    def GetOpenPluginInfo(self, hPlugin, OpenInfo):
        print('GetOpenPluginInfo', hPlugin, OpenInfo)

    def GetVirtualFindData(self, hPlugin, PanelItem, ItemsNumber, Path):
        print('GetVirtualFindData', hPlugin, PanelItem, ItemsNumber, Path)

    def MakeDirectory(self, hPlugin, Name, OpMode):
        print('MakeDirectory', hPlugin, Name, OpMode)

    def OpenFilePlugin(self, Name, Data, DataSize, OpMode):
        print('OpenFilePlugin', Name, Data, DataSize, OpMode)

    def OpenPlugin(self, OpenFrom, Item):
        if OpenFrom == ffic.OPEN_COMMANDLINE:
            line = ffi.string(ffi.cast("wchar_t *", Item))
            print('OpenFrom:', OpenFrom, line)
            plugin = self.cmd2plugin[0].Plugin(self, self.Info, ffi, ffic)
            rc = plugin.HandleCommandLine(line)
            if rc not in (-1, None):
                rc = id(plugin)
                self.openplugins[rc] = plugin
            return rc
        print('OpenPlugin', OpenFrom, Item)
        plugin = self.cmd2plugin[Item].Plugin(self, self.Info, ffi, ffic)
        rc = plugin.OpenPlugin(OpenFrom)
        if rc not in (-1, None):
            rc = id(plugin)
            self.openplugins[rc] = plugin
        return rc

    def ProcessDialogEvent(self, Event, Param):
        #print('ProcessDialogEvent', Event, Param)
        pass

    def ProcessEditorEvent(self, Event, Param):
        #print('ProcessEditorEvent', Event, Param)
        pass

    def ProcessEditorInput(self, Rec):
        print('ProcessEditorInput', Rec)

    def ProcessEvent(self, hPlugin, Event, Param):
        #print('ProcessEvent', hPlugin, Event, Param)
        pass

    def ProcessHostFile(self, hPlugin, PanelItem, ItemsNumber, OpMode):
        print('ProcessHostFile', hPlugin, PanelItem, ItemsNumber, OpMode)

    def ProcessKey(self, hPlugin, Key, ControlState):
        print('ProcessKey', hPlugin, Key, ControlState)

    def ProcessSynchroEvent(self, Event, Param):
        #print('ProcessSynchroEvent', Event, Param)
        pass

    def ProcessViewerEvent(self, Event, Param):
        #print('ProcessViewerEvent', Event, Param)
        pass

    def PutFiles(self, hPlugin, PanelItem, ItemsNumber, Move, SrcPath, OpMode):
        print('PutFiles', hPlugin, PanelItem, ItemsNumber, Move, SrcPath, OpMode)

    def SetDirectory(self, hPlugin, Dir, OpMode):
        print('SetDirectory', hPlugin, Dir, OpMode)

    def SetFindList(self, hPlugin, PanelItem, ItemsNumber):
        print('SetFindList', hPlugin, PanelItem, ItemsNumber)
