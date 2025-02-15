import logging
from far2l.plugin import PluginBase
from far2l.fardialogbuilder import (
    TEXT,
    EDIT,
    BUTTON,
    USERCONTROL,
    HLine,
    HSizer,
    VSizer,
    DialogBuilder,
)


log = logging.getLogger(__name__)


class Plugin(PluginBase):
    label = "Python Character Map"
    openFrom = ["PLUGINSMENU", "COMMANDLINE", "EDITOR", "VIEWER", "DIALOG"]

    def OpenPlugin(self, OpenFrom):
        try:
            return self._OpenPlugin(OpenFrom)
        except Exception as ex:
            log.exception('run')

    def GetCursorPos(self, hDlg, ID):
        cpos = self.ffi.new("COORD *")
        self.info.SendDlgMessage(hDlg, self.ffic.DM_GETCURSORPOS, ID, self.ffi.cast("LONG_PTR", cpos))
        return (cpos.X, cpos.Y)

    def SetCursorPos(self, hDlg, ID, col, row):
        cpos = self.ffi.new("COORD *", dict(X=col, Y=row))
        self.info.SendDlgMessage(hDlg, self.ffic.DM_SETCURSORPOS, ID, self.ffi.cast("LONG_PTR", cpos))

    def _OpenPlugin(self, OpenFrom):
        if 0:
            import debugpy
            debugpy.breakpoint()

        def GetColor(no):
            data = self.ffi.new("DWORD *")
            rc = self.info.AdvControl(
                self.info.ModuleNumber,
                self.ffic.ACTL_GETCOLOR,
                self.ffi.cast("VOID *", no),
                self.ffi.cast("VOID *", data)
            )
            return data[0]

        self.offset = 0
        self.max_col = 64
        self.max_row = 20
        self.text = ''
        self.charbuf = self.ffi.new("CHAR_INFO []", self.max_col*self.max_row)
        attrNormal = GetColor(self.ffic.COL_EDITORTEXT)
        attrSelected = GetColor(self.ffic.COL_EDITORSELECTEDTEXT)
        #attrNormal = 0x170
        #attrSelected = 0x1c

        def setVBuf(hDlg):
            self.info.SendDlgMessage(hDlg, self.ffic.DM_ENABLEREDRAW, 0, 0)
            for i in range(self.max_col*self.max_row):
                cb = self.charbuf[i]
                cb.Attributes = attrNormal
                if self.offset+i <= 0xffff:
                    cb.Char.UnicodeChar = self.offset+i
                else:
                    cb.Char.UnicodeChar = 0x20
            self.info.SendDlgMessage(hDlg, self.ffic.DM_ENABLEREDRAW, 1, 0)

        def xy2lin(col, row):
            return row * self.max_col + col

        def setColor(col, row, attr):
            self.charbuf[xy2lin(col, row)].Attributes = attr

        def updateOffset(offset):
            dlg.SetText(dlg.ID_voffset, f"{offset:5d}/0x{offset:04x}")

        def _DialogProc(hDlg, Msg, Param1, Param2):
            if Msg == self.ffic.DN_INITDIALOG:
                setVBuf(hDlg)
                self.SetCursorPos(hDlg, dlg.ID_hex, 0, 0)
                setColor(0, 0, attrSelected)
                updateOffset(0)
                self.info.SendDlgMessage(hDlg, self.ffic.DM_SETCURSORSIZE, dlg.ID_hex, 0|(0<<16))
                dlg.SetFocus(dlg.ID_hex)
                return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)
            elif Msg == self.ffic.DN_BTNCLICK:
                #log.debug(f"btn DialogProc({Param1}, {Param2})")
                if Param1 == dlg.ID_vgoto:
                    v = dlg.GetText(dlg.ID_vgotovalue).strip()
                    if v:
                        try:
                            offset = int(v, 16 if v[:2] == '0x' else 10)
                            if offset > 0xffff:
                                offset = 0
                                dlg.SetText(dlg.ID_vgotovalue, '0')
                            self.offset = offset
                        except:
                            if len(v) == 1:
                                self.offset = ord(v)
                                setVBuf(hDlg)
                            else:
                                dlg.SetText(dlg.ID_vgotovalue, '')
                                log.exception('goto')
                    setVBuf(hDlg)
                    updateOffset(self.offset)
                    col, row = self.GetCursorPos(hDlg, dlg.ID_hex)
                    setColor(col, row, attrNormal)
                    setColor(0, 0, attrSelected)
                    self.SetCursorPos(hDlg, dlg.ID_hex, 0, 0)
                    dlg.SetFocus(dlg.ID_hex)
                    return 1
                elif Param1 == dlg.ID_vok:
                    col, row = self.GetCursorPos(hDlg, dlg.ID_hex)
                    offset = self.offset + xy2lin(col, row)
                    if offset <= 0xffff:
                        try:
                            self.text = chr(offset)
                        except:
                            log.exception(f'OK click {offset}')
                return 0
            elif Msg == self.ffic.DN_KEY and Param1 == dlg.ID_hex:
                col, row = self.GetCursorPos(hDlg, dlg.ID_hex)
                setColor(col, row, attrNormal)
                #log.debug(f"key DialogProc({Param1}, {Param2:x}), col={col} row={row})")
                if Param2 == self.ffic.KEY_PGUP:
                    self.offset -= self.max_row * self.max_col
                    if self.offset < 0:
                        self.offset = 0
                    setVBuf(hDlg)
                    offset = self.offset + xy2lin(col, row)
                    updateOffset(offset)
                elif Param2 == self.ffic.KEY_HOME:
                    row = 0
                    col = 0
                elif Param2 == self.ffic.KEY_PGDN:
                    self.offset += self.max_row * self.max_col
                    if self.offset > 0xffff:
                        self.offset = 0x10000 - self.max_row * self.max_col
                    setVBuf(hDlg)
                    offset = self.offset + xy2lin(col, row)
                    updateOffset(offset)
                elif Param2 == self.ffic.KEY_LEFT:
                    col -= 1
                elif Param2 == self.ffic.KEY_UP:
                    row -= 1
                elif Param2 == self.ffic.KEY_RIGHT:
                    col += 1
                elif Param2 == self.ffic.KEY_DOWN:
                    row += 1
                elif Param2 == self.ffic.KEY_ENTER:
                    #log.debug(f"enter at offset:{self.offset} row:{row} col:{col}")
                    offset = self.offset + xy2lin(col, row)
                    if offset <= 0xffff:
                        try:
                            self.text = chr(offset)
                        except:
                            log.exception(f'enter at {offset}')
                    return 0
                elif Param2 == self.ffic.KEY_ESC:
                    return 0
                else:
                    return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)
                if col == self.max_col:
                    col = 0
                elif col == -1:
                    col = self.max_col - 1
                if row == self.max_row:
                    row = 0
                elif row == -1:
                    row = self.max_row - 1
                setColor(col, row, attrSelected)
                offset = self.offset + xy2lin(col, row)
                updateOffset(offset)
                #log.debug(f"col={col}/{self.max_col} row={row}/{self.max_row}")
                self.SetCursorPos(hDlg, dlg.ID_hex, col, row)
                return 1
            elif Msg == self.ffic.DN_MOUSECLICK and Param1 == dlg.ID_hex:
                col, row = self.GetCursorPos(hDlg, dlg.ID_hex)
                setColor(col, row, attrNormal)
                mou = self.ffi.cast("MOUSE_EVENT_RECORD *", Param2)
                col = mou.dwMousePosition.X
                row = mou.dwMousePosition.Y
                setColor(col, row, attrSelected)
                self.SetCursorPos(hDlg, dlg.ID_hex, col, row)
                #log.debug(f"mou DialogProc(col={col} row={row})")
                offset = self.offset + xy2lin(col, row)
                #self.text = self.symbols[offset]
                updateOffset(offset)
            return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)

        @self.ffi.callback("FARWINDOWPROC")
        def DialogProc(hDlg, Msg, Param1, Param2):
            try:
                return _DialogProc(hDlg, Msg, Param1, Param2)
            except:
                log.exception('dialogproc')
                return 1

        b = DialogBuilder(
            self,
            DialogProc,
            "Python CharMap",
            "charmap",
            0,
            VSizer(
                HSizer(
                    TEXT(None, "Offset:"),
                    TEXT("voffset", " "*12),
                    TEXT(None, "&Goto:"),
                    EDIT("vgotovalue", width=7),
                    BUTTON("vgoto", "GOTO"),
                ),
                HLine(),
                USERCONTROL('hex', self.max_col, self.max_row, param={'VBuf':self.ffi.cast("CHAR_INFO *", self.ffi.addressof(self.charbuf))}),
                HLine(),
                HSizer(
                    BUTTON("vok", "OK", flags=self.ffic.DIF_CENTERGROUP),
                    BUTTON("vcancel", "Cancel", flags=self.ffic.DIF_CENTERGROUP),
                ),
            ),
        )
        dlg = b.build(-1, -1)

        res = self.info.DialogRun(dlg.hDlg)
        #log.debug(f'res={res} text=***{self.text}***')
        if res in (dlg.ID_hex, dlg.ID_vok):
            self.info.FSF.CopyToClipboard(self.s2f(self.text))
        self.info.DialogFree(dlg.hDlg)
