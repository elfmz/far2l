import os
import time
import stat
import re
import logging
from datetime import datetime
import configparser

from far2l.plugin import PluginVFS
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

from adbutils import AdbClient, AdbError

log = logging.getLogger(__name__)

TICKS_PER_SECOND = 10000000
EPOCH_DIFFERENCE = 11644473600

FILE_ATTRIBUTE_READONLY = 0x00000001
FILE_ATTRIBUTE_HIDDEN = 0x00000002
FILE_ATTRIBUTE_SYSTEM = 0x00000004
FILE_ATTRIBUTE_DIRECTORY = 0x00000010
FILE_ATTRIBUTE_ARCHIVE = 0x00000020
FILE_ATTRIBUTE_DEVICE = 0x00000040
FILE_ATTRIBUTE_NORMAL = 0x00000080
FILE_ATTRIBUTE_TEMPORARY = 0x00000100
FILE_ATTRIBUTE_SPARSE_FILE = 0x00000200
FILE_ATTRIBUTE_REPARSE_POINT = 0x00000400
FILE_ATTRIBUTE_COMPRESSED = 0x00000800
FILE_ATTRIBUTE_OFFLINE = 0x00001000
FILE_ATTRIBUTE_NOT_CONTENT_INDEXED = 0x00002000
FILE_ATTRIBUTE_ENCRYPTED = 0x00004000
FILE_ATTRIBUTE_INTEGRITY_STREAM = 0x00008000
FILE_ATTRIBUTE_VIRTUAL = 0x00010000
FILE_ATTRIBUTE_NO_SCRUB_DATA = 0x00020000
FILE_ATTRIBUTE_BROKEN = 0x00200000
FILE_ATTRIBUTE_EXECUTABLE = 0x00400000


class Config:
    def __init__(self, home):
        self.inipath = os.path.join(home, "adb.ini")
        self.config = configparser.ConfigParser()
        self.config["adb"] = {
            "box": "default",
            "rexec": "False",
        }
        self.load()

    def load(self):
        try:
            self._load()
        except:
            log.exception("load config")

    def _load(self):
        if os.path.isfile(self.inipath):
            with open(self.inipath, "rt") as fp:
                self.config.read_file(fp)

    def save(self):
        try:
            self._save()
        except:
            log.exception("save config")

    def _save(self):
        with open(self.inipath, "wt") as fp:
            self.config.write(fp)

    @property
    def box(self):
        return self.config.get("adb", "box")

    @box.setter
    def box(self, value):
        self.config.set("adb", "box", value)

    @property
    def rexec(self):
        return self.config.getboolean("adb", "rexec")

    @rexec.setter
    def rexec(self, value):
        self.config.set("adb", "rexec", str(value))


class Entry:
    def __init__(
        self,
        dirname=None,
        perms=None,
        links=1,
        uid=0,
        gid=0,
        size=None,
        date_time=None,
        date=None,
        name=None,
    ):
        """initialize file"""
        self.mode = 0
        self.perms = perms
        self.links = links
        self.uid = uid
        self.gid = gid
        self.size = int(size) if size else 0
        self.date_time = date_time
        self.name = name
        self.date = date

        self.dirname = dirname
        self.link_target = None
        self.filepath = os.path.join(self.dirname, self.name)
        self.perms2mode()

    def update(self, box):
        """update object fields"""
        month_num = {
            "Jan": 1,
            "Feb": 2,
            "Mar": 3,
            "Apr": 4,
            "May": 5,
            "Jun": 6,
            "Jul": 7,
            "Aug": 8,
            "Sep": 9,
            "Oct": 10,
            "Nov": 11,
            "Dec": 12,
        }
        if self.date_time:
            date = self.date_time.split()
            date = "%s-%02d-%s %s" % (date[1], month_num[date[0]], date[3], date[2])
            date = datetime.strptime(date, "%d-%m-%Y %H:%M:%S")
        else:
            date = datetime.strptime(self.date, box["date_re"])

        self.date = date
        self.date_time = date.strftime("%Y-%m-%d %H:%M")

        type = self.perms[0] if self.perms else None

        if type == "l" and " -> " in self.name:
            self._correct_link()

    def perms2mode(self):
        perms = self.perms[1:]
        lperms = len(perms)
        mode = 0
        # Check if 'perms' has the right format
        if not [x for x in perms if x not in "-rwxXsStT"] and lperms == 9:
            pos = lperms - 1
            for c in perms:
                if c in "sStT":
                    # Special modes
                    mode += (1 << pos // 3) << 9

                mode += 1 << pos if c in "rwxst" else 0
                pos -= 1
        self.mode = (
            mode
            | {
                "-": 0,
                "b": stat.S_IFBLK,
                "c": stat.S_IFCHR,
                "d": stat.S_IFDIR,
                "l": stat.S_IFLNK,
                "p": 0,  # stat.S_IF???,
                "s": 0,  # stat.S_IF???,
            }[self.perms[0]]
        )

    def _correct_link(self):
        """Canonize filename and fill the link attr"""
        try:
            name, target = self.name.split(" -> ")
        except ValueError:
            return

        self.name = name

        if not self.size:
            self.size = 0

        if target.startswith("/"):
            self.link_target = target
        else:
            self.link_target = os.path.abspath(os.path.join(self.dirname, target))
        self.filepath = os.path.join(self.dirname, self.name)

    def mk_link_relative(self):
        """Convert links to relative"""
        self.link_target = os.path.relpath(self.link_target, self.dirname)

    def __str__(self):
        if not self.name:
            return ""

        template = (
            "{mode:4o} {perms} {links:>4} {uid:<8} {gid:<8} {size:>8} {date} {name}"
        )
        return template.format(**self.__dict__)


class Shell:
    boxes = {
        "busybox": {
            "ls": "busybox ls -anL --full-time {}",
            "rls": "busybox ls -Ranl {}",
            "date_re": "%Y-%m-%d %H:%M:%S %z",
            "file_re": r"^"
            r"(?P<perms>[-bcdlps][-rwxsStT]{9})\s+"
            r"(?P<links>\d+)\s"
            r"(?P<uid>\d+)\s+"
            r"(?P<gid>\d+)\s+"
            r"(?P<size>\d+)\s"
            r"(?P<date>\d{4}-\d{2}-\d{2}\s\d{2}:\d{2}:\d{2}\s\+\d{4})\s"
            r"(?P<name>.*)",
        },
        "toolbox": {
            "ls": "toolbox ls -anl {}",
            "rls": "toolbox ls -Ranl {}",
            "date_re": "%Y-%m-%d %H:%M",
            "file_re": r"^(?P<perms>[-bcdlps][-rwxsStT]{9})\s+"
            r"(?P<links>\d+)\s"
            r"(?P<uid>\d+)\s+"
            r"(?P<gid>\d+)\s+"
            r"(?P<size>\d+)?\s"
            r"(?P<date>\d{4}-\d{2}-\d{2}\s\d{2}:\d{2})\s"
            r"(?P<name>.*)",
        },
        "toybox": {
            "ls": "toybox ls -anLc {}",
            "rls": "toybox ls -Ranl {}",
            "date_re": "%Y-%m-%d %H:%M",
            "file_re": r"^(?P<perms>[-bcdlps][-rwxsStT]{9})\s+"
            r"(?P<links>\d+)\s+"
            r"(?P<uid>\d+)\s+"
            r"(?P<gid>\d+)\s+"
            r"(?P<size>\d+)?\s"
            r"(?P<date>\d{4}-\d{2}-\d{2}\s\d{2}:\d{2})\s"
            r"(?P<name>.*)",
        },
    }

    def __init__(self, config, dev):
        box = config.box
        if box == "default":
            for name in (
                "toybox",
                "busybox",
                "toolbox",
            ):
                line = dev.shell("which " + name).strip()
                if line:
                    box = name
                    break
            else:
                raise IOError(2, "unknown shell")
        self.box = self.boxes[box]
        self.file_re = re.compile(self.box["file_re"])

    def list(self, dev, top):
        dtop = top
        if dtop[-1] != "/":
            dtop += "/"
        cmd = self.box["ls"].format(dtop)
        lines = dev.shell(cmd)
        result = []
        for line in lines.split("\n"):
            line = line.strip()
            if not line:
                continue
            rm = self.file_re.match(line)
            if not rm:
                if line.split()[0] != "total":
                    log.debug("ignored entry in ({}): {}".format(top, line))
                continue
            entry = Entry(top, **rm.groupdict())
            entry.update(self.box)
            if entry.perms[0] == "l":
                self.listfix(dev, entry, entry.link_target)
            result.append(entry)
        return result

    def listfix(self, dev, entry, target):
        lines = dev.shell("stat " + target).split("\n")
        target = lines[0].split(" -> ")
        if len(target) == 2:
            target = target[1].strip()[1:-1]
            return self.listfix(dev, entry, target)
        try:
            perms = lines[3].split(")")[0]
            perms = perms.split("(")[1]
            perms = perms.split("/")[1]
            if perms[0] != "d":
                entry.size = int(lines[1].split()[1])
            entry.perms = perms
            entry.perms2mode()
        except IndexError:
            # link to non existing element
            pass


class Plugin(PluginVFS):
    label = "Python ADB"
    openFrom = ["PLUGINSMENU", "DISKMENU"]

    def __init__(self, parent, info, ffi, ffic):
        super().__init__(parent, info, ffi, ffic)
        self.config = Config(self.USERHOME)

    def OpenPlugin(self, OpenFrom):
        self.names = []
        self.Items = []
        self.clt = AdbClient()
        self.deviceserial = None
        self.device = None
        self.devicepath = "/"
        return True

    def loadDevices(self):
        for d in self.clt.list():
            self.addName(d.serial, FILE_ATTRIBUTE_DIRECTORY, 0)

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
            if rec.name in (".", ".."):
                continue
            n += 1
        self.Items = self.ffi.new("struct PluginPanelItem []", n)
        self.names = [None] * n
        i = 0
        for rec in result:
            if rec.name in (".", ".."):
                continue
            name = rec.name.replace('\\ ', ' ')
            attr = 0
            attr |= FILE_ATTRIBUTE_DIRECTORY if rec.mode & stat.S_IFDIR else 0
            attr |= FILE_ATTRIBUTE_DEVICE if rec.mode & stat.S_IFCHR else 0
            attr |= FILE_ATTRIBUTE_ARCHIVE if rec.mode & stat.S_IFREG else 0
            # log.debug('{} mode={:5o} attr={} perms={}'.format(name, rec.mode, attr, rec.perms))
            # datetime.datetime.fromtimestamp(rec.time).strftime('%Y-%m-%d %H:%M:%S')
            item = self.setName(i, name, attr, rec.size)
            item.dwUnixMode = rec.mode

            t = (
                int(time.mktime(rec.date.timetuple())) + EPOCH_DIFFERENCE
            ) * TICKS_PER_SECOND
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

    def Message(self, lines):
        _MsgItems = [
            self.s2f("ADB"),
            self.s2f(""),
        ]
        for line in lines:
            _MsgItems.append(self.s2f(line))
        _MsgItems.extend(
            [
                self.s2f(""),
                self.s2f("\x01"),
                self.s2f("&Ok"),
            ]
        )
        # log.debug('_msgItems: %s', _MsgItems)
        MsgItems = self.ffi.new("wchar_t *[]", _MsgItems)
        self.info.Message(
            self.info.ModuleNumber,  # GUID
            self.ffic.FMSG_WARNING | self.ffic.FMSG_LEFTALIGN,  # Flags
            self.s2f("Contents"),  # HelpTopic
            MsgItems,  # Items
            len(MsgItems),  # ItemsNumber
            1,  # ButtonsNumber
        )

    def ConfirmDialog(self, title, lines):
        _MsgItems = [
            self.s2f(title),
            self.s2f(""),
        ]
        for line in lines:
            _MsgItems.append(self.s2f(line))
        _MsgItems.extend(
            [
                self.s2f(""),
                self.s2f("\x01"),
                self.s2f("&Cancel"),
                self.s2f("&Ok"),
            ]
        )
        # log.debug('_msgItems: %s', _MsgItems)
        MsgItems = self.ffi.new("wchar_t *[]", _MsgItems)
        rc = self.info.Message(
            self.info.ModuleNumber,  # GUID
            self.ffic.FMSG_WARNING | self.ffic.FMSG_LEFTALIGN,  # Flags
            self.s2f("Contents"),  # HelpTopic
            MsgItems,  # Items
            len(MsgItems),  # ItemsNumber
            2,  # ButtonsNumber
        )
        # 0 = Cancel, 1 = OK
        # log.debug('confirm={}'.format(rc))
        return rc

    @staticmethod
    def HandleCommandLine(line):
        return line in ("uadb",)

    def CommandLine(self, line):
        sline = line.split()
        if len(sline) == 2 and sline[1] == "config":
            self.Configure()
        else:
            self.Message(["Unknown command line:", line])

    def Configure(self):
        @self.ffi.callback("FARWINDOWPROC")
        def DialogProc(hDlg, Msg, Param1, Param2):
            if Msg == self.ffic.DN_INITDIALOG:
                try:
                    dlgid = {
                        "default": dlg.ID_default,
                        "busybox": dlg.ID_busybox,
                        "toybox": dlg.ID_toybox,
                        "toolbox": dlg.ID_toolbox,
                    }[self.config.box]
                    dlg.SetCheck(dlgid, self.ffic.BSTATE_CHECKED)
                    dlg.SetCheck(
                        dlg.ID_rexec,
                        self.ffic.BSTATE_CHECKED if self.config.rexec else 0,
                    )
                except:
                    log.exception("bang")
            return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)

        b = DialogBuilder(
            self,
            DialogProc,
            "ADB Config",
            "adb config",
            0,
            VSizer(
                HSizer(
                    VSizer(
                        TEXT(None, "Shell:"),
                        RADIOBUTTON("default", "default", flags=self.ffic.DIF_GROUP),
                        RADIOBUTTON("busybox", "busybox"),
                        RADIOBUTTON("toybox", "toybox"),
                        RADIOBUTTON("toolbox", "toolbox"),
                    ),
                    HSizer(
                        CHECKBOX("rexec", "Allow remote execute"),
                    ),
                ),
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
            box = 0
            for n, did in (
                (0, dlg.ID_default),
                (1, dlg.ID_busybox),
                (2, dlg.ID_toybox),
                (3, dlg.ID_toolbox),
            ):
                box |= n if dlg.GetCheck(did) else 0
            self.config.box = {
                0: "default",
                1: "busybox",
                2: "toybox",
                3: "toolbox",
            }[box]
            self.config.rexec = True if dlg.GetCheck(dlg.ID_rexec) else False
            self.config.save()

    def GetOpenPluginInfo(self, OpenInfo):
        if self.device is not None:
            title = "adb://{}{}".format(self.device.serial, self.devicepath)
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
                self.loadDevices()
            except RuntimeError as why:
                log.error(why)
                lines = str(why).split("\n")
                lines.insert(0, "RuntimeError:")
                self.Message(lines)
                return False
            except AdbError as ex:
                log.exception("AdbError")
                self.Message(str(ex).split("\n"))
                return False
            except Exception as ex:
                log.exception("unknown exception:")
                msg = str(ex).split("\n")
                msg.insert(0, "Unknown exception.")
                self.Message(msg)
                return False
        else:
            try:
                shell = Shell(self.config, self.device)
                result = shell.list(self.device, self.devicepath)
                self.addResult(result)
            except AdbError as ex:
                log.exception("AdbError")
                self.Message(str(ex).split("\n"))
                return False
            except Exception as ex:
                log.exception("unknown exception:")
                msg = str(ex).split("\n")
                msg.insert(0, "Unknown exception.")
                self.Message(msg)
                return False
        PanelItem = self.ffi.cast("struct PluginPanelItem **", PanelItem)
        ItemsNumber = self.ffi.cast("int *", ItemsNumber)
        n = len(self.Items)
        p = self.ffi.NULL if n == 0 else self.Items
        PanelItem[0] = p
        ItemsNumber[0] = n
        # log.debug("ADB.GetFindData, p={} n={}".format(p, n))
        return True

    def FreeFindData(self, PanelItem, ItemsNumber):
        # log.debug("ADB.FreeFindData({0}, {1}, n.names={2}, n.Items={3})".format(PanelItem, ItemsNumber, len(self.names), len(self.Items)))
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
                self.device = self.clt.device(name)
                self.devicepath = "/"
                try:
                    self.device.root()
                    # log.debug('goto.3: rooted')
                except:
                    # log.debug('goto.3: not rooted')
                    pass
            except AdbError as ex:
                self.device = None
                log.exception("AdbError")
                self.Message(str(ex).split("\n"))
                return False
            except Exception as ex:
                self.device = None
                log.exception("unknown exception:")
                msg = str(ex).split("\n")
                msg.insert(0, "Unknown exception.")
                self.Message(msg)
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
        if self.device is None:
            self.Message(["GetFiles allowed only inside device."])
            return True
        # TODO dialog with copy parameters
        # TODO progress dialog
        # TODO copy in background
        items = self.ffi.cast("struct PluginPanelItem *", PanelItem)
        DestPath = self.ffi.cast("wchar_t **", DestPath)
        dpath = self.ffi.string(DestPath[0])
        # log.debug('GetFiles: {} {} OpMode={}'.format(ItemsNumber, OpMode, dpath))
        for i in range(ItemsNumber):
            name = self.f2s(items[i].FindData.lpwszFileName)
            if name in (".", ".."):
                continue
            sqname = os.path.join(self.devicepath, name)
            dqname = os.path.join(dpath, name)
            # log.debug('pull: {} -> {} OpMode={}'.format(sqname, dqname, OpMode))
            try:
                self.device.sync.pull(sqname, dqname)
            except AdbError as ex:
                log.exception("AdbError")
                self.Message(str(ex).split("\n"))
                return False
            except Exception as ex:
                log.exception("unknown exception:")
                msg = str(ex).split("\n")
                msg.insert(0, "Unknown exception.")
                self.Message(msg)
                return False
        return True

    def PutFiles(self, PanelItem, ItemsNumber, Move, SrcPath, OpMode):
        # super().PutFiles(PanelItem, ItemsNumber, Move, SrcPath, OpMode)
        if ItemsNumber == 0 or Move:
            return False
        if self.device is None:
            self.Message(["PutFiles allowed only inside device."])
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
                self.device.sync.push(sqname, dqname)
            except AdbError as ex:
                log.exception("AdbError")
                self.Message(str(ex).split("\n"))
                return False
            except Exception as ex:
                log.exception("unknown exception:")
                msg = str(ex).split("\n")
                msg.insert(0, "Unknown exception.")
                self.Message(msg)
                return False
        return True

    def DeleteFiles(self, PanelItem, ItemsNumber, OpMode):
        # super().DeleteFiles(PanelItem, ItemsNumber, OpMode)
        if ItemsNumber == 0:
            return False
        if self.device is None:
            self.Message(["DeleteFiles allowed only inside device."])
            return True
        items = self.ffi.cast("struct PluginPanelItem *", PanelItem)
        if ItemsNumber > 1:
            rc = self.ConfirmDialog("Delete", ["Do you wish to delete the files"])
        else:
            name = self.f2s(items[0].FindData.lpwszFileName)
            rc = self.ConfirmDialog("Delete", ["Do you wish to delete the file", name])
        if rc != 1:
            return True
        for i in range(ItemsNumber):
            name = self.f2s(items[i].FindData.lpwszFileName)
            if name in (".", ".."):
                continue
            dqname = os.path.join(self.devicepath, name)
            log.debug("remove: {}, OpMode={}".format(dqname, OpMode))
            try:
                st = self.device.sync.stat(dqname)
                if stat.S_ISDIR(st.mode):
                    s = self.device.shell("rmdir " + dqname)
                else:
                    s = self.device.shell("rm " + dqname)
                if s:
                    log.debug("result:" + s)
                    self.Message(s.split("\n"))
            except AdbError as ex:
                log.exception("AdbError")
                self.Message(str(ex).split("\n"))
                return False
            except Exception as ex:
                log.exception("unknown exception:")
                msg = str(ex).split("\n")
                msg.insert(0, "Unknown exception.")
                self.Message(msg)
                return False
        return True

    def MakeDirectory(self, Name, OpMode):
        # super().DMakeDirectory(Name, OpMode)
        if self.device is None:
            self.Message(["Make directory allowed only inside device."])
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
            path = os.path.normpath(os.path.join(self.devicepath, path))
            log.debug("mkdir {}".format(path))
            if path:
                try:
                    s = self.device.shell("mkdir " + path)
                    if s:
                        log.debug("result:" + s)
                        self.Message(s.split("\n"))
                except AdbError as ex:
                    log.exception("AdbError")
                    self.Message(str(ex).split("\n"))
                    return False
                except Exception as ex:
                    log.exception("unknown exception:")
                    msg = str(ex).split("\n")
                    msg.insert(0, "Unknown exception.")
                    self.Message(msg)
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
