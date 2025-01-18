import os
import stat
import mmap
import logging
import far2lc
from far2l import fardialogbuilder as fdb
from yfar import FarPlugin


log = logging.getLogger(__name__)

class FileInfo:
    def __init__(self, mmap, filename, filesize):
        self._mmap = mmap
        self._filename = filename
        self._filesize = filesize
        self._fileoffset = 0
        self._codepage = 0
        self._readonly = True
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

    def get_readonly(self):
        return self._readonly

    def set_readonly(self, readonly):
        self._readonly = readonly

    readonly = property(get_readonly, set_readonly)

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
            0,
            self.width-8-1-8-2-5-2-2,
            self.width-8-1-8-2-5-2,
            self.width-8-1-8-2-5,
            self.width-8-1-8,
            self.width-8-1,
        ]
        self._fileinfo = fileinfo
        self.write_filename()
        self.write_filesize()
        self.write_readonly()
        self.write_modified()
        self.write_codepage()
        self.write_offset()

    def write_filename(self):
        maxlen = self.positions[1]
        filename = self._fileinfo.filename
        if len(filename) > maxlen:
            filename = '...'+filename[:maxlen-3]
        self.write(0, 0, filename)

    def write_readonly(self):
        self.write(0, self.positions[1], ['RW', 'RO'][self._fileinfo.readonly])

    def write_modified(self):
        self.write(0, self.positions[2], [' ','*'][len(self._fileinfo.modified)!=0])

    def write_codepage(self):
        #self.write(0, self.positions[3], self._fileinfo.codepage)
        pass

    def write_offset(self):
        self.write(0, self.positions[4], f'{self._fileinfo.fileoffset:08x}')

    def write_filesize(self):
        self.write(0, self.positions[5], f'/{self._fileinfo.filesize:08x}')


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
        log.debug(f'keybar.state={self._state:x}')

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
            if self._fileinfo.modified:
                self.write_label(2, "Save")
            self.write_label(4, ["RW", "RO"][self._fileinfo.readonly])
            self.write_label(5, "Goto")
            self.write_label(7, "Search")
            self.write_label(8, "CodePg")
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

    def Click(self, parent, col):
        fkey = self.col2fkey(col)
        if self.state == self.ST_NORMAL:
            if fkey == 1: # Help
                pass
            elif fkey == 2: # Save if fileinfo.modified
                pass
            elif fkey == 4: # swap fileinfo.readonly
                pass
            elif fkey == 5: # goto
                pass
            elif fkey == 7: # search
                pass
            elif fkey == 8: # codepage
                pass
            elif fkey == 9: # setup
                pass
            elif fkey == 10: # quit
                pass
            pass
        elif self.state == self.ST_ALT:
            if fkey == 7: # search prev
                pass
        elif self.state == self.ST_SHIFT:
            if fkey == 2: # save as
                pass
            elif fkey == 7: # search next
                pass


class HexEditor(Screen):
    COL = 12
    def __init__(self, plugin, fileinfo, width, height):
        super().__init__(plugin, width, height)
        self.color = self.plugin.GetColor(self.ffic.COL_EDITORTEXT)
        self.colorm = 0xe0
        super().alloc(self.color)
        self._fileinfo = fileinfo
        self._area = False # hex=false, ascii=true

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
                self.write(row, col * 3 + 12, f'{b:02x}', self.colorm)
                self.write_bytes(row, 64 + col, bytes([b]))

    def Offset(self, parent, row, col):
        offset = self._fileinfo.fileoffset + row * 16
        if 12 <= col < 60:
            i = (col-12)//3*3+12
            lo = 1 if col > i else 0
            offset += (col-12)//3
            return offset, lo, False
        elif 64 <= col < 80:
            return offset+col-64, True
        return None, None, False

    def Click(self, parent, row, col):
        if 12 <= col < 60:
            i = (col-12)//3*3+12
            i += 1 if col > i else 0
            return i
        elif 64 <= col < 80:
            return col
        return

    def Tab(self, parent, row, col):
        if 12 <= col < 60:
            return row, (col-12)//3+64
        elif 64 <= col < 80:
            return row, 12 + (col - 64) * 3
        return None, None

    def Move(self, parent, row, col, rowinc, colinc):
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

class Plugin(FarPlugin):
    label = "Python Hex Edit"
    openFrom = ["PLUGINSMENU", "EDITOR", "VIEWER"]

    def OpenPlugin(self, OpenFrom):
        # 5=edit, 6=viewer, 1=panel
        if OpenFrom == 5:
            fqname = self.get_editor().file_name
        elif OpenFrom == 6:
            data = self.ffi.new("struct ViewerInfo *")
            data.StructSize = self.ffi.sizeof("struct ViewerInfo")
            self.info.ViewerControl(self.ffic.VCTL_GETINFO, data)
            fqname = self.f2s(data.FileName)
        elif OpenFrom == 1:
            pnl = self.get_panel()
            fqname = os.path.join(pnl.directory, pnl.current.file_name)
        else:
            log.debug(f"unsupported open from {OpenFrom}")
            return
        log.debug(f"fqname={fqname}")
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

    def HexEdit(self, fqname, mm):
        # import debugpy; debugpy.breakpoint()

        r = self.GetFarRect()
        wwidth = max(r.Right-r.Left+1, 80)
        wheight = max(r.Bottom-r.Top+1, 3)
        fileinfo = FileInfo(mm, fqname, os.stat(fqname)[stat.ST_SIZE])
        hexeditor = HexEditor(self, fileinfo, wwidth, wheight-2)
        statusbar = StatusBar(self, fileinfo, wwidth)
        keybar = KeyBar(self, fileinfo, wwidth)

        hexeditor.fill()
        log.debug(f'{wwidth}x{wheight}')

        def Save(nname=None, showinfo=True):
            if nname:
                # TODO save as
                pass
            else:
                for offset, b in fileinfo.modified.items():
                    mm[offset] = b
            fileinfo._modified = {}
            if showinfo:
                self.notice("File is saved.", "HexEditor")

        def Scroll(hDlg, size):
            if size < 0:
                fileoffset = max(0, fileinfo.fileoffset + size)
                if fileoffset == fileinfo.fileoffset:
                    return False
            else:
                fileoffset = fileinfo.fileoffset + size
                if fileoffset >= fileinfo.filesize:
                    return False
            fileinfo.fileoffset = fileoffset
            hexeditor.fill()
            self.info.SendDlgMessage(hDlg, self.ffic.DM_ENABLEREDRAW, 0, 0)
            self.info.SendDlgMessage(hDlg, self.ffic.DM_ENABLEREDRAW, 1, 0)
            return True

        def _DialogProc(hDlg, Msg, Param1, Param2):
            if Msg < self.ffic.DN_FIRST and Msg != 9:
                log.debug(f"dlg: Msg={Msg:08x} Param1={Param1:08x} Param2={Param2:08x}")
            if Msg == self.ffic.DN_INITDIALOG:
                self.SetCursorPos(hDlg, dlg.ID_hexeditor, hexeditor.COL, 0)
                self.info.SendDlgMessage(hDlg, self.ffic.DM_SETCURSORSIZE, dlg.ID_hexeditor, 1|(100<<16))
                dlg.SetFocus(dlg.ID_hexeditor)
                return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)
            elif Msg == self.ffic.DN_RESIZECONSOLE:
                # TODO resize
                size = self.ffi.cast("COORD *", Param2)
                log.debug(f"resize {size}")
            elif Msg == self.ffic.DN_KEY and Param1 == dlg.ID_hexeditor:
                st = KeyBar.ST_NORMAL
                if Param2 & self.ffic.KEY_SHIFT:
                    st |= KeyBar.ST_SHIFT
                if Param2 & self.ffic.KEY_CTRL:
                    st |= KeyBar.ST_CTRL
                if Param2 & self.ffic.KEY_ALT:
                    st |= KeyBar.ST_ALT
                if keybar.state != st:
                    log.debug(f"dlg.DN_KEY: Param1={Param1:08x} Param2={Param2:08x}")
                    keybar.state = st
                    self.info.SendDlgMessage(hDlg, self.ffic.DM_ENABLEREDRAW, 0, 0)
                    self.info.SendDlgMessage(hDlg, self.ffic.DM_ENABLEREDRAW, 1, 0)
                col, row = self.GetCursorPos(hDlg, dlg.ID_hexeditor)
                if Param2 == self.ffic.KEY_PGUP:
                    Scroll(hDlg, -16 * hexeditor.height)
                    return 1
                elif Param2 == self.ffic.KEY_PGDN:
                    Scroll(hDlg, 16 * hexeditor.height)
                    return 1
                elif Param2 == self.ffic.KEY_LEFT:
                    row, col = hexeditor.Move(self, row, col, None, False)
                elif Param2 == self.ffic.KEY_UP:
                    row, col = hexeditor.Move(self, row, col, False, None)
                    if row == -1:
                        Scroll(hDlg, -16)
                        return 1
                elif Param2 == self.ffic.KEY_RIGHT:
                    row, col = hexeditor.Move(self, row, col, None, True)
                    offset, lo, ascii = hexeditor.Offset(self, row, col)
                    if offset >= fileinfo.filesize:
                        return 1
                elif Param2 == self.ffic.KEY_DOWN:
                    row, col = hexeditor.Move(self, row, col, True, None)
                    offset, lo, ascii = hexeditor.Offset(self, row, col)
                    if offset >= fileinfo.filesize:
                        return 1
                    if row == hexeditor.height:
                        Scroll(hDlg, 16)
                        return 1
                elif Param2 == self.ffic.KEY_TAB:
                    row, col = hexeditor.Tab(self, row, col)
                elif Param2 == self.ffic.KEY_ESC:
                    return 0
                elif Param2 == self.ffic.KEY_F1:
                    # TODO help
                    log.debug(f"dlg.DN_KEY: F1={Param2:08x} state={keybar.state:08x}")
                    return 1
                elif Param2 in (
                    self.ffic.KEY_F2,
                    self.ffic.KEY_F2|self.ffic.KEY_SHIFT,
                ):
                    log.debug(f"dlg.DN_KEY: F2={Param2:08x} state={keybar.state:08x}")
                    if not fileinfo.modified:
                        self.notice("File is not modified - not saved.", "HexEditor")
                    elif Param2 == self.ffic.KEY_F2:
                        Save(None, True)
                    else:
                        self.notice("Not implemented SHIFT+F2=SaveAs.", "HexEditor")
                        pass
                    return 1
                elif Param2 == self.ffic.KEY_F4:
                    log.debug(f"dlg.DN_KEY: F4={Param2:08x} state={keybar.state:08x}")
                    self.notice("Not implemented F4=RO/RW swap.", "HexEditor")
                    return 1
                elif Param2 == self.ffic.KEY_F5:
                    offset = self.GotoDialog()
                    if offset is not None and offset >= 0 and offset < fileinfo.filesize:
                        if fileinfo.fileoffset <= offset and offset < 16 * hexeditor.height:
                            offset -= fileinfo.fileoffset
                            row = offset // 16
                            col = 12+(offset % 16)*3
                        else:
                            fileinfo.fileoffset = max(offset - 8 * hexeditor.height, 0)
                            offset -= fileinfo.fileoffset
                            row = offset // 16
                            col = 12+(offset % 16)*3
                            hexeditor.fill()
                        self.SetCursorPos(hDlg, dlg.ID_hexeditor, col, row)
                        self.info.SendDlgMessage(hDlg, self.ffic.DM_ENABLEREDRAW, 0, 0)
                        self.info.SendDlgMessage(hDlg, self.ffic.DM_ENABLEREDRAW, 1, 0)
                    return 1
                elif Param2 in (
                    self.ffic.KEY_F7, 
                    self.ffic.KEY_F7|self.ffic.KEY_SHIFT,
                    self.ffic.KEY_F7|self.ffic.KEY_ALT,
                ):
                    log.debug(f"dlg.DN_KEY: F7={Param2:08x} state={keybar.state:08x}")
                    self.notice("Not implemented F7=Search +ALT=Prev +SHIFT=Next.", "HexEditor")
                    return 1
                elif Param2 == self.ffic.KEY_F9:
                    log.debug(f"dlg.DN_KEY: F9={Param2:08x} state={keybar.state:08x}")
                    self.notice("Not implemented F9=Setup.", "HexEditor")
                    return 1
                elif Param2 == self.ffic.KEY_F8:
                    log.debug(f"dlg.DN_KEY: F8={Param2:08x} state={keybar.state:08x}")
                    self.notice("Not implemented F8=CodePg.", "HexEditor")
                    return 1
                elif Param2 == self.ffic.KEY_F10:
                    if not fileinfo.modified:
                        return 0
                    rc = self._popup("File is modified. Save ?", "HexEditor", self.ffic.FMSG_MB_OKCANCEL)
                    if rc == 0: #OK
                        Save(None, True)
                        return 0
                    return 1
                else:
                    if 48 <= Param2 <= 57 or 97 <= Param2 <= 102:
                        offset, lo, ascii = hexeditor.Offset(self, row, col)
                        if not ascii:
                            b = fileinfo.modified.get(offset) or mm[offset]
                            v = Param2 - (48 if Param2 < 91 else 87)
                            if lo:
                                b = b&0xf0|v
                            else:
                                b = b&0x0f|(v<<4)
                            fileinfo.modifiy(offset, b)
                            hexeditor.fill()
                            row, col = hexeditor.Move(self, row, col, None, True)
                            self.SetCursorPos(hDlg, dlg.ID_hexeditor, col, row)
                            self.info.SendDlgMessage(hDlg, self.ffic.DM_ENABLEREDRAW, 0, 0)
                            self.info.SendDlgMessage(hDlg, self.ffic.DM_ENABLEREDRAW, 1, 0)
                        return 1
                    return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)
                if row is not None:
                    self.SetCursorPos(hDlg, dlg.ID_hexeditor, col, row)
                return 1
            elif Msg == self.ffic.DN_MOUSECLICK:
                mou = self.ffi.cast("MOUSE_EVENT_RECORD *", Param2)
                col = mou.dwMousePosition.X
                row = mou.dwMousePosition.Y
                if Param1 == dlg.ID_hexeditor:
                    col = hexeditor.Click(self, row, col)
                    if col is not None:
                        self.SetCursorPos(hDlg, dlg.ID_hexeditor, col, row)
                elif Param1 == dlg.ID_keybar:
                    keybar.Click(self, col)
                return 1
            return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)

        @self.ffi.callback("FARWINDOWPROC")
        def DialogProc(hDlg, Msg, Param1, Param2):
            try:
                return _DialogProc(hDlg, Msg, Param1, Param2)
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
                fdb.USERCONTROL('statusbar', statusbar.width, statusbar.height, param={'VBuf':self.ffi.cast("CHAR_INFO *", self.ffi.addressof(statusbar.buffer))}),
                fdb.USERCONTROL('hexeditor', hexeditor.width, hexeditor.height, param={'VBuf':self.ffi.cast("CHAR_INFO *", self.ffi.addressof(hexeditor.buffer))}),
                fdb.USERCONTROL('keybar', keybar.width, keybar.height, param={'VBuf':self.ffi.cast("CHAR_INFO *", self.ffi.addressof(keybar.buffer))}),
            ),
            (0, 0, 0, 0)
        )
        dlg = b.build_nobox(-1, -1, wwidth, wheight)

        self.info.DialogRun(dlg.hDlg)
        self.info.DialogFree(dlg.hDlg)
