import logging
from far2l.plugin import PluginBase
from far2l.fardialogbuilder import (
    TEXT,
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
    openFrom = ["PLUGINSMENU", "COMMANDLINE", "EDITOR", "VIEWER"]

    def OpenPlugin(self, OpenFrom):
        try:
            return self._OpenPlugin(OpenFrom)
        except Exception as ex:
            log.exception('run')

    def GetCursorPos(self, hDlg, ID):
        cpos = self.ffi.new("COORD *", dict(X=0, Y=0))
        self.info.SendDlgMessage(hDlg, self.ffic.DM_GETCURSORPOS, ID, self.ffi.cast("LONG_PTR", cpos))
        return (cpos.X, cpos.Y)

    def SetCursorPos(self, hDlg, ID, col, row):
        cpos = self.ffi.new("COORD *", dict(X=col, Y=row))
        self.info.SendDlgMessage(hDlg, self.ffic.DM_SETCURSORPOS, ID, self.ffi.cast("LONG_PTR", cpos))

    def _OpenPlugin(self, OpenFrom):
        if 0:
            import debugpy
            debugpy.breakpoint()

        self.max_col = 32
        symbols = ''.join([chr(i) for i in range(256)])+(
            'ЂЃ‚ѓ„…†‡€‰Љ‹ЊЌЋЏ'
            'ђ‘’“”•–—™љ›њќћџ'
            ' ЎўЈ¤Ґ¦§Ё©Є«¬"®Ї'
            '°±Ііґµ¶·ё№є»јЅѕї'
            'АБВГДЕЖЗИЙКЛМНОП'
            'РСТУФХЦЧШЩЪЫЬЭЮЯ'
            'абвгдежзийклмноп'
            'рстуфхцчшщъыьэюя'
            '░▒▓│┤╡╢╖╕╣║╗╝╜╛┐'
            '└┴┬├─┼╞╟╚╔╩╦╠═╬╧'
            '╨╤╥╙╘╒╓╫╪┘┌█▄▌▐▀'
            '∙√■⌠≈≤≥⌡²÷ąćęłńó'
            'śżźĄĆĘŁŃÓŚŻŹ'
        )
        n = len(symbols) % self.max_col
        if n:
            symbols += ' '*(self.max_col-n)
        self.max_row = len(symbols) // self.max_col
        self.text = ''
        self.charbuf = self.ffi.new("CHAR_INFO []", len(symbols))
        attrNormal = 0x170
        attrSelected = 0x1c
        for i in range(len(symbols)):
            cb = self.charbuf[i]
            cb.Attributes = attrNormal
            cb.Char.UnicodeChar = ord(symbols[i])

        def xy2lin(col, row):
            return row * self.max_col + col

        def setColor(col, row, attr):
            self.charbuf[xy2lin(col, row)].Attributes = attr

        def updateOffset(offset):
            char = symbols[offset]
            dlg.SetText(dlg.ID_voffset, f"{offset}")
            dlg.SetText(dlg.ID_vchar, f'{char}/{ord(char):4x}')

        @self.ffi.callback("FARWINDOWPROC")
        def DialogProc(hDlg, Msg, Param1, Param2):
            if Msg == self.ffic.DN_INITDIALOG:
                self.SetCursorPos(hDlg, dlg.ID_hex, 0, 0)
                setColor(0, 0, attrSelected)
                updateOffset(0)
                self.info.SendDlgMessage(hDlg, self.ffic.DM_SETCURSORSIZE, dlg.ID_hex, 1|(100<<32))
                return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)
            elif Msg == self.ffic.DN_BTNCLICK:
                #log.debug(f"btn DialogProc({Param1}, {Param2})")
                col, row = self.GetCursorPos(hDlg, dlg.ID_hex)
                offset = xy2lin(col, row)
                self.text = symbols[offset]
                updateOffset(offset)
                #log.debug(f"enter:{offset} row:{row} col:{col}, ch:{self.text} cb={self.charbuf[offset].Attributes:x}")
                return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)
            elif Msg == self.ffic.DN_KEY and Param1 == dlg.ID_hex:
                col, row = self.GetCursorPos(hDlg, dlg.ID_hex)
                setColor(col, row, attrNormal)
                #log.debug(f"key DialogProc({Param1}, {Param2:x}), col={col} row={row})")
                if Param2 == self.ffic.KEY_LEFT:
                    col -= 1
                elif Param2 == self.ffic.KEY_UP:
                    row -= 1
                elif Param2 == self.ffic.KEY_RIGHT:
                    col += 1
                elif Param2 == self.ffic.KEY_DOWN:
                    row += 1
                elif Param2 == self.ffic.KEY_ENTER:
                    offset = xy2lin(col, row)
                    self.text = symbols[offset]
                    #log.debug(f"enter:{offset} row:{row} col:{col}, ch:{self.text} cb={self.charbuf[offset].Attributes:x}")
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
                offset = xy2lin(col, row)
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
                offset = xy2lin(col, row)
                self.text = self.symbols[offset]
                updateOffset(offset)
            return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)

        b = DialogBuilder(
            self,
            DialogProc,
            "Python CharMap",
            "charmap",
            0,
            VSizer(
                HSizer(
                    TEXT(None, "Offset:"),
                    TEXT("voffset", " "*8),
                    TEXT(None, "Char:"),
                    TEXT("vchar", " "*6),
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
        if res in (1, dlg.ID_vok):
            self.info.FSF.CopyToClipboard(self.s2f(self.text))
        self.info.DialogFree(dlg.hDlg)
