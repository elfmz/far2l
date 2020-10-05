import os
import cffi
from sys import platform
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
        data = open(os.path.join(dname, fname), 'rt', encoding='utf-8').read()
        ffi.cdef(data, packed=True)

ffi = cffi.FFI()
installffi(ffi)
#ffi.cdef(open(__file__+'.h', 'rt').read(), packed=True)
ffic = ffi.dlopen('c' if platform != 'darwin' else 'libSystem.dylib')

from . import (
    udialog,
    ucharmap,
    upanel,
)

class PluginManager:
    Info = None

    def __init__(self):
        self.openplugins = {}
        self.plugins = []

        self.AddPlugin(udialog)
        self.AddPlugin(ucharmap)
        self.AddPlugin(upanel)

    def AddPlugin(self, plugin):
        self.plugins.append(plugin)

    def debugger(self):
        # in another window type: nc 127.0.0.1 7654
        rpdb.RemotePdb(host='127.0.0.1', port=7654).set_trace()

    def SetStartupInfo(self, Info):
        #print('SetStartupInfo %08X' % Info)
        self.Info = ffi.cast("struct PluginStartupInfo *", Info)

    def area2where(self, area):
        cvt = {
            'disk': ffic.OPEN_DISKMENU,
            'plugins': ffic.OPEN_PLUGINSMENU,
            'find': ffic.OPEN_FINDLIST,
            'shortcut': ffic.OPEN_SHORTCUT,
            'shell': ffic.OPEN_COMMANDLINE,
            'editor': ffic.OPEN_EDITOR,
            'viewer': ffic.OPEN_VIEWER,
            'panel': ffic.OPEN_FILEPANEL,
            'dialog': ffic.OPEN_DIALOG,
            'analyse': ffic.OPEN_ANALYSE,
        }
        where = 0
        for area in area.split():
            where |= 1<<cvt[area.lower()]
        return where

    def getplugin(self, hPlugin):
        v = self.openplugins.get(hPlugin, None)
        if v is None:
            class Nop:
                def __getattr__(self, name):
                    print('Nop.', name)
                    return self
                def __call__(self, *args):
                    print('Nop(', name,')')
                    return None
            v = Nop
        return v

    def GetPluginInfo(self, Info):
        #print('GetPluginInfo %08X' % Info)
        Info = ffi.cast("struct PluginInfo *", Info)
        self._DiskItems = []
        self._MenuItems = []
        self._ConfigItems = []
        print('GetPluginInfo')
        for plugin in sorted(self.plugins, key=lambda plugin: plugin.Plugin.label):
            where = self.area2where(plugin.Plugin.area)
            print(plugin.Plugin.label, '%08X'%where)
            if where & 1<<ffic.OPEN_DISKMENU:
                self._DiskItems.append(ffi.new("wchar_t []", plugin.Plugin.label))
            if plugin.Plugin.conf:
                self._ConfigItems.append(ffi.new("wchar_t []", plugin.Plugin.label))
            # find
            # shortcut
            if where & 1<<ffic.OPEN_PLUGINSMENU:
                self._MenuItems.append(ffi.new("wchar_t []", plugin.Plugin.label))
            elif where & 1<<ffic.OPEN_COMMANDLINE:
                self._MenuItems.append(ffi.new("wchar_t []", plugin.Plugin.label))
            elif where & 1<<ffic.OPEN_EDITOR:
                self._MenuItems.append(ffi.new("wchar_t []", plugin.Plugin.label))
            elif where & 1<<ffic.OPEN_VIEWER:
                self._MenuItems.append(ffi.new("wchar_t []", plugin.Plugin.label))
            elif where & 1<<ffic.OPEN_FILEPANEL:
                self._MenuItems.append(ffi.new("wchar_t []", plugin.Plugin.label))
            elif where & 1<<ffic.OPEN_DIALOG:
                self._MenuItems.append(ffi.new("wchar_t []", plugin.Plugin.label))
            # analyse
        self.DiskItems = ffi.new("wchar_t *[]", self._DiskItems)
        self.MenuItems = ffi.new("wchar_t *[]", self._MenuItems)
        self.ConfigItems = ffi.new("wchar_t *[]", self._ConfigItems)
        Info.Flags = ffic.PF_EDITOR |ffic.PF_VIEWER |ffic.PF_DIALOG
        Info.DiskMenuStrings = self.DiskItems
        Info.DiskMenuStringsNumber = len(self._DiskItems)
        Info.PluginMenuStrings = self.MenuItems
        Info.PluginMenuStringsNumber = len(self._MenuItems)
        Info.PluginConfigStrings = self.ConfigItems
        Info.PluginConfigStringsNumber = len(self._ConfigItems)
        Info.CommandPrefix = ffi.new("wchar_t []", "py")

    def ClosePlugin(self, hPlugin):
        print('ClosePlugin %08X' % hPlugin)
        plugin = self.openplugins.get(hPlugin, None)
        if plugin is not None:
            plugin.Close()
            del self.openplugins[hPlugin]

    def Compare(self, hPlugin, PanelItem1, PanelItem2, Mode):
        print('Compare', hPlugin, PanelItem1, PanelItem2, Mode)
        plugin = self.getplugin(hPlugin)
        return plugin.Compare(PanelItem1, PanelItem2, Mode)

    def Configure(self, ItemNumber):
        print('Configure', ItemNumber)

    def DeleteFiles(self, hPlugin, PanelItem, ItemsNumber, OpMode):
        print('DeleteFiles', hPlugin, PanelItem, ItemsNumber, OpMode)
        plugin = self.getplugin(hPlugin)
        return plugin.DeleteFiles(PanelItem, ItemsNumber, OpMode)

    def ExitFAR(self):
        print('ExitFAR')

    def FreeFindData(self, hPlugin, PanelItem, ItemsNumber):
        print('FreeFindData', hPlugin, PanelItem, ItemsNumber)
        plugin = self.getplugin(hPlugin)
        return plugin.FreeFindData(PanelItem, ItemsNumber)

    def FreeVirtualFindData(self, hPlugin, PanelItem, ItemsNumber):
        print('FreeVirtualData', hPlugin, PanelItem, ItemsNumber)
        plugin = self.getplugin(hPlugin)
        return plugin.FreeVirtualFindData(PanelItem, ItemsNumber)

    def GetFiles(self, hPlugin, PanelItem, ItemsNumber, Move, DestPath, OpMode):
        print('GetFiles', hPlugin, PanelItem, ItemsNumber, Move, DestPath, OpMode)
        plugin = self.getplugin(hPlugin)
        return plugin.GetFiles(PanelItem, ItemsNumber, Move, DestPath, OpMode)

    def GetFindData(self, hPlugin, PanelItem, ItemsNumber, OpMode):
        print('GetFindData', hPlugin, PanelItem, ItemsNumber, OpMode)
        plugin = self.getplugin(hPlugin)
        return plugin.GetFindData(PanelItem, ItemsNumber, OpMode)

    def GetMinFarVersion(self):
        print('GetMinFarVersion')

    def GetOpenPluginInfo(self, hPlugin, OpenInfo):
        print('GetOpenPluginInfo', hPlugin, OpenInfo)
        plugin = self.getplugin(hPlugin)
        return plugin.GetOpenPluginInfo(OpenInfo)

    def GetVirtualFindData(self, hPlugin, PanelItem, ItemsNumber, Path):
        print('GetVirtualFindData', hPlugin, PanelItem, ItemsNumber, Path)
        plugin = self.getplugin(hPlugin)
        return plugin.GetVirtualFindData(PanelItem, ItemsNumber, Path)

    def MakeDirectory(self, hPlugin, Name, OpMode):
        print('MakeDirectory', hPlugin, Name, OpMode)
        plugin = self.getplugin(hPlugin)
        return plugin.MakeDirectory(PanelItem, Name, OpMode)

    def OpenFilePlugin(self, Name, Data, DataSize, OpMode):
        print('OpenFilePlugin', Name, Data, DataSize, OpMode)

    def OpenPlugin(self, OpenFrom, Item):
        print('OpenPlugin:', OpenFrom, Item)
        if OpenFrom == ffic.OPEN_COMMANDLINE:
            line = ffi.string(ffi.cast("wchar_t *", Item))
            print('cmd:', line)
            return
        this = None
        no = 0
        for plugin in sorted(self.plugins, key=lambda plugin: plugin.Plugin.label):
            where = self.area2where(plugin.Plugin.area)
            print(1, plugin.Plugin.label, '|', plugin.Plugin.area, '|', '%04X'%(1<<OpenFrom), 'where:', '%04X'%where, 'mask:', (1<<OpenFrom)&where, 'no:', no, 'Item', Item)
            if (1<<OpenFrom) & where:
                if no == Item:
                    this = plugin
                    break
                no += 1
        if this is None:
            for plugin in sorted(self.plugins, key=lambda plugin: plugin.Plugin.label):
                where = self.area2where(plugin.Plugin.area)
                print(2, plugin.Plugin.label, '|', plugin.Plugin.area, '|', '%04X'%(1<<OpenFrom), 'where:', '%04X'%where, 'mask:', (1<<OpenFrom)&where, 'no:', no, 'Item', Item)
                if where & 1<<ffic.OPEN_PLUGINSMENU:
                    if no == Item:
                        this = plugin
                        break
                    no += 1
        if not this:
            print('unknown')
            return
        plugin = this.Plugin(self, self.Info, ffi, ffic)
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
        plugin = self.getplugin(hPlugin)
        return plugin.ProcessEvent(Event, Param)

    def ProcessHostFile(self, hPlugin, PanelItem, ItemsNumber, OpMode):
        print('ProcessHostFile', hPlugin, PanelItem, ItemsNumber, OpMode)
        plugin = self.getplugin(hPlugin)
        return plugin.ProcessHostFile(PanelItem, ItemsNumber, OpMode)

    def ProcessKey(self, hPlugin, Key, ControlState):
        print('ProcessKey', hPlugin, Key, ControlState)
        plugin = self.getplugin(hPlugin)
        return plugin.ProcessKey(Key, ControlState)

    def ProcessSynchroEvent(self, Event, Param):
        #print('ProcessSynchroEvent', Event, Param)
        pass

    def ProcessViewerEvent(self, Event, Param):
        #print('ProcessViewerEvent', Event, Param)
        pass

    def PutFiles(self, hPlugin, PanelItem, ItemsNumber, Move, SrcPath, OpMode):
        print('PutFiles', hPlugin, PanelItem, ItemsNumber, Move, SrcPath, OpMode)
        plugin = self.getplugin(hPlugin)
        return plugin.PutFiles(PanelItem, ItemsNumber, Move, SrcPath, OpMode)

    def SetDirectory(self, hPlugin, Dir, OpMode):
        print('SetDirectory', hPlugin, Dir, OpMode)
        plugin = self.getplugin(hPlugin)
        return plugin.SetDirectory(Dir, OpMode)

    def SetFindList(self, hPlugin, PanelItem, ItemsNumber):
        print('SetFindList', hPlugin, PanelItem, ItemsNumber)
        plugin = self.getplugin(hPlugin)
        return plugin.SetFindList(PanelItem, ItemsNumber)
