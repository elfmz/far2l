import os
import tempfile
import logging
import sqlite3
from far2l.plugin import PluginVFS
from far2l import fardialogbuilder as fdb

log = logging.getLogger(__name__)


FILE_ATTRIBUTE_DIRECTORY = 0x00000010

PanelModeBrief          = 0
PanelModeMedium         = 1
PanelModeFull           = 2
PanelModeWide           = 3
PanelModeDetailed       = 4
PanelModeDiz            = 5
PanelModeLongDiz        = 6
PanelModeOwners         = 7
PanelModeLinks          = 8
PanelModeAlternative    = 9
PanelModeMax            = 10


class Screen:
    def __init__(self, parent):
        self.parent = parent
        self.hScreen = None

    def __enter__(self):
        self.hScreen = self.parent.info.SaveScreen(0, 0, -1, -1)

    def __exit__(self, *args):
        self.parent.info.RestoreScreen(self.hScreen)


class SqlData:
    def __init__(self, parent, conn):
        self.parent = parent
        self.ffi = parent.ffi
        self.ffic = parent.ffic
        self.s2f = parent.s2f
        self.f2s = parent.f2s

        self.conn = conn
        self.names = []
        self.Items = []
        self.rows = []

    def close(self):
        self.conn = None
        self.parent = None

    def GetCurrentPanelItem(self):
        handle = self.ffi.cast("HANDLE", -1)
        # get buffer size
        rc = self.parent.info.Control(handle, self.ffic.FCTL_GETCURRENTPANELITEM, 0, 0)
        if rc:
            # allocate buffer
            data = self.ffi.new("char []", rc)
            rc = self.parent.info.Control(handle, self.ffic.FCTL_GETCURRENTPANELITEM, 0, self.ffi.cast("LONG_PTR", data))
            # cast buffer to PluginPanelItem *
            pnli = self.ffi.cast("struct PluginPanelItem *", data)
            return pnli, data
        return None, None

class SqlDataHandler(SqlData):
    def __init__(self, parent, conn, tablename):
        super().__init__(parent, conn)
        self.tablename = tablename
        self.offset = 0
        self.limit = 100
        self.getdata()

    def getdata(self):
        stmt = '''\
select
    rowid,
    *
from {}
order by
    rowid
limit {}, {}
'''.format(self.tablename, self.offset, self.limit)
        #log.debug('stmt:{}'.format(stmt))
        cur = self.conn.cursor()
        res = cur.execute(stmt)
        self.desc = res.description
        self.rows = res.fetchall()
        #log.debug('data:{}'.format(self.rows))
        cur.close()
        #log.debug('desc:{}'.format(self.desc))

    def GetOpenPluginInfo(self, OpenInfo):
        #log.debug("usqlite.SqlDataHandler.GetOpenPluginInfo")
        self.paneltitle = self.parent.label+': {}/{}'.format(self.parent.dbname.split('/')[-1], self.tablename)

        ColumnTypes = []
        ColumnWidths = []
        py_pm_py_titles = []
        for n in range(len(self.desc)-1):
            desc = self.desc[n+1]
            py_pm_py_titles.append(self.s2f(desc[0]))
            ColumnTypes.append(f'C{n}')
            ColumnWidths.append('5')
        ColumnWidths[-1] = '0'
        ColumnTypes = ','.join(ColumnTypes)
        ColumnWidths = ','.join(ColumnWidths)
        self.py_pm_titles = self.ffi.new("wchar_t *[]", py_pm_py_titles)
        py_pm = [
            self.s2f(ColumnTypes),  # ColumnTypes
            self.s2f(ColumnWidths), # ColumnWidths
            self.py_pm_titles,      # ColumnTitles
            0,                      # FullScreen
            0,                      # DetailedStatus
            0,                      # AlignExtensions
            0,                      # CaseConversion
            self.ffi.NULL,          # StatusColumnTypes
            self.ffi.NULL,          # StatusColumnWidths
            [0,0],                  # Reserved
        ]
        self.pm = self.ffi.new("struct PanelMode *", py_pm)

        wcn = self.ffi.cast("wchar_t *", self.ffi.NULL)
        py_kbt_normal = [wcn]*12
        py_kbt_ctrl = [wcn]*12
        py_kbt_alt = [wcn]*12
        py_kbt_ctrl_shift = [wcn]*12
        py_kbt_alt_shift = [wcn]*12
        py_kbt_ctrl_alt = [wcn]*12

        py_kbt_normal[4-1]=self.s2f("Edit Row")
        py_kbt_normal[6-1]=self.s2f("+100")
        py_kbt_normal[7-1]=self.s2f("-100")

        self.py_kbt = [
            py_kbt_normal,
            py_kbt_ctrl,
            py_kbt_alt,
            py_kbt_ctrl_shift,
            py_kbt_alt_shift,
            py_kbt_ctrl_alt
        ]
        self.kbt = self.ffi.new("struct KeyBarTitles *", self.py_kbt)

        Info = self.ffi.cast("struct OpenPluginInfo *", OpenInfo)
        Info.Flags = (
            self.ffic.OPIF_USEFILTER
            | self.ffic.OPIF_ADDDOTS
            | self.ffic.OPIF_SHOWNAMESONLY
        )
        Info.HostFile = self.ffi.NULL
        Info.CurDir = self.s2f(self.parent.dbname.split('/')[-1])
        Info.Format = self.s2f(self.paneltitle)
        Info.PanelTitle = self.s2f(self.paneltitle)
        #InfoLines
        #InfoLinesNumber
        Info.PanelModesArray = self.pm
        Info.PanelModesNumber = 1
        Info.StartPanelMode = PanelModeDetailed
        Info.StartSortMode = self.ffic.SM_UNSORTED
        #Info.StartSortOrder
        Info.KeyBar = self.kbt
        #Info.ShortcutData
        #Info.Reserved

    def GetFindData(self, PanelItem, ItemsNumber, OpMode):
        #log.debug("usqlite.SqlDataHandler.GetFindData({0}, {1}, {2})".format(PanelItem, ItemsNumber, OpMode))

        self.Items = []
        self.names = []
        if len(self.rows):
            names = []
            items = self.ffi.new("struct PluginPanelItem []", len(self.rows))
            for no in range(len(self.rows)):
                rec = self.rows[no]
                items[no].FindData.nPhysicalSize = rec[0]
                fields = []
                names.append([])
                #log.debug(f'{rec}')
                for fldno in range(1, len(rec)):
                    v = rec[fldno]
                    if isinstance(v, str):
                        fields.append(v)
                    elif isinstance(v, int):
                        fields.append(str(v))
                    else:
                        fields.append(str(v))
                    names[-1].append(self.s2f(fields[-1]))
                names.append(self.ffi.new("wchar_t *[]", names[-1]))
                items[no].CustomColumnData = names[-1]
                items[no].CustomColumnNumber = len(names[-2])
            self.Items = items
            self.names = names

        PanelItem = self.ffi.cast("struct PluginPanelItem **", PanelItem)
        ItemsNumber = self.ffi.cast("int *", ItemsNumber)
        n = len(self.Items)
        p = self.ffi.NULL if n == 0 else self.Items
        PanelItem[0] = p
        ItemsNumber[0] = n
        #log.debug('/usqlite.SqlMetadataHandler.GetFindData')
        return 1

    def FreeFindData(self, PanelItem, ItemsNumber):
        #log.debug("usqlite.SqlDataHandler.FreeFindData({0}, {1}, n.names={2}, n.Items={3})".format(PanelItem, ItemsNumber, len(self.names), len(self.Items)))
        self.Items = []
        self.names = []

    def SetDirectory(self, dirname):
        if dirname == '..':
            self.parent.dbhandler = SqlMetadataHandler(self.parent, self.parent.dbconn)
            return 1
        return 0

    def EditRow(self, rec):
        #log.debug(f'EditRec: {rec}')

        @self.ffi.callback("FARWINDOWPROC")
        def DialogProc(hDlg, Msg, Param1, Param2):
            return self.parent.info.DefDlgProc(hDlg, Msg, Param1, Param2)

        fields = []
        for fldno in range(1, len(rec)):
            name = self.desc[fldno]
            v = rec[fldno]
            if isinstance(v, str):
                pass
            elif isinstance(v, int):
                v = str(v)
            else:
                v = str(v)
            if len(v) > 64:
                v = v[:64]+' ...'
            fields.append(
                fdb.HSizer(
                    fdb.TEXT(None, self.desc[fldno][0]+':'),
                    fdb.TEXT(self.desc[fldno][0], v),
                )
            )
        fields.append(
            fdb.HLine()
        )
        fields.append(
            fdb.HSizer(
                fdb.BUTTON("vcancel", "Cancel", default=True, flags=self.ffic.DIF_CENTERGROUP),
#                fdb.BUTTON("vupdate", "Update", flags=self.ffic.DIF_CENTERGROUP),
            )
        )
        b = fdb.DialogBuilder(
            self.parent,
            DialogProc,
            f"SQLite rec: {self.tablename}/{rec[0]}",
            "helptopic",
            0,
            fdb.VSizer(
                *list(fields)
            ),
        )
        dlg = b.build(-1, -1)
        res = self.parent.info.DialogRun(dlg.hDlg)
        log.debug(f'EditRec: res={res}')
        if res not in (-1, dlg.ID_vcancel):
            #dlg.GetText(dlg.ID_vapath),
            #log.debug(f'saverec: {rec}')
            pass
        self.parent.info.DialogFree(dlg.hDlg)

    def ProcessKey(self, Key, ControlState):
        #log.debug('usqlite.SqlDataHandler.ProcessKey: key:{:x} state:{:x} f4:{}'.format(Key, ControlState, Key == self.ffic.VK_F4))
        if ControlState == 0:
            if Key == self.ffic.VK_F4:
                pnli, pnlidata = self.GetCurrentPanelItem()
                if pnli:
                    rowid = pnli.FindData.nPhysicalSize
                    for rec in self.rows:
                        if rec[0] == rowid:
                            self.EditRow(rec)
                            break
                return True
            elif Key == self.ffic.VK_F6:
                #log.debug('F6')
                self.offset += self.limit
                self.getdata()
                if len(self.rows) == 0:
                    self.offset -= self.limit
                    self.getdata()
                handle = self.ffi.cast("HANDLE", -1)
                self.parent.info.Control(handle, self.ffic.FCTL_UPDATEPANEL, 0, 0)
                self.parent.info.Control(handle, self.ffic.FCTL_REDRAWPANEL, 0, 0)
                return True
            elif Key == self.ffic.VK_F7:
                #log.debug('F7')
                self.offset -= self.limit
                if self.offset < 0:
                    self.offset = 0
                self.getdata()
                handle = self.ffi.cast("HANDLE", -1)
                self.parent.info.Control(handle, self.ffic.FCTL_UPDATEPANEL, 0, 0)
                self.parent.info.Control(handle, self.ffic.FCTL_REDRAWPANEL, 0, 0)
                return True
        return False



class SqlMetadataHandler(SqlData):
    def GetOpenPluginInfo(self, OpenInfo):
        #log.debug("usqlite.SqlMetadataHandler.GetOpenPluginInfo")
        py_pm_py_titles = [
            self.s2f("name"),
            self.s2f("type"),
            self.s2f("count"),
        ]
        self.py_pm_titles = self.ffi.new("wchar_t *[]", py_pm_py_titles)
        py_pm = [
            self.s2f("N,C0,C1"),    # ColumnTypes
            self.s2f("0,5,6"),      # ColumnWidths
            self.py_pm_titles,      # ColumnTitles
            0,                      # FullScreen
            0,                      # DetailedStatus
            0,                      # AlignExtensions
            0,                      # CaseConversion
            self.ffi.NULL,          # StatusColumnTypes
            self.ffi.NULL,          # StatusColumnWidths
            [0,0],                  # Reserved
        ]
        self.pm = self.ffi.new("struct PanelMode *", py_pm)

        wcn = self.ffi.cast("wchar_t *", self.ffi.NULL)
        py_kbt_normal = [wcn]*12
        py_kbt_ctrl = [wcn]*12
        py_kbt_alt = [wcn]*12
        py_kbt_ctrl_shift = [wcn]*12
        py_kbt_alt_shift = [wcn]*12
        py_kbt_ctrl_alt = [wcn]*12

        py_kbt_normal[4-1]=self.s2f("DDL")

        self.py_kbt = [
            py_kbt_normal,
            py_kbt_ctrl,
            py_kbt_alt,
            py_kbt_ctrl_shift,
            py_kbt_alt_shift,
            py_kbt_ctrl_alt
        ]
        self.kbt = self.ffi.new("struct KeyBarTitles *", self.py_kbt)

        Info = self.ffi.cast("struct OpenPluginInfo *", OpenInfo)
        Info.Flags = (
            self.ffic.OPIF_USEFILTER
            | self.ffic.OPIF_ADDDOTS
            | self.ffic.OPIF_SHOWNAMESONLY
        )
        Info.HostFile = self.ffi.NULL
        Info.CurDir = self.s2f(self.parent.dbname.split('/')[-1])
        Info.Format = self.s2f(self.parent.label)
        Info.PanelTitle = self.s2f(self.parent.label)
        #InfoLines
        #InfoLinesNumber
        Info.PanelModesArray = self.pm
        Info.PanelModesNumber = 1
        Info.StartPanelMode = PanelModeDetailed
        Info.StartSortMode = self.ffic.SM_UNSORTED
        #Info.StartSortOrder
        Info.KeyBar = self.kbt
        #Info.ShortcutData
        #Info.Reserved

    def GetFindData(self, PanelItem, ItemsNumber, OpMode):
        #log.debug("usqlite.SqlMetadataHandler.GetFindData({0}, {1}, {2})".format(PanelItem, ItemsNumber, OpMode))

        stmt = '''\
select
    name, type, sql
from sqlite_master
'''
        cur = self.conn.cursor()
        res = cur.execute(stmt)
        rows = res.fetchall()
        cur.close()

        if len(rows):
            names = []
            items = self.ffi.new("struct PluginPanelItem []", len(rows))
            for no in range(len(rows)):
                rec = rows[no]
                names.append(self.s2f(rec[0])) # name
                items[no].FindData.lpwszFileName = names[-1]
                if rec[1] in ('table', 'view'):
                    items[no].FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY
                    cur = self.conn.cursor()
                    res = cur.execute('select count(*) as cnt from '+rec[0])
                    n = str(res.fetchone()[0])
                    cur.close()
                else:
                    n = '--'
                names.append(n)
                names.append([self.s2f(rec[1]), self.s2f(names[-1])]) # type, size
                names.append(self.ffi.new("wchar_t *[]", names[-1]))
                items[no].CustomColumnData = names[-1]
                items[no].CustomColumnNumber = 2
            self.Items = items
            self.names = names
            self.rows = rows

        PanelItem = self.ffi.cast("struct PluginPanelItem **", PanelItem)
        ItemsNumber = self.ffi.cast("int *", ItemsNumber)
        n = len(self.Items)
        p = self.ffi.NULL if n == 0 else self.Items
        PanelItem[0] = p
        ItemsNumber[0] = n
        #log.debug('/usqlite.SqlMetadataHandler.GetFindData')
        return 1

    def FreeFindData(self, PanelItem, ItemsNumber):
        #log.debug("usqlite.SqlMetadataHandler.FreeFindData({0}, {1}, n.names={2}, n.Items={3})".format(PanelItem, ItemsNumber, len(self.names), len(self.Items)))
        self.Items = []
        self.names = []

    def SetDirectory(self, dirname):
        if dirname == '..':
            self.parent.info.Control(self.parent.hplugin, self.ffic.FCTL_CLOSEPLUGIN, 0, 0)
            return 1
        for rec in self.rows:
            if rec[0] == dirname and rec[1] in ('table', 'view'):
                self.parent.dbhandler = SqlDataHandler(self.parent, self.parent.dbconn, dirname)
                return 1
        return 0

    def ProcessKey(self, Key, ControlState):
        #log.debug('usqlite.SqlMetadataHandler.ProcessKey: key:{:x} state:{:x} f4:{}'.format(Key, ControlState, Key == self.ffic.VK_F4))
        if Key == self.ffic.VK_F4:
            if ControlState == 0:
                pnli, pnlidata = self.GetCurrentPanelItem()
                if pnli:
                    name = self.f2s(pnli.FindData.lpwszFileName)
                    #log.debug('F4: rc={} {}'.format(rc, name))
                    for rec in self.rows:
                        if rec[0] == name:
                            fd, fname = tempfile.mkstemp(suffix='.sql', prefix=None, dir='/tmp', text=True)
                            os.close(fd)
                            with open(fname, 'wt') as fp:
                                fp.write(rec[2])
                            self.parent.info.Editor(
                                fname,
                                fname,
                                0, 0, -1, -1,
                                0,#self.ffic.EF_DELETEONCLOSE,
                                0,
                                0,
                                0xFFFFFFFF,  # =-1=self.ffic.CP_AUTODETECT
                            )
                            os.unlink(fname)
                            break
                return True
            #elif ControlState & self.ffic.PKF_SHIFT:
        return False



class Plugin(PluginVFS):
    label = "Python usqlite"
    openFrom = ["PLUGINSMENU", "DISKMENU"]

    dbname = ''

    def __init__(self, parent, info, ffi, ffic, name=None, conn=None):
        super().__init__(parent, info, ffi, ffic)
        self.dbname = name
        self.dbconn = conn
        self.dbhandler = SqlMetadataHandler(self, self.dbconn)

    def Close(self):
        if self.dbhandler:
            self.dbhandler.close()
        if self.dbconn:
            self.dbconn.close()
        self.dbconn = None
        self.dbhandler = None
        super().Close()

    @staticmethod
    def OpenFilePlugin(parent, info, ffi, ffic, Name, Data, DataSize, OpMode):
        data = b''.join(ffi.cast("char *", Data)[0:min(DataSize, 16)])
        if data != b'SQLite format 3\0':
            return False
        name = ffi.string(ffi.cast("wchar_t *", Name))
        try:
            conn = sqlite3.connect(name)
        except:
            return False
        #log.debug('{} - {}'.format(Plugin.label, name))
        self = Plugin(parent, info, ffi, ffic, name, conn)
        return self

    def GetOpenPluginInfo(self, OpenInfo):
        return self.dbhandler.GetOpenPluginInfo(OpenInfo)

    def GetFindData(self, PanelItem, ItemsNumber, OpMode):
        return self.dbhandler.GetFindData(PanelItem, ItemsNumber, OpMode)

    def FreeFindData(self, PanelItem, ItemsNumber):
        return self.dbhandler.FreeFindData(PanelItem, ItemsNumber)

    def SetDirectory(self, Dir, OpMode):
        if OpMode & self.ffic.OPM_FIND:
            return 0
        dirname = self.f2s(Dir)
        #log.debug('usqlite.SetDirectory({}, {})'.format(dirname, self.dbhandler))
        return self.dbhandler.SetDirectory(dirname)

    def ProcessKey(self, Key, ControlState):
        #log.debug("ProcessKey({0}, {1})".format(Key, ControlState))
        return self.dbhandler.ProcessKey(Key, ControlState)
