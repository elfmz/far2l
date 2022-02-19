import logging
from far2l.plugin import PluginBase


log = logging.getLogger(__name__)


class Plugin(PluginBase):
    label = "Python Character Map"
    openFrom = ["PLUGINSMENU", "COMMANDLINE", "EDITOR", "VIEWER"]

    def Rebuild(self, hDlg):
        self.info.SendDlgMessage(hDlg, self.ffic.DM_ENABLEREDRAW, 0, 0)
        prefix = ["", "&"]
        for i in range(len(self.symbols)):
            row = i // self.max_col
            col = i % self.max_col
            p = prefix[row == self.cur_row and col == self.cur_col]
            offset = self.first_text_item+row*self.max_col+col
            ch = self.s2f(p+self.symbols[i])
            self.info.SendDlgMessage(hDlg, self.ffic.DM_SETTEXTPTR, offset, self.ffi.cast("LONG_PTR", ch))
        self.info.SendDlgMessage(hDlg, self.ffic.DM_ENABLEREDRAW, 1, 0)

    def OpenPlugin(self, OpenFrom):
        if 0:
            import debugpy
            debugpy.breakpoint()
        symbols = []
        for i in range(256):
            symbols.append(chr(i))
        symbols.extend([
            "Ђ", "Ѓ", "‚", "ѓ", "„", "…", "†", "‡", "€", "‰", "Љ", "‹", "Њ", "Ќ", "Ћ", "Џ", "ђ", "‘", "’", "“", "”", "•", "–", "—", "", "™", "љ", "›", "њ", "ќ", "ћ", "џ",
            " ", "Ў", "ў", "Ј", "¤", "Ґ", "¦", "§", "Ё", "©", "Є", "«", "¬", "­", "®", "Ї", "°", "±", "І", "і", "ґ", "µ", "¶", "·", "ё", "№", "є", "»", "ј", "Ѕ", "ѕ", "ї",
            "А", "Б", "В", "Г", "Д", "Е", "Ж", "З", "И", "Й", "К", "Л", "М", "Н", "О", "П", "Р", "С", "Т", "У", "Ф", "Х", "Ц", "Ч", "Ш", "Щ", "Ъ", "Ы", "Ь", "Э", "Ю", "Я",
            "а", "б", "в", "г", "д", "е", "ж", "з", "и", "й", "к", "л", "м", "н", "о", "п", "р", "с", "т", "у", "ф", "х", "ц", "ч", "ш", "щ", "ъ", "ы", "ь", "э", "ю", "я",
            "░", "▒", "▓", "│", "┤", "╡", "╢", "╖", "╕", "╣", "║", "╗", "╝", "╜", "╛", "┐", "└", "┴", "┬", "├", "─", "┼", "╞", "╟", "╚", "╔", "╩", "╦", "╠", "═", "╬", "╧",
            "╨", "╤", "╥", "╙", "╘", "╒", "╓", "╫", "╪", "┘", "┌", "█", "▄", "▌", "▐", "▀", "∙", "√", "■", "⌠", "≈", "≤", "≥", "⌡", "²", "÷", "ą", "ć", "ę", "ł", "ń", "ó",
            "ś", "ż", "ź", "Ą", "Ć", "Ę", "Ł", "Ń", "Ó", "Ś", "Ż", "Ź", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ",
        ])
        Items = [
            (self.ffic.DI_DOUBLEBOX,   3,  1, 38, 18, 0, {'Selected':0}, 0, 0, self.s2f("Character Map"), 0),
            (self.ffic.DI_BUTTON,      7, 17, 12, 18, 0, {'Selected':0}, 1, self.ffic.DIF_DEFAULT + self.ffic.DIF_CENTERGROUP, self.s2f("OK"), 0),
            (self.ffic.DI_BUTTON,     13, 17, 38, 18, 0, {'Selected':0}, 0, self.ffic.DIF_CENTERGROUP, self.s2f("Cancel"), 0),
            (self.ffic.DI_USERCONTROL, 3,  2, 38, 16, 0, {'Selected':0}, 0, self.ffic.DIF_FOCUS, self.ffi.NULL, 0),
        ]
        self.cur_row = 0
        self.cur_col = 0
        self.max_col = 32
        self.max_row = len(symbols) // self.max_col
        self.first_text_item = len(Items)
        self.symbols = symbols
        self.text = None

        for i in range(len(symbols)):
            row = i // self.max_col
            col = i % self.max_col
            Items.append((self.ffic.DI_TEXT, 5+col, self.first_text_item-2+row, 5+col, self.first_text_item-2+row, 0, {'Selected':0}, 0, 0, self.ffi.NULL, 0))

        @self.ffi.callback("FARWINDOWPROC")
        def DialogProc(hDlg, Msg, Param1, Param2):
            if Msg == self.ffic.DN_INITDIALOG:
                self.Rebuild(hDlg)
                return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)
            elif Msg == self.ffic.DN_BTNCLICK:
                return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)
            elif Msg == self.ffic.DN_KEY and Param1 == self.first_text_item-1:
                log.debug('key DialogProc({0}, {1}, {2}, {3})'.format(hDlg, 'DN_KEY', Param1, Param2))
                if Param2 == self.ffic.KEY_LEFT:
                    self.cur_col -= 1
                elif Param2 == self.ffic.KEY_UP:
                    self.cur_row -= 1
                elif Param2 == self.ffic.KEY_RIGHT:
                    self.cur_col += 1
                elif Param2 == self.ffic.KEY_DOWN:
                    self.cur_row += 1
                elif Param2 == self.ffic.KEY_ENTER:
                    offset = self.cur_row*self.max_col+self.cur_col
                    self.text = self.symbols[offset]
                    log.debug('enter:{0} row:{1} col:{2}, ch:{3}'.format(offset, self.cur_row, self.cur_col, self.text))
                    return 0
                elif Param2 == self.ffic.KEY_ESC:
                    return 0
                else:
                    return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)
                if self.cur_col == self.max_col:
                    self.cur_col = 0
                elif self.cur_col == -1:
                    self.cur_col = self.max_col - 1
                if self.cur_row == self.max_row:
                    self.cur_row = 0
                elif self.cur_row == -1:
                    self.cur_row = self.max_row - 1
                self.Rebuild(hDlg)
                return 1
            elif Msg == self.ffic.DN_MOUSECLICK:
                log.debug('mou DialogProc({0}, {1}, {2}, {3})'.format(hDlg, 'DN_MOUSECLICK', Param1, Param2))
                ch = Param1 - self.first_text_item
                if ch >= 0:
                    focus = self.info.SendDlgMessage(hDlg, self.ffic.DM_GETFOCUS, 0, 0)
                    log.debug('ch:{0} focus:{1}'.format(ch, focus))
                    if focus != self.first_text_item-1:
                        self.info.SendDlgMessage(hDlg, self.ffic.DM_SETFOCUS, self.first_text_item-1, 0)
                    self.cur_row = ch // self.max_col
                    self.cur_col = ch % self.max_col
                    self.cur_col = min(max(0, self.cur_col), self.max_col-1)
                    self.cur_row = min(max(0, self.cur_row), self.max_row-1)
                    offset = self.cur_row*self.max_col+self.cur_col
                    self.text = self.symbols[offset]
                    self.Rebuild(hDlg)
                    return 0
            return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)

        fdi = self.ffi.new("struct FarDialogItem []", Items)
        hDlg = self.info.DialogInit(self.info.ModuleNumber, -1, -1, 42, 20, self.s2f("Character Map"), fdi, len(fdi), 0, 0, DialogProc, 0)
        res = self.info.DialogRun(hDlg)
        if res == 1:
            self.info.FSF.CopyToClipboard(self.s2f(self.text))
        self.info.DialogFree(hDlg)
