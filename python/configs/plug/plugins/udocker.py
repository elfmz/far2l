import os
import stat
import time
import logging

from far2l.plugin import PluginVFS
from far2l.farprogress import ProgressMessage
from far2l.fardialogbuilder import (
    Spacer,
    TEXT,
    EDIT,
    PASSWORD,
    MASKED,
    MEMOEDIT,
    BUTTON,
    CHECKBOX,
    RADIOBUTTON,
    COMBOBOX,
    LISTBOX,
    USERCONTROL,
    HLine,
    HSizer,
    VSizer,
    DialogBuilder,
)

from udockerio import Docker

log = logging.getLogger(__name__)

class Plugin(PluginVFS):
    label = "Python Docker"
    openFrom = ["PLUGINSMENU", "DISKMENU"]

    def OpenPlugin(self, OpenFrom):
        self.names = []
        self.Items = []
        self.clt = Docker()
        self.device = None
        self.devicepath = "/"
        return True

    def devStart(self, name):
        self.clt.start(name)

    def devStop(self, name):
        self.clt.stop(name)

    def devLoadDevices(self):
        for d in self.clt.list():
            if d[2]:
                self.addName(d[1], self.ffic.FILE_ATTRIBUTE_DIRECTORY, 0)
            else:
                self.addName(d[1], self.ffic.FILE_ATTRIBUTE_OFFLINE, 0)

    def devSelectDevice(self, name):
        self.device = None
        for d in self.clt.list():
            if d[1] == name:
                self.device = d[0]
        self.devicepath = "/"

    def loadDirectory(self):
        result = self.clt.ls(self.device, self.devicepath)
        self.addResult(result)

    def devGetFile(self, sqname, dqname):
        self.clt.pull(self.device, sqname, dqname)

    def devPutFile(self, sqname, dqname):
        self.clt.push(self.device, sqname, dqname)

    def devDelete(self, dqname):
        self.clt.remove(self.device, dqname)

    def devMakeDirectory(self, dqname):
        self.clt.mkdir(self.device, dqname)

    def setName(self, i, name, attr, size):
        self.names[i] = self.s2f(name)
        item = self.Items[i].FindData
        item.dwFileAttributes = attr
        item.nFileSize = size
        item.lpwszFileName = self.names[i]
        return item

    def addName(self, name, attr, size):
        # log.debug('addName({})'.format(name))
        # increase C array
        items = self.ffi.new("struct PluginPanelItem []", len(self.Items) + 1)
        for i in range(len(self.Items)):
            items[i] = self.Items[i]
        self.Items = items
        self.names.append(None)
        self.setName(len(self.Items) - 1, name, attr, size)

    def addResult(self, result):
        n = 0
        for rec in result:
            if rec.st_name in (".", ".."):
                continue
            n += 1
        self.Items = self.ffi.new("struct PluginPanelItem []", n)
        self.names = [None] * n
        i = 0
        for rec in result:
            if rec.st_name in (".", ".."):
                continue
            attr = 0
            attr |= self.ffic.FILE_ATTRIBUTE_DIRECTORY if rec.st_mode & stat.S_IFDIR else 0
            attr |= self.ffic.FILE_ATTRIBUTE_DEVICE if rec.st_mode & stat.S_IFCHR else 0
            attr |= self.ffic.FILE_ATTRIBUTE_ARCHIVE if rec.st_mode & stat.S_IFREG else 0
            # log.debug('{} mode={:5o} attr={} perms={}'.format(rec.name, rec.mode, attr, rec.perms))
            # datetime.datetime.fromtimestamp(rec.time).strftime('%Y-%m-%d %H:%M:%S')
            item = self.setName(i, rec.st_name, attr, rec.st_size)
            item.dwUnixMode = rec.st_mode

            t = (
                int(time.mktime(rec.st_mtime.timetuple())) + self.ffic.EPOCH_DIFFERENCE
            ) * self.ffic.TICKS_PER_SECOND
            item.ftLastWriteTime.dwHighDateTime = t >> 32
            item.ftLastWriteTime.dwLowDateTime = t & 0xFFFFFFFF
            i += 1

    def deleteNames(self, names):
        found = []
        for i in range(len(self.names)):
            if self.f2s(self.names[i]) in names:
                found.append(i)
        if len(found) == 0:
            return False
        # new array
        n = len(self.Items) - len(found)
        if n > 0:
            items = self.ffi.new("struct PluginPanelItem []", n)
            j = 0
            for i in range(len(self.Items)):
                if i not in found:
                    items[j] = self.Items[i]
                    j += 1
            # delete
            for i in sorted(found, reverse=True):
                del self.names[i]
        else:
            self.names = []
            items = []
        self.Items = items
        self.info.Control(self.hplugin, self.ffic.FCTL_UPDATEPANEL, 0, 0)
        self.info.Control(self.hplugin, self.ffic.FCTL_REDRAWPANEL, 0, 0)
        return True

    def Message(self, body, title="Docker", flags=0):
        items = []
        if title:
            items.extend([
                title,
            ])
        items.extend([s for s in body.split('\n')])
        items = [self.s2f(s) for s in items]
        MsgItems = self.ffi.new("wchar_t *[]", items)
        rc = self.info.Message(
            self.info.ModuleNumber,  # GUID
            flags,  # Flags
            self.ffi.NULL,  # HelpTopic
            MsgItems,  # Items
            len(MsgItems),  # ItemsNumber
            1,  # ButtonsNumber
        )
        return rc

    def GetOpenPluginInfo(self, OpenInfo):
        if self.device is not None:
            title = "docker://{}{}".format(self.device, self.devicepath)
        else:
            title = self.label
        #
        # WARNING WARNING - dangerous pointers and lifetime of a variable in python
        # anything passed as a pointer must be kept in local python variables
        # otherwise gc/cffi will delete them and miracles will happen
        #
        self._curdir = self.s2f(self.devicepath)
        self._title = self.s2f(title)
        self._label = self.s2f(self.label)

        Info = self.ffi.cast("struct OpenPluginInfo *", OpenInfo)
        Info.Flags = (
            self.ffic.OPIF_USEFILTER
            | self.ffic.OPIF_USESORTGROUPS
            | self.ffic.OPIF_USEHIGHLIGHTING
            | self.ffic.OPIF_ADDDOTS
            | self.ffic.OPIF_SHOWNAMESONLY
        )
        Info.HostFile = self.ffi.NULL
        Info.CurDir = self._curdir
        Info.Format = self._label
        Info.PanelTitle = self._title
        # const struct InfoPanelLine *InfoLines;
        # int                   InfoLinesNumber;
        # const wchar_t * const   *DescrFiles;
        # int                   DescrFilesNumber;
        # const struct PanelMode *PanelModesArray;
        Info.PanelModesNumber = 0
        Info.StartPanelMode = 0
        # Info.StartSortMode = self.ffic.SM_NAME
        # Info.StartSortOrder = 0
        # const struct KeyBarTitles *KeyBar;
        # Info.ShortcutData = self.s2f('py:adb cd '+title)

    def GetFindData(self, PanelItem, ItemsNumber, OpMode):
        # super().GetFindData(PanelItem, ItemsNumber, OpMode)
        if OpMode & self.ffic.OPM_FIND:
            # find in root of device causes core dump on linux
            return False
        if self.device is None:
            try:
                self.devLoadDevices()
            except Exception as ex:
                log.exception("unknown exception:")
                self.Message(str(ex), "Unknown exception.", self.ffic.FMSG_MB_OK|self.ffic.FMSG_WARNING)
                return False
        else:
            try:
                self.loadDirectory()
            except Exception as ex:
                log.exception("unknown exception:")
                self.Message(str(ex), "Unknown exception.", self.ffic.FMSG_MB_OK|self.ffic.FMSG_WARNING)
                return False
        PanelItem = self.ffi.cast("struct PluginPanelItem **", PanelItem)
        ItemsNumber = self.ffi.cast("int *", ItemsNumber)
        n = len(self.Items)
        p = self.ffi.NULL if n == 0 else self.Items
        PanelItem[0] = p
        ItemsNumber[0] = n
        return True

    def FreeFindData(self, PanelItem, ItemsNumber):
        # log.debug("FreeFindData({0}, {1}, n.names={2}, n.Items={3})".format(PanelItem, ItemsNumber, len(self.names), len(self.Items)))
        self.names = []
        self.Items = []

    def SetDirectory(self, Dir, OpMode):
        # super().SetDirectory(Dir, OpMode)
        # if OpMode & self.ffic.OPM_FIND:
        #    return False
        name = self.f2s(Dir)
        # log.debug('goto.0: devicepath={} name={}'.format(self.devicepath, name))
        if name == "":
            self.info.Control(self.hplugin, self.ffic.FCTL_CLOSEPLUGIN, 0, 0)
            return True
        # log.debug('goto.1: devicepath={} name={}'.format(self.devicepath, name))
        if name == "..":
            if self.device is None:
                self.info.Control(self.hplugin, self.ffic.FCTL_CLOSEPLUGIN, 0, Dir)
                return True
            if self.devicepath == "/":
                self.device = None
            else:
                self.devicepath = os.path.normpath(os.path.join(self.devicepath, name))
            # log.debug('goto.2: devicepath={}'.format(self.devicepath))
        elif self.device is None:
            try:
                self.devSelectDevice(name)
            except Exception as ex:
                self.device = None
                log.exception("unknown exception:")
                self.Message(str(ex), "Unknown exception.", self.ffic.FMSG_MB_OK|self.ffic.FMSG_WARNING)
                return False
        else:
            self.devicepath = os.path.join(self.devicepath, name)
        return True

    def Compare(self, PanelItem1, PanelItem2, Mode):
        def cmp(a, b):
            if a < b:
                return -1
            elif a == b:
                return 0
            return 1

        p1 = self.ffi.cast("struct PluginPanelItem *", PanelItem1)
        p2 = self.ffi.cast("struct PluginPanelItem *", PanelItem2)
        n1 = self.f2s(p1.FindData.lpwszFileName)
        n2 = self.f2s(p2.FindData.lpwszFileName)
        rc = cmp(n1, n2)
        # log.debug('Compare: cmp({}, {})={}, mode={}'.format(n1, n2, rc, Mode))
        return rc

    def GetVirtualFindData(self, PanelItem, ItemsNumber, Path):
        return False

    def FreeVirtualFindData(self, PanelItem, ItemsNumber):
        log.debug("FreeVirtualFindData: {}, {}".format(PanelItem, ItemsNumber))

    def ProcessEvent(self, Event, Param):
        # if Event == self.ffic.FE_IDLE:
        pass

    def GetFiles(self, PanelItem, ItemsNumber, Move, DestPath, OpMode):
        # super().GetFiles(PanelItem, ItemsNumber, Move, DestPath, OpMode)
        if ItemsNumber == 0 or Move:
            return False
        items = self.ffi.cast("struct PluginPanelItem *", PanelItem)
        if self.device is None:
            name = self.f2s(items[ItemsNumber-1].FindData.lpwszFileName)
            yesno = self.Message(f"Device '{name}' is not started.\nStart it ?", flags=self.ffic.FMSG_MB_YESNO)
            if yesno == 0:
                # yes
                log.debug(f'Start device: {name}')
                try:
                    self.devStart(name)
                except Exception as ex:
                    self.device = None
                    log.exception("start device:")
                    self.Message(str(ex), f"Start device '{name}' failed", self.ffic.FMSG_MB_OK|self.ffic.FMSG_WARNING)
                    return False
                return True
            return True
        # TODO dialog with copy parameters
        # TODO progress dialog
        # TODO copy in background

        t = ProgressMessage(self, "Progress demo", "Please wait ... working", 100)
        t.show()
        DestPath = self.ffi.cast("wchar_t **", DestPath)
        dpath = self.ffi.string(DestPath[0])
        # log.debug('GetFiles: {} {} OpMode={}'.format(ItemsNumber, OpMode, dpath))
        for i in range(ItemsNumber):
            t.update(i)
            name = self.f2s(items[i].FindData.lpwszFileName)
            if name in (".", ".."):
                continue
            sqname = os.path.join(self.devicepath, name)
            dqname = os.path.join(dpath, name)
            # log.debug('pull: {} -> {} OpMode={}'.format(sqname, dqname, OpMode))
            try:
                self.devGetFile(sqname, dqname)
            except Exception as ex:
                t.close()
                log.exception("unknown exception:")
                self.Message(str(ex), "Unknown exception.", self.ffic.FMSG_MB_OK|self.ffic.FMSG_WARNING)
                return False
        t.close()
        return True

    def PutFiles(self, PanelItem, ItemsNumber, Move, SrcPath, OpMode):
        # super().PutFiles(PanelItem, ItemsNumber, Move, SrcPath, OpMode)
        if ItemsNumber == 0 or Move:
            return False
        if self.device is None:
            self.Message("PutFiles allowed only inside device.", flags=self.ffic.FMSG_MB_OK|self.ffic.FMSG_WARNING)
            return True
        items = self.ffi.cast("struct PluginPanelItem *", PanelItem)
        spath = self.f2s(SrcPath)
        for i in range(ItemsNumber):
            name = self.f2s(items[i].FindData.lpwszFileName)
            if name in (".", ".."):
                continue
            sqname = os.path.join(spath, name)
            dqname = os.path.join(self.devicepath, name)
            # log.debug('push: {} -> {} OpMode={}'.format(sqname, dqname, OpMode))
            try:
                self.devPutFile(sqname, dqname)
            except Exception as ex:
                log.exception("unknown exception:")
                self.Message(str(ex), "Unknown exception.", self.ffic.FMSG_MB_OK|self.ffic.FMSG_WARNING)
                return False
        return True

    def DeleteFiles(self, PanelItem, ItemsNumber, OpMode):
        # super().DeleteFiles(PanelItem, ItemsNumber, OpMode)
        if ItemsNumber == 0:
            return False
        items = self.ffi.cast("struct PluginPanelItem *", PanelItem)
        if self.device is None:
            name = self.f2s(items[ItemsNumber-1].FindData.lpwszFileName)
            yesno = self.Message(f"Device '{name}' is started.\nStop it ?", flags=self.ffic.FMSG_MB_YESNO)
            if yesno == 0:
                # yes
                log.debug(f'Stop device: {name}')
                try:
                    self.devStop(name)
                except Exception as ex:
                    self.device = None
                    log.exception("stopt device:")
                    self.Message(str(ex), f"Stop device '{name}' failed", self.ffic.FMSG_MB_OK|self.ffic.FMSG_WARNING)
                    return False
                return True
            return True
        if ItemsNumber > 1:
            yesno = self.Message("Do you wish to delete the files ?", flags=self.ffic.FMSG_MB_YESNO)
        else:
            name = self.f2s(items[0].FindData.lpwszFileName)
            yesno = self.Message(f"Do you wish to delete the file:\n{name}", flags=self.ffic.FMSG_MB_YESNO)
        if yesno == 1:
            # no
            return True
        for i in range(ItemsNumber):
            name = self.f2s(items[i].FindData.lpwszFileName)
            if name in (".", ".."):
                continue
            dqname = os.path.join(self.devicepath, name)
            log.debug("remove: {}, OpMode={}".format(dqname, OpMode))
            try:
                self.devDelete(dqname)
            except Exception as ex:
                log.exception("unknown exception:")
                self.Message(str(ex), "Unknown exception.", self.ffic.FMSG_MB_OK|self.ffic.FMSG_WARNING)
                return False
        return True

    def MakeDirectory(self, Name, OpMode):
        # super().DMakeDirectory(Name, OpMode)
        if self.device is None:
            self.Message("Make directory allowed only inside device.", flags=self.ffic.FMSG_MB_OK|self.ffic.FMSG_WARNING)
            return True
        name = self.ffi.cast("wchar_t **", Name)
        name = self.ffi.string(name[0])

        @self.ffi.callback("FARWINDOWPROC")
        def DialogProc(hDlg, Msg, Param1, Param2):
            if Msg == self.ffic.DN_INITDIALOG:
                log.debug("INITDIALOG")
                try:
                    dlg.SetText(dlg.ID_path, name)
                    dlg.SetFocus(dlg.ID_path)
                except:
                    log.exception("bang")
                log.debug("/INITDIALOG")
            elif Msg == self.ffic.DN_BTNCLICK:
                pass
            elif Msg == self.ffic.DN_KEY:
                if Param2 == self.ffic.KEY_ESC:
                    return 0
                if Param2 == self.ffic.KEY_LEFT:
                    pass
                elif Param2 == self.ffic.KEY_UP:
                    pass
                elif Param2 == self.ffic.KEY_RIGHT:
                    pass
                elif Param2 == self.ffic.KEY_DOWN:
                    pass
                elif Param2 == self.ffic.KEY_ENTER:
                    pass
                elif Param2 == self.ffic.KEY_ESC:
                    pass
            elif Msg == self.ffic.DN_MOUSECLICK:
                pass
            return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)

        b = DialogBuilder(
            self,
            DialogProc,
            "Make folder",
            "makefolder",
            0,
            VSizer(
                TEXT(None, "Create the &folder"),
                EDIT("path", 36, maxlength=40),
                HLine(),
                HSizer(
                    BUTTON("OK", "OK", default=True, flags=self.ffic.DIF_CENTERGROUP),
                    BUTTON("CANCEL", "Cancel", flags=self.ffic.DIF_CENTERGROUP),
                ),
            ),
        )
        dlg = b.build(-1, -1)

        res = self.info.DialogRun(dlg.hDlg)
        if res == dlg.ID_OK:
            path = dlg.GetText(dlg.ID_path).strip()
            dqname = os.path.normpath(os.path.join(self.devicepath, path))
            log.debug("mkdir {}".format(dqname))
            if path:
                try:
                    self.devMakeDirectory(dqname)
                except Exception as ex:
                    log.exception("unknown exception:")
                    self.Message(str(ex), "Unknown exception.", self.ffic.FMSG_MB_OK|self.ffic.FMSG_WARNING)
                    return False
        self.info.DialogFree(dlg.hDlg)

        return True

    def Execute(self):
        pass

    def ProcessKey(self, Key, ControlState):
        # log.debug("ProcessKey({0}, {1})".format(Key, ControlState))
        if ControlState == self.ffic.PKF_CONTROL:
            log.debug("ProcessKey(0x{:x}, {})".format(Key, ControlState))
        # if (
        #    Key == 0x80051 # FARMACRO_KEY_EVENT = 0x80000|1...
        #    and ControlState == self.ffic.PKF_CONTROL
        # ):
        #    # 0x80051 = CTRL+Q = quick view, here its equal to core dump when scanning /proc directory
        #    return True
        if (
            False
            and Key == self.ffic.KEY_CTRLA - self.ffic.KEY_CTRL
            and ControlState == self.ffic.PKF_CONTROL
        ):
            log.debug("ProcessKey: CTRL+A")
            # self.EditAttributes()
            return True
        if (
            Key == self.ffic.KEY_ENTER
            and ControlState == self.ffic.PKF_CONTROL
            and self.config.rexec
        ):
            log.debug("ProcessKey: CTRL+ENTER")
            self.Execute()
            return True
        return False
