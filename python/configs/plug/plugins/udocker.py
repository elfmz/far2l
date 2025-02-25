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

try:
    # pip install runlike
    from runlike import inspector
    class Inspector(inspector.Inspector):
        def inspect(self):
            try:
                output = inspector.check_output(
                    ["docker", "container", "inspect", self.container],
                    stderr=inspector.STDOUT)
                self.container_facts = inspector.loads(output.decode('utf8', 'strict'))
                image_hash = self.get_container_fact("Image")
                output = inspector.check_output(
                    ["docker", "image", "inspect", image_hash],
                    stderr=inspector.STDOUT)
                self.image_facts = inspector.loads(output.decode('utf8', 'strict'))
            except inspector.CalledProcessError as e:
                return str(e)
            return None
except:
    Inspector = None


class DockerBase:

    label = "Python Docker"

    def __init__(self, parent):
        self.parent = parent
        self.info = parent.info
        self.hplugin = parent.hplugin
        self.ffi = parent.ffi
        self.ffic = parent.ffic
        self.s2f = parent.s2f
        self.f2s = parent.f2s
        self.clt = parent.clt

    def Close(self):
        del self.parent
        del self.info
        del self.hplugin
        del self.ffi
        del self.ffic
        del self.s2f
        del self.f2s
        del self.clt

    def OpenPlugin(self, OpenFrom):
        self.names = []
        self.Items = []
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
        #self.info.Control(self.hplugin, self.ffic.FCTL_UPDATEPANEL, 0, 0)
        #self.info.Control(self.hplugin, self.ffic.FCTL_REDRAWPANEL, 0, 0)
        return True

    def doRefresh(self):
        self.info.Control(self.hplugin, self.ffic.FCTL_UPDATEPANEL, 0, 0)
        self.info.Control(self.hplugin, self.ffic.FCTL_REDRAWPANEL, 0, 0)


class DockerDevice(DockerBase):
    def __init__(self, parent, device):
        super().__init__(parent)
        self.device = device
        self.devicepath = "/"

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

    def GetOpenPluginInfo(self, OpenInfo):
        #
        # WARNING WARNING - dangerous pointers and lifetime of a variable in python
        # anything passed as a pointer must be kept in local python variables
        # otherwise gc/cffi will delete them and miracles will happen
        #
        self.title = f"docker://{self.device}{self.devicepath}"
        self._curdir = self.s2f(self.devicepath)
        self._title = self.s2f(self.title)
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
        try:
            self.loadDirectory()
        except Exception as ex:
            log.exception("unknown exception:")
            self.Message(str(ex), "Unknown exception.", self.ffic.FMSG_MB_OK|self.ffic.FMSG_WARNING)
            return False
        return True

    def SetDirectory(self, Dir, OpMode):
        name = self.f2s(Dir)
        if name == "..":
            if self.devicepath == "/":
                self.parent.handler = DockerDevices(self.parent)
                self.Close()
            else:
                self.devicepath = os.path.normpath(os.path.join(self.devicepath, name))
        else:
            self.devicepath = os.path.join(self.devicepath, name)
        return True

    def GetFiles(self, PanelItem, ItemsNumber, Move, DestPath, OpMode):
        items = self.ffi.cast("struct PluginPanelItem *", PanelItem)
        name = self.f2s(items[ItemsNumber-1].FindData.lpwszFileName)
        t = ProgressMessage(self, "Copying", "Please wait ... working", 100)
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
                self.Message(str(ex), "GetFile exception.", self.ffic.FMSG_MB_OK|self.ffic.FMSG_WARNING)
                return False
        t.close()
        return True

    def PutFiles(self, PanelItem, ItemsNumber, Move, SrcPath, OpMode):
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
                self.Message(str(ex), "PutFile exception.", self.ffic.FMSG_MB_OK|self.ffic.FMSG_WARNING)
                return False
        return True

    def DeleteFiles(self, PanelItem, ItemsNumber, OpMode):
        items = self.ffi.cast("struct PluginPanelItem *", PanelItem)
        if ItemsNumber > 1:
            rc = self.Message("Do you wish to delete the files ?", flags=self.ffic.FMSG_MB_YESNO)
        else:
            name = self.f2s(items[0].FindData.lpwszFileName)
            rc = self.Message(f"Do you wish to delete the file:\n{name}", flags=self.ffic.FMSG_MB_YESNO)
        if rc == 1: # NO
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
        name = self.ffi.cast("wchar_t **", Name)
        name = self.ffi.string(name[0])

        b = DialogBuilder(
            self,
            self.info.DefDlgProc,
            "Make folder",
            "makefolder",
            0,
            VSizer(
                TEXT(None, "Create &folder"),
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

    def EditAttributes(self):
        item, ppidata = self.parent.panel.GetCurrentPanelItem()
        name = self.f2s(item.FindData.lpwszFileName)
        fqname = os.path.normpath(os.path.join(self.devicepath, name))
        self.Message(f"Current item: {fqname}", "TODO EditAttributes", self.ffic.FMSG_MB_OK|self.ffic.FMSG_WARNING)

    def ProcessKey(self, Key, ControlState):
        if (
            Key == self.ffic.KEY_CTRLA - self.ffic.KEY_CTRL
            and ControlState == self.ffic.PKF_CONTROL
        ):
            self.EditAttributes()
            return 1
        return 0


class DockerDevices(DockerBase):
    def __init__(self, parent):
        super().__init__(parent)
        self.devicepath = '/'

    def addResult(self, result):
        self.Items = self.ffi.new("struct PluginPanelItem []", len(result))
        self.names = [None] * len(result)
        i = 0
        for rec in result:
            if rec[2]:
                self.setName(i, rec[1], self.ffic.FILE_ATTRIBUTE_DIRECTORY, 0)
            else:
                self.setName(i, rec[1], self.ffic.FILE_ATTRIBUTE_OFFLINE, 0)
            i += 1

    def devStart(self, name):
        self.clt.start(name)

    def devStop(self, name):
        self.clt.stop(name)

    def devLoadDevices(self):
        result = self.clt.list()
        self.addResult(result)

    def GetOpenPluginInfo(self, OpenInfo):
        #
        # WARNING WARNING - dangerous pointers and lifetime of a variable in python
        # anything passed as a pointer must be kept in local python variables
        # otherwise gc/cffi will delete them and miracles will happen
        #
        self.title = self.label
        self._curdir = self.s2f(self.devicepath)
        self._title = self.s2f(self.title)
        self._label = self.s2f(self.label)

        #self._nokey = self.ffi.cast("wchar_t *", self.ffi.NULL)
        self._nokey = self.s2f("")
        py_keybar_normal = [self._nokey]*12
        py_keybar_ctrl = [self._nokey]*12
        py_keybar_alt = [self._nokey]*12
        py_keybar_ctrl_shift = [self._nokey]*12
        py_keybar_alt_shift = [self._nokey]*12
        py_keybar_ctrl_alt = [self._nokey]*12

        py_keybar_normal[2-1]=self.s2f("Ena/Dis")
        py_keybar_normal[3-1]=self.s2f("Run Like")

        self.py_keybar = [
            py_keybar_normal,
            py_keybar_ctrl,
            py_keybar_alt,
            py_keybar_ctrl_shift,
            py_keybar_alt_shift,
            py_keybar_ctrl_alt
        ]
        self._keybar = self.ffi.new("struct KeyBarTitles *", self.py_keybar)

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
        Info.KeyBar = self._keybar
        # Info.ShortcutData = self.s2f('py:adb cd '+title)

    def GetFindData(self, PanelItem, ItemsNumber, OpMode):
        try:
            self.devLoadDevices()
        except Exception as ex:
            log.exception("unknown exception:")
            self.Message(str(ex), "Unknown exception.", self.ffic.FMSG_MB_OK|self.ffic.FMSG_WARNING)
            return False
        return True

    def SetDirectory(self, Dir, OpMode):
        name = self.f2s(Dir)
        if name == "":
            self.info.Control(self.hplugin, self.ffic.FCTL_CLOSEPLUGIN, 0, 0)
            return True
        if name == "..":
            self.info.Control(self.hplugin, self.ffic.FCTL_CLOSEPLUGIN, 0, Dir)
            return True
        self.parent.handler = DockerDevice(self.parent, name)
        self.Close()
        return True

    def GetFiles(self, PanelItem, ItemsNumber, Move, DestPath, OpMode):
        self.Message("GetFiles allowed only inside device.", flags=self.ffic.FMSG_MB_OK|self.ffic.FMSG_WARNING)
        return False

    def PutFiles(self, PanelItem, ItemsNumber, Move, SrcPath, OpMode):
        self.Message("PutFiles allowed only inside device.", flags=self.ffic.FMSG_MB_OK|self.ffic.FMSG_WARNING)
        return False

    def DeleteFiles(self, PanelItem, ItemsNumber, OpMode):
        self.Message("DeleteFiles allowed only inside device.", flags=self.ffic.FMSG_MB_OK|self.ffic.FMSG_WARNING)
        return False

    def MakeDirectory(self, Name, OpMode):
        self.Message("MakeDirectory allowed only inside device.", flags=self.ffic.FMSG_MB_OK|self.ffic.FMSG_WARNING)
        return False

    def CMDStartStop(self, item, name):
        running = (item.FindData.dwFileAttributes & self.ffic.FILE_ATTRIBUTE_DIRECTORY) != 0
        proc = self.devStop if running else self.devStart
        try:
            proc(name)
            self.doRefresh()
        except Exception as ex:
            log.exception("unknown exception:")
            self.Message(str(ex), "Unknown exception.", self.ffic.FMSG_MB_OK|self.ffic.FMSG_WARNING)

    def CMDRunLike(self, item, name):
        if Inspector is None:
            self.Message("Package runlike required but not installed.", flags=self.ffic.FMSG_MB_OK|self.ffic.FMSG_WARNING)
            return
        running = (item.FindData.dwFileAttributes & self.ffic.FILE_ATTRIBUTE_DIRECTORY) != 0
        insp = Inspector(name, None, True)
        error = insp.inspect()
        if error is not None:
            self.Message(error, "RunLike error.", flags=self.ffic.FMSG_MB_OK|self.ffic.FMSG_WARNING)
            return
        open("/tmp/uinfo", "wt").write(f'''\
container name={name} running={running}

{insp.format_cli()}
''')
        self.info.Editor(
            "/tmp/uinfo",
            "uinfo",
            0, 0, -1, -1,
            #0,0,50,25,
            self.ffic.EF_DISABLEHISTORY,#|self.ffic.EF_NONMODAL|self.ffic.EF_IMMEDIATERETURN
            0,
            0,
            0xFFFFFFFF,  # =-1=self.ffic.CP_AUTODETECT
        )
        os.unlink('/tmp/uinfo')

    def ProcessKey(self, Key, ControlState):
        if Key == self.ffic.VK_F2 and ControlState == 0:
            item, ppidata = self.parent.panel.GetCurrentPanelItem()
            name = self.f2s(item.FindData.lpwszFileName)
            if name == "..":
                return
            self.CMDStartStop(item, name)
            return 1
        if Key == self.ffic.VK_F3 and ControlState == 0:
            item, ppidata = self.parent.panel.GetCurrentPanelItem()
            name = self.f2s(item.FindData.lpwszFileName)
            if name == "..":
                return
            self.CMDRunLike(item, name)
            return 1
        if Key == self.ffic.KEY_ENTER and ControlState == self.ffic.PKF_CONTROL:
            log.debug("ProcessKey: CTRL+ENTER")
            return 1
        return 0


class Plugin(PluginVFS):
    label = "Python Docker"
    openFrom = ["PLUGINSMENU", "DISKMENU"]

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.clt = Docker()
        self.handler = DockerDevices(self)

    def OpenPlugin(self, OpenFrom):
        return self.handler.OpenPlugin(OpenFrom)

    def GetOpenPluginInfo(self, OpenInfo):
        return self.handler.GetOpenPluginInfo(OpenInfo)

    def GetFindData(self, PanelItem, ItemsNumber, OpMode):
        if OpMode & self.ffic.OPM_FIND:
            # find in root of device causes core dump on linux
            return False
        ok = self.handler.GetFindData(PanelItem, ItemsNumber, OpMode)
        if ok:
            PanelItem = self.ffi.cast("struct PluginPanelItem **", PanelItem)
            ItemsNumber = self.ffi.cast("int *", ItemsNumber)
            n = len(self.handler.Items)
            p = self.ffi.NULL if n == 0 else self.handler.Items
            PanelItem[0] = p
            ItemsNumber[0] = n
        return ok

    def FreeFindData(self, PanelItem, ItemsNumber):
        log.debug(f"FreeFindData({PanelItem}, {ItemsNumber}, n.Items={len(self.handler.Items)})")
        self.handler.names = []
        self.handler.Items = []

    def SetDirectory(self, Dir, OpMode):
        return self.handler.SetDirectory(Dir, OpMode)

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

    def GetFiles(self, PanelItem, ItemsNumber, Move, DestPath, OpMode):
        if ItemsNumber == 0 or Move:
            return False
        return self.handler.GetFiles(PanelItem, ItemsNumber, Move, DestPath, OpMode)

    def PutFiles(self, PanelItem, ItemsNumber, Move, SrcPath, OpMode):
        if ItemsNumber == 0 or Move:
            return False
        return self.handler.PutFiles(PanelItem, ItemsNumber, Move, SrcPath, OpMode)

    def DeleteFiles(self, PanelItem, ItemsNumber, OpMode):
        if ItemsNumber == 0:
            return False
        return self.handler.DeleteFiles(PanelItem, ItemsNumber, OpMode)

    def MakeDirectory(self, Name, OpMode):
        return self.handler.MakeDirectory(Name, OpMode)

    def ProcessEvent(self, Event, Param):
        return 0

    def ProcessKey(self, Key, ControlState):
        return self.handler.ProcessKey(Key, ControlState)
