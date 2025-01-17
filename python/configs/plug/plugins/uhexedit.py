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
        self._modified = False

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

    def get_modified(self):
        return self._modified

    def set_modified(self, modified):
        self._modified = modified

    modified = property(get_modified, set_modified)


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
            b[start].Attributes = attr

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
        self.write(0, self.positions[2], [' ','*'][self._fileinfo.modified])

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

    state = property(get_state, set_state)

    def write_label(self, no, text):
        w = self.width // 12
        col = w * (no-1)
        w -= 2
        if len(text) < w:
            text = text+' '*(w-len(text))
        if len(text) > w:
            text = text[:w]
        self.write(0, col+0, None, self._color_num)
        self.write(0, col+1, None, self._color_num)
        self.write(0, col, f'{no:2d}')
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
        super().alloc(self.color)
        self._fileinfo = fileinfo

    def fill(self):
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
                if col == 7:
                    self.write(row, n, '|')
                    n += 2
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
                return

    def Click(self, parent, row, col):
        if 12 <= col < 36:
            i = (col-12)//3*3+12
        elif 38 <= col < 61:
            i = (col-38)//3*3+38
        elif 64 <= col < 80:
            return col
        else:
            return
        i += 1 if col > i else 0
        return i

    def Tab(self, parent, row, col):
        if 12 <= col < 36:
            return row, (col-12)//3+64
        elif 38 <= col < 61:
            return row, (col-38)//3+64+8
        elif 64 <= col < 80:
            log.debug(f'1.col={col}')
            col = (col - 64) * 3
            col += 12 if col < 24 else 14
            log.debug(f'2.col={col}')
            return row, col
        return None, None

    def Move(self, parent, row, col, rowinc, colinc):
        if rowinc is not None:
            row += 1 if rowinc else -1
        else:
            if 12 <= col < 36:
                i = (col-12)//3*3+12
                if col == i:
                    i += 1 if colinc else -2
                elif col > i:
                    i += 3 if colinc else 0
                else:
                    i += 2 if colinc else -1
                if i == 10:
                    i = 60
                elif i == 36:
                    i = 38
            elif 38 <= col < 61:
                i = (col-38)//3*3+38
                if col == i:
                    i += 1 if colinc else -2
                elif col > i:
                    i += 3 if colinc else 0
                else:
                    i += 2 if colinc else -1
                if i == 36:
                    i = 34
                elif i == 62:
                    i = 12
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
        if 0:
            import debugpy
            debugpy.breakpoint()

        r = self.GetFarRect()
        wwidth = max(r.Right-r.Left+1, 80)
        wheight = max(r.Bottom-r.Top+1, 3)
        fileinfo = FileInfo(mm, fqname, os.stat(fqname)[stat.ST_SIZE])
        hexeditor = HexEditor(self, fileinfo, wwidth, wheight-2)
        statusbar = StatusBar(self, fileinfo, wwidth)
        keybar = KeyBar(self, fileinfo, wwidth)

        hexeditor.fill()
        log.debug(f'{wwidth}x{wheight}')

        def Scroll(hDlg, size):
            if size < 0:
                fileoffset = max(0, fileinfo.fileoffset + size)
                if fileoffset == fileinfo.fileoffset:
                    return
            else:
                fileoffset = fileinfo.fileoffset + size
                if fileoffset >= fileinfo.filesize:
                    return
            fileinfo.fileoffset = fileoffset
            hexeditor.fill()
            self.info.SendDlgMessage(hDlg, self.ffic.DM_ENABLEREDRAW, 0, 0)
            self.info.SendDlgMessage(hDlg, self.ffic.DM_ENABLEREDRAW, 1, 0)

        def _DialogProc(hDlg, Msg, Param1, Param2):
            if Msg == self.ffic.DN_INITDIALOG:
                self.SetCursorPos(hDlg, dlg.ID_hexeditor, hexeditor.COL, 0)
                self.info.SendDlgMessage(hDlg, self.ffic.DM_SETCURSORSIZE, dlg.ID_hexeditor, 1|(100<<16))
                dlg.SetFocus(dlg.ID_hexeditor)
                return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)
            elif Msg == self.ffic.DN_RESIZECONSOLE:
                size = self.ffi.cast("COORD *", Param2)
                log.debug(f"resize {size}")
            #elif Msg == self.ffic.DN_CONTROLINPUT:
            #    XXX control, shift, alt ? howto catch them ???
            #    rec = self.ffi.cast("INPUT_RECORD *", Param2)
            #    log.debug(f"ctrl {rec.EventType}")
            #    if rec.EventType == self.ffic.KEY_EVENT:
            #        log.debug(f"ctrl key {rec.Event.KeyEvent}")
            #        if rec.Event.KeyEvent.wVirtualKeyCode in (self.ffic.VK_SHIFT, self.ffic.VK_CONTROL, self.ffic.VK_MENU):
            #            return 1
            #    return 0
            elif Msg == self.ffic.DN_KEY and Param1 == dlg.ID_hexeditor:
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
                    if row == 0:
                        # scroll 1 row
                        return None, None
                    elif row >= self.height:
                        # scroll 1 row
                        return None, None

                    row, col = hexeditor.Move(self, row, col, False, None)
                elif Param2 == self.ffic.KEY_RIGHT:
                    row, col = hexeditor.Move(self, row, col, None, True)
                elif Param2 == self.ffic.KEY_DOWN:
                    row, col = hexeditor.Move(self, row, col, True, None)
                elif Param2 == self.ffic.KEY_TAB:
                    row, col = hexeditor.Tab(self, row, col)
                elif Param2 == self.ffic.KEY_ESC:
                    return 0
                else:
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

        res = self.info.DialogRun(dlg.hDlg)
        log.debug(f'res={res}')
        self.info.DialogFree(dlg.hDlg)
