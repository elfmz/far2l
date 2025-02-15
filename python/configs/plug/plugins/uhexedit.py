import os
import stat
import mmap
import logging
import far2lc
from far2l import fardialogbuilder as fdb
from far2l.plugin import PluginBase
from far2l.far2ledit import Edit

log = logging.getLogger(__name__)

class FileInfo:
    def __init__(self, mmap, filename, filesize):
        self._mmap = mmap
        self._filename = filename
        self._filesize = filesize
        self._fileoffset = 0
        self._modified = {}

    @property
    def mmap(self):
        return self._mmap

    @property
    def filename(self):
        return self._filename

    @property
    def filesize(self):
        return self._filesize

    def get_fileoffset(self):
        return self._fileoffset

    def set_fileoffset(self, fileoffset):
        self._fileoffset = fileoffset

    fileoffset = property(get_fileoffset, set_fileoffset)

    @property
    def modified(self):
        return self._modified

    def modifiy(self, offset, b):
        self._modified[offset] = b


class Screen:
    MIN_WIDTH = 80
    MIN_HEIGHT = 1
    def __init__(self, plugin, width, height):
        self._plugin = plugin
        self._width = max(width, self.MIN_WIDTH)
        self._height = max(height, self.MIN_HEIGHT)
        self._buffer = None

    @property
    def plugin(self):
        return self._plugin

    @property
    def ffi(self):
        return self._plugin.ffi

    @property
    def ffic(self):
        return self._plugin.ffic

    @property
    def width(self):
        return self._width

    @property
    def height(self):
        return self._height

    @property
    def buffer(self):
        return self._buffer

    def alloc(self, color):
        self._buffer = self.ffi.new("CHAR_INFO []", self._width*self._height)
        self.fillbuffer(color)

    def fillbuffer(self, color):
        for i in range(self._width*self._height):
            self._buffer[i].Char.AsciiChar = b' '
            self._buffer[i].Attributes = color

    def write(self, row, col, val=None, attr=None):
        b = self._buffer
        start = row * self._width + col
        if val is not None:
            for i in range(len(val)):
                b[start + i].Char.UnicodeChar = ord(val[i])
                if attr is not None:
                    b[start + i].Attributes = attr

    def write_bytes(self, row, col, val):
        b = self._buffer
        start = row * self._width + col
        for i in range(len(val)):
            self._buffer[start+i].Char.AsciiChar = bytes([val[i]])

class StatusBar(Screen):
    def __init__(self, plugin, fileinfo, width):
        super().__init__(plugin, width, 1)
        self.color = self.plugin.GetColor(self.ffic.COL_EDITORSTATUS)
        super().alloc(self.color)
        self.positions = [
            0,                    # file name
            self.width-8-1-8-1-1, # modified
            self.width-8-1-8,     # offset
            self.width-8-1,       # file size
        ]
        self._fileinfo = fileinfo
        self.write_filename()
        self.write_filesize()
        self.write_modified()
        self.write_offset(0)

    def write_filename(self):
        maxlen = self.positions[1]
        filename = self._fileinfo.filename
        if len(filename) > maxlen:
            filename = '...'+filename[:maxlen-3]
        self.write(0, 0, filename)

    def write_modified(self):
        self.write(0, self.positions[1], [' ','*'][len(self._fileinfo.modified)!=0])

    def write_offset(self, offset):
        self.write(0, self.positions[2], f'{offset:08x}')

    def write_filesize(self):
        self.write(0, self.positions[3], f'/{self._fileinfo.filesize:08x}')


class KeyBar(Screen):
    ST_NORMAL = 1<<0
    ST_ALT = 1<<1
    ST_SHIFT = 1<<2
    ST_CTRL = 1<<3
    def __init__(self, plugin, fileinfo, width):
        super().__init__(plugin, width, 1)
        self._color_num = self.plugin.GetColor(self.ffic.COL_KEYBARNUM)
        self._color_txt = self.plugin.GetColor(self.ffic.COL_KEYBARTEXT)
        super().alloc(self._color_txt)
        self._state = self.ST_NORMAL
        self._fileinfo = fileinfo
        self.set_labels()

    def get_state(self):
        return self._state

    def set_state(self, state):
        self._state = state
        self.set_labels()

    state = property(get_state, set_state)

    def write_label(self, no, text):
        w = self.width // 12
        col = w * (no-1)
        w -= 2
        if len(text) < w:
            text = text+' '*(w-len(text))
        if len(text) > w:
            text = text[:w]
        self.write(0, col, f'{no:2d}', self._color_num)
        self.write(0, col+2, text)

    def set_labels(self):
        for i in range(12):
            self.write_label(i+1, '')
        if self._state == self.ST_NORMAL:
            self.write_label(1, "Help")
            self.write_label(2, "Save")
            self.write_label(5, "Goto")
            self.write_label(7, "Search")
            self.write_label(9, "Setup")
            self.write_label(10, "Quit")
        elif self._state == self.ST_ALT:
            self.write_label(7, "Prev")
        elif self._state == self.ST_SHIFT:
            self.write_label(2, "SaveAs")
            self.write_label(7, "Next")
        elif self._state == self.ST_CTRL:
            pass

    def col2fkey(self, col):
        w = self.width // 12
        return (col // w)+1


class HexEditor(Screen):
    COL = 12
    def __init__(self, plugin, fileinfo, width, height):
        super().__init__(plugin, width, height)
        self.color = self.plugin.GetColor(self.ffic.COL_EDITORTEXT)
        self.colormodified = 0xe0
        super().alloc(self.color)
        self._fileinfo = fileinfo

    def fill(self):
        super().fillbuffer(self.color)
        fileoffset = self._fileinfo.fileoffset
        filesize = self._fileinfo.filesize
        mmap = self._fileinfo.mmap
        for row in range(self.height):
            self.write(row, 0, f'{fileoffset:010x}:')
            n = 12
            for col in range(16):
                if fileoffset < filesize:
                    b = mmap[fileoffset]
                    self.write(row, n, f'{b:02x}')
                else:
                    self.write(row, n, '  ')
                n += 3
                fileoffset += 1
            b = mmap[fileoffset-16:fileoffset]
            b = bytes([c if c!=0 else 32 for c in b])
            if len(b) < 16:
                b += bytes([32]*(16-len(b)))
            self.write_bytes(row, 64, b)
            if fileoffset >= filesize:
                b = ' '*self.width
                row += 1
                while row < self.height:
                    self.write(row, 0, b)
                    row += 1
                break
        filemin = self._fileinfo.fileoffset
        for k, b in self._fileinfo.modified.items():
            if filemin <= k < fileoffset:
                row = (k - filemin) // 16
                col = ((k - filemin) % 16)
                self.write(row, col * 3 + 12, f'{b:02x}', self.colormodified)
                self.write_bytes(row, 64 + col, bytes([b]))

    def Offset(self, row, col):
        offset = self._fileinfo.fileoffset + row * 16
        if 12 <= col < 60:
            i = (col-12)//3*3+12
            lo = 1 if col > i else 0
            offset += (col-12)//3
            return offset, lo, False
        elif 64 <= col < 80:
            return offset+col-64, 0, True
        return None, None, False

    def Click(self, row, col):
        if 12 <= col < 60:
            i = (col-12)//3*3+12
            i += 1 if col > i else 0
            return i
        elif 64 <= col < 80:
            return col
        return

    def Tab(self, row, col):
        if 12 <= col < 60:
            return row, (col-12)//3+64
        elif 64 <= col < 80:
            return row, 12 + (col - 64) * 3
        return None, None

    def Move(self, row, col, rowinc, colinc):
        if rowinc is not None:
            row += 1 if rowinc else -1
        else:
            if 12 <= col < 60:
                i = (col-12)//3*3+12
                if col == i:
                    i += 1 if colinc else -2
                elif col > i:
                    i += 3 if colinc else 0
                if i == 60:
                    i = 12
                elif i == 10:
                    i = 58
            elif 64 <= col < 80:
                i = col + (1 if colinc else -1)
                if i < 64:
                    i = 79
                elif i == 80:
                    i = 64
            else:
                return None, None
            col = i
        return row, col

class Plugin(PluginBase):
    label = "Python Hex Edit"
    openFrom = ["PLUGINSMENU", "EDITOR", "VIEWER"]

    def OpenPlugin(self, OpenFrom):
        # 5=edit, 6=viewer, 1=panel
        if OpenFrom == 5:
            fqname = Edit(self, self.info, self.ffi, self.ffic).GetFileName()
        elif OpenFrom == 6:
            data = self.ffi.new("struct ViewerInfo *")
            data.StructSize = self.ffi.sizeof("struct ViewerInfo")
            self.info.ViewerControl(self.ffic.VCTL_GETINFO, data)
            fqname = self.f2s(data.FileName)
        elif OpenFrom == 1:
            pnli, pnlidata = self.api.GetCurrentPanelItem()
            fname = self.f2s(pnli.FindData.lpwszFileName)
            fqname = os.path.join(self.api.GetPanelDir(), fname)
        else:
            log.debug(f"unsupported open from {OpenFrom}")
            return
        # log.debug(f"fqname={fqname}")
        try:
            with open(fqname, "r+b") as f:
                mm = mmap.mmap(f.fileno(), 0)
                try:
                    self.HexEdit(fqname, mm)
                except Exception as ex:
                    log.exception('run')
                finally:
                    mm.close()
        except Exception as ex:
            log.exception('run')

    def GotoDialog(self):
        @self.ffi.callback("FARWINDOWPROC")
        def DialogProc(hDlg, Msg, Param1, Param2):
            return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)

        b = fdb.DialogBuilder(
            self,
            DialogProc,
            "Goto",
            "charmap",
            0,
            fdb.VSizer(
                fdb.HSizer(
                    fdb.TEXT(None, "&Goto:"),
                    fdb.EDIT("vgoto", width=10, history='uhexeditvgoto', flags=self.ffic.DIF_USELASTHISTORY),
                ),
                fdb.HLine(),
                fdb.HSizer(
                    fdb.BUTTON("vok", "OK", default=True, flags=self.ffic.DIF_CENTERGROUP),
                    fdb.BUTTON("vcancel", "Cancel", flags=self.ffic.DIF_CENTERGROUP),
                ),
            ),
        )
        dlg = b.build(-1, -1)

        res = self.info.DialogRun(dlg.hDlg)
        if res == dlg.ID_vok:
            v = dlg.GetText(dlg.ID_vgoto)
            try:
                offset = int(v, 16 if v[:2] == '0x' else 10)
                return offset
            except:
                log.exception('goto')
                return None
        return None

    def GetCursorPos(self, hDlg, ID):
        cpos = self.ffi.new("COORD *")
        self.info.SendDlgMessage(hDlg, self.ffic.DM_GETCURSORPOS, ID, self.ffi.cast("LONG_PTR", cpos))
        return (cpos.X, cpos.Y)

    def SetCursorPos(self, hDlg, ID, col, row):
        cpos = self.ffi.new("COORD *", dict(X=col, Y=row))
        self.info.SendDlgMessage(hDlg, self.ffic.DM_SETCURSORPOS, ID, self.ffi.cast("LONG_PTR", cpos))

    def GetColor(self, no):
        data = self.ffi.new("DWORD *")
        rc = self.info.AdvControl(
            self.info.ModuleNumber,
            self.ffic.ACTL_GETCOLOR,
            self.ffi.cast("VOID *", no),
            self.ffi.cast("VOID *", data)
        )
        return data[0]

    def GetFarRect(self):
        data = self.ffi.new("SMALL_RECT *")
        rc = self.info.AdvControl(
            self.info.ModuleNumber,
            self.ffic.ACTL_GETFARRECT,
            self.ffi.cast("VOID *", data),
            self.ffi.NULL
        )
        return data

    def Help(self):
        # TODO
        self.notice("TODO: F1=Help.", "HexEditor")

    def Save(self, nname=False, showinfo=True):
        if nname:
            # TODO save as
            self.notice("TODO: F2+SHIFT=Save as ...", "HexEditor")
            return
        else:
            if not self.fileinfo.modified:
                self.notice("File is not modified - not saved.", "HexEditor")
                return
            mm = self.fileinfo.mmap
            for offset, b in self.fileinfo.modified.items():
                mm[offset] = b
        self.fileinfo._modified = {}
        self.hexeditor.fill()
        self.info.SendDlgMessage(self.dlg.hDlg, self.ffic.DM_SHOWITEM, self.dlg.ID_statusbar, 1)
        self.info.SendDlgMessage(self.dlg.hDlg, self.ffic.DM_SHOWITEM, self.dlg.ID_hexeditor, 1)
        if showinfo:
            self.notice("File is saved.", "HexEditor")

    def Goto(self):
        offset = self.GotoDialog()
        if offset is None or offset < 0 or offset >= self.fileinfo.filesize:
            return None, None
        if self.fileinfo.fileoffset <= offset and offset < 16 * self.hexeditor.height:
            offset -= self.fileinfo.fileoffset
            row = offset // 16
            col = 12+(offset % 16)*3
        else:
            self.fileinfo.fileoffset = max(offset - 8 * self.hexeditor.height, 0)
            offset -= self.fileinfo.fileoffset
            row = offset // 16
            col = 12+(offset % 16)*3
            self.hexeditor.fill()
        return row, col

    def Search(self):
        # TODO
        self.notice("TODO: F7=Search +ALT=Prev +SHIFT=Next.", "HexEditor")
    SearchNext = SearchPrev = Search

    def Setup(self):
        # TODO
        self.notice("Not implemented F9=Setup.", "HexEditor")

    def Quit(self):
        if not self.fileinfo.modified:
            return 0
        rc = self._popup("File is modified. Save ?", "HexEditor", self.ffic.FMSG_MB_YESNOCANCEL)
        if rc == 0: # Yes
            self.Save(False, True)
            return 0
        elif rc == 1: # NO
            return 0
        return 1

    def Resize(self, size):
        # TODO resize
        pass

    def Scroll(self, size):
        if size < 0:
            fileoffset = max(0, self.fileinfo.fileoffset + size)
            if fileoffset == self.fileinfo.fileoffset:
                return False
        else:
            fileoffset = self.fileinfo.fileoffset + size
            if fileoffset >= self.fileinfo.filesize:
                return False
        self.fileinfo.fileoffset = fileoffset
        self.hexeditor.fill()
        self.info.SendDlgMessage(self.dlg.hDlg, self.ffic.DM_SHOWITEM, self.dlg.ID_hexeditor, 1)
        return True

    def DialogProc(self, Msg, Param1, Param2):
        # log.debug(f"dlg: Msg={Msg:08x} Param1={Param1:08x} Param2={Param2:08x}")
        if Msg == self.ffic.DN_INITDIALOG:
            self.hexeditor.fill()
            self.SetCursorPos(self.dlg.hDlg, self.dlg.ID_hexeditor, self.hexeditor.COL, 0)
            self.info.SendDlgMessage(self.dlg.hDlg, self.ffic.DM_SETCURSORSIZE, self.dlg.ID_hexeditor, 1|(100<<16))
            self.dlg.SetFocus(self.dlg.ID_hexeditor)
            return self.info.DefDlgProc(self.dlg.hDlg, Msg, Param1, Param2)
        elif Msg == self.ffic.DN_RESIZECONSOLE:
            size = self.ffi.cast("COORD *", Param2)
            self.Resize(size)
            return 1
        elif Msg == self.ffic.DN_KEY and Param1 == 0:
            st = KeyBar.ST_NORMAL
            if Param2 & self.ffic.KEY_SHIFT:
                st = KeyBar.ST_SHIFT
            elif Param2 & self.ffic.KEY_CTRL:
                st = KeyBar.ST_CTRL
            elif Param2 & self.ffic.KEY_ALT:
                st = KeyBar.ST_ALT
            if self.keybar.state != st:
                self.keybar.state = st
                self.info.SendDlgMessage(self.dlg.hDlg, self.ffic.DM_SHOWITEM, self.dlg.ID_keybar, 1)
                return 1
        elif Msg == self.ffic.DN_KEY and Param1 == self.dlg.ID_hexeditor:
            # log.debug(f"dlg.DN_KEY: Param1={Param1:08x} Param2={Param2:08x}")
            col, row = self.GetCursorPos(self.dlg.hDlg, self.dlg.ID_hexeditor)
            if Param2 == self.ffic.KEY_PGUP:
                self.Scroll(-16 * self.hexeditor.height)
            elif Param2 == self.ffic.KEY_PGDN:
                self.Scroll(16 * self.hexeditor.height)
            elif Param2 == self.ffic.KEY_LEFT:
                row, col = self.hexeditor.Move(row, col, None, False)
            elif Param2 == self.ffic.KEY_UP:
                row, col = self.hexeditor.Move(row, col, False, None)
                if row == -1:
                    self.Scroll(-16)
                    row = 0
            elif Param2 == self.ffic.KEY_RIGHT:
                row, col = self.hexeditor.Move(row, col, None, True)
                offset, lo, ascii = self.hexeditor.Offset(row, col)
                if offset >= self.fileinfo.filesize:
                    return 1
            elif Param2 == self.ffic.KEY_DOWN:
                row, col = self.hexeditor.Move(row, col, True, None)
                offset, lo, ascii = self.hexeditor.Offset(row, col)
                if offset >= self.fileinfo.filesize:
                    return 1
                if row == self.hexeditor.height:
                    self.Scroll(16)
            elif Param2 == self.ffic.KEY_TAB:
                row, col = self.hexeditor.Tab(row, col)
            elif Param2 == self.ffic.KEY_ESC:
                return self.Quit()
            elif Param2 == self.ffic.KEY_F1:
                self.Help()
                return 1
            elif Param2 in (
                self.ffic.KEY_F2,
                self.ffic.KEY_F2|self.ffic.KEY_SHIFT,
            ):
                self.Save(Param2 == self.ffic.KEY_F2|self.ffic.KEY_SHIFT, True)
                return 1
            elif Param2 == self.ffic.KEY_F5:
                row, col = self.Goto()
                if row is None:
                    return 1
            elif Param2 in (
                self.ffic.KEY_F7, 
                self.ffic.KEY_F7|self.ffic.KEY_SHIFT,
                self.ffic.KEY_F7|self.ffic.KEY_ALT,
            ):
                if Param2 == self.ffic.KEY_F7:
                    self.Search()
                elif Param2 == self.ffic.KEY_F7|self.ffic.KEY_SHIFT:
                    self.SearchNext()
                else:
                    self.SearchPrev()
                return 1
            elif Param2 == self.ffic.KEY_F9:
                self.Setup()
                return 1
            elif Param2 == self.ffic.KEY_F10:
                return self.Quit()
            else:
                if 48 <= Param2 <= 57 or 97 <= Param2 <= 102:
                    offset, lo, ascii = self.hexeditor.Offset(row, col)
                    if not ascii:
                        b = self.fileinfo.modified.get(offset) or self.fileinfo.mmap[offset]
                        v = Param2 - (48 if Param2 < 91 else 87)
                        if lo:
                            b = b&0xf0|v
                        else:
                            b = b&0x0f|(v<<4)
                        self.fileinfo.modifiy(offset, b)
                        self.hexeditor.fill()
                        row, col = self.hexeditor.Move(row, col, None, True)
                else:
                    return self.info.DefDlgProc(self.dlg.hDlg, Msg, Param1, Param2)
            if row is not None:
                self.SetCursorPos(self.dlg.hDlg, self.dlg.ID_hexeditor, col, row)
                self.statusbar.write_offset(self.hexeditor.Offset(row, col)[0])
                self.info.SendDlgMessage(self.dlg.hDlg, self.ffic.DM_SHOWITEM, self.dlg.ID_statusbar, 1)
            return 1
        elif Msg == self.ffic.DN_MOUSECLICK:
            mou = self.ffi.cast("MOUSE_EVENT_RECORD *", Param2)
            col = mou.dwMousePosition.X
            row = mou.dwMousePosition.Y
            if Param1 == self.dlg.ID_hexeditor:
                col = self.hexeditor.Click(row, col)
                if col is not None:
                    self.SetCursorPos(self.dlg.hDlg, self.dlg.ID_hexeditor, col, row)
            elif Param1 == self.dlg.ID_keybar:
                fkey = self.keybar.col2fkey(col)
                if self.keybar.state == KeyBar.ST_NORMAL:
                    if fkey == 1:
                        self.Help()
                    elif fkey == 2:
                        self.Save(False, True)
                    elif fkey == 5:
                        row, col = self.Goto()
                        if row is not None:
                            self.SetCursorPos(self.dlg.hDlg, self.dlg.ID_hexeditor, col, row)
                    elif fkey == 7:
                        self.Search()
                    elif fkey == 9:
                        self.Setup()
                    elif fkey == 10:
                        if self.Quit() == 0:
                            self.info.SendDlgMessage(self.dlg.hDlg, self.ffic.DM_CLOSE, 1, 0)
                elif self.state == KeyBar.ST_ALT:
                    if fkey == 7:
                        self.SearchPrev()
                elif self.state == KeyBar.ST_SHIFT:
                    if fkey == 2:
                        self.Save(True, True)
                    elif fkey == 7: # search next
                        self.SearchNext()
            return 1
        return self.info.DefDlgProc(self.dlg.hDlg, Msg, Param1, Param2)

    def HexEdit(self, fqname, mm):
        # import debugpy; debugpy.breakpoint()

        r = self.GetFarRect()
        self.wwidth = max(r.Right-r.Left+1, 80)
        self.wheight = max(r.Bottom-r.Top+1, 3)
        self.fileinfo = FileInfo(mm, fqname, os.stat(fqname)[stat.ST_SIZE])
        self.hexeditor = HexEditor(self, self.fileinfo, self.wwidth, self.wheight-2)
        self.statusbar = StatusBar(self, self.fileinfo, self.wwidth)
        self.keybar = KeyBar(self, self.fileinfo, self.wwidth)

        @self.ffi.callback("FARWINDOWPROC")
        def DialogProc(hDlg, Msg, Param1, Param2):
            try:
                return self.DialogProc(Msg, Param1, Param2)
            except:
                log.exception('dialogproc')
                return 1

        b = fdb.DialogBuilder(
            self,
            DialogProc,
            "Python HexEdit",
            "hexedit",
            0,
            fdb.VSizer(
                fdb.USERCONTROL('statusbar', self.statusbar.width, self.statusbar.height,
                    param={'VBuf':self.ffi.cast("CHAR_INFO *", self.ffi.addressof(self.statusbar.buffer))}),
                fdb.USERCONTROL('hexeditor', self.hexeditor.width, self.hexeditor.height,
                    param={'VBuf':self.ffi.cast("CHAR_INFO *", self.ffi.addressof(self.hexeditor.buffer))}),
                fdb.USERCONTROL('keybar', self.keybar.width, self.keybar.height,
                    param={'VBuf':self.ffi.cast("CHAR_INFO *", self.ffi.addressof(self.keybar.buffer))}),
            ),
            (0, 0, 0, 0)
        )
        self.dlg = b.build_nobox(-1, -1, self.wwidth, self.wheight)

        self.info.DialogRun(self.dlg.hDlg)
        self.info.DialogFree(self.dlg.hDlg)
        self.dlg = None
        self.keybar = None
        self.statusbar = None
        self.hexeditor = None
        self.fileinfo = None
