import random
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


class Miner:
    def __init__(self, rows, cols, bombs):
        self.score = 0
        self.rows = rows
        self.cols = cols
        self.bombs = bombs
        self.gamemap = [[0 for col in range(self.cols)] for row in range(self.rows)]
        self.usermap = [["-" for col in range(self.cols)] for row in range(self.rows)]
        self.GenerateMap()
        self.gameover = False
        self.gamewon = False

    @property
    def done(self):
        return self.gameover or self.gamewon

    def GenerateMap(self):
        mmap = self.gamemap
        rows = self.rows
        cols = self.cols

        def mark(y, x):
            if 0 <= y < rows and 0 <= x < cols:
                if mmap[y][x] != "X":
                    mmap[y][x] += 1

        for num in range(self.bombs):
            while True:
                y = random.randint(0, rows - 1)
                x = random.randint(0, cols - 1)
                if mmap[y][x] != "X":
                    break
            mmap[y][x] = "X"
            mark(y - 1, x - 1)
            mark(y - 1, x)
            mark(y - 1, x + 1)
            mark(y, x - 1)
            mark(y, x + 1)
            mark(y + 1, x - 1)
            mark(y + 1, x)
            mark(y + 1, x + 1)

    def CheckWon(self):
        bombs = self.bombs
        for row in self.usermap:
            for cell in row:
                if cell == "-":
                    bombs -= 1
        self.gamewon = bombs == 0

    def OpenMap(self, y, x):
        ch = self.gamemap[y][x]
        if ch == "X":
            self.gameover = True
            self.gamewon = False
            return
        pmap = self.usermap
        mmap = self.gamemap
        pmap[y][x] = mmap[y][x]
        self.score += 1
        checked = {}
        stack = [(y, x)]

        def mark(y, x):
            if 0 <= y < self.rows and 0 <= x < self.cols:
                if mmap[y][x] != "X" and pmap[y][x] != mmap[y][x]:
                    self.score += 1
                    pmap[y][x] = mmap[y][x]
                stack.append((y, x))

        while len(stack):
            y, x = stack.pop()
            if checked.get((y, x)) == True:
                continue
            checked[(y, x)] = True
            if mmap[y][x] != 0:
                continue
            pmap[y][x] = 0
            mark(y - 1, x - 1)
            mark(y - 1, x)
            mark(y - 1, x + 1)
            mark(y, x - 1)
            mark(y, x + 1)
            mark(y + 1, x - 1)
            mark(y + 1, x)
            mark(y + 1, x + 1)

        self.CheckWon()


class Plugin(PluginBase):
    label = "Python uminer"
    openFrom = ["PLUGINSMENU"]

    def Redraw(self, dlg, usermap=True):
        # dlg.EnableRedraw(False)
        mapa = self.game.usermap if usermap else self.game.gamemap
        for row in range(self.game.rows):
            for col in range(self.game.cols):
                did = getattr(dlg, "ID_{}_{}".format(row, col))
                ch = str(mapa[row][col])
                dlg.SetText(did, ch)
        dlg.SetText(dlg.ID_bombs, "Bombs: {:2} ".format(self.bombs))
        msg = "Score: {:2}".format(self.game.score)
        if self.game.gameover:
            msg += ", Game Over"
        elif self.game.gamewon:
            msg += ", Game Won"
        dlg.SetText(dlg.ID_status, msg)
        # dlg.EnableRedraw(True)
        # dlg.RedrawDialog()

    def OpenPlugin(self, OpenFrom):
        self.rows = 9
        self.cols = 16
        self.bombs = 10
        self.game = Miner(self.rows, self.cols, self.bombs)
        dlg = None

        def OnButton(hDlg, Msg, Param1, Param2):
            if Param1 == dlg.ID_vshow:
                self.game.gameover = True
                self.Redraw(dlg, False)
            elif Param1 == dlg.ID_vrestart:
                self.game = Miner(self.rows, self.cols, self.bombs)
                self.Redraw(dlg)
            elif Param1 == dlg.ID_vbplus:
                self.game.gameover = True
                if self.bombs < (self.rows * self.cols) // 2:
                    self.bombs += 1
                    self.Redraw(dlg, False)
            elif Param1 == dlg.ID_vbminus:
                self.game.gameover = True
                if self.bombs > 3:
                    self.bombs -= 1
                    self.Redraw(dlg, False)
            elif Param1 == dlg.ID_vcancel:
                return self.Close(dlg.ID_vcancel)
            else:
                return False
            return True

        def OnMouseClick(hDlg, Msg, Param1, Param2):
            idmin = getattr(dlg, "ID_{}_{}".format(0, 0))
            idmax = getattr(dlg, "ID_{}_{}".format(self.rows - 1, self.cols - 1))

            if Param1 < idmin or Param1 > idmax or self.game.done:
                return False
                # return OnButton(hDlg, Msg, Param1, Param2)

            no = Param1 - 1
            row, col = divmod(no, self.game.cols)
            self.game.OpenMap(row, col)
            if self.game.gameover:
                self.game.gamemap[row][col] = "O"
                self.Redraw(dlg, False)
            else:
                self.Redraw(dlg)
            return True

        @self.ffi.callback("FARWINDOWPROC")
        def DialogProc(hDlg, Msg, Param1, Param2):
            if Msg == self.ffic.DN_INITDIALOG:
                self.Redraw(dlg)
            elif Msg == self.ffic.DN_BTNCLICK:
                return OnButton(hDlg, Msg, Param1, Param2)
            elif Msg == self.ffic.DN_MOUSECLICK:
                if not self.game.done:
                    return OnMouseClick(hDlg, Msg, Param1, Param2)
            return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)

        vs = []
        for row in range(self.game.rows):
            hs = []
            for col in range(self.game.cols):
                hs.append(TEXT("{}_{}".format(row, col), " "))
            vs.append(HSizer(*hs, border=(0, 0, 0, 0)))
        vs = VSizer(*vs)

        b = DialogBuilder(
            self,
            DialogProc,
            "Python Miner",
            "miner",
            0,
            VSizer(
                vs,
                HLine(),
                TEXT("status", " " * 25),
                HSizer(
                    BUTTON("vshow", "Show Me", flags=self.ffic.DIF_CENTERGROUP),
                    TEXT(
                        "bombs",
                        "Bombs: {:2} ".format(self.game.bombs),
                        flags=self.ffic.DIF_CENTERGROUP,
                    ),
                    BUTTON("vbplus", "+", flags=self.ffic.DIF_CENTERGROUP),
                    BUTTON("vbminus", "-", flags=self.ffic.DIF_CENTERGROUP),
                ),
                HSizer(
                    BUTTON("vrestart", "Again", flags=self.ffic.DIF_CENTERGROUP),
                    BUTTON("vcancel", "Cancel", flags=self.ffic.DIF_CENTERGROUP),
                ),
            ),
        )
        dlg = b.build(-1, -1)

        res = self.info.DialogRun(dlg.hDlg)
        if res == -1:
            msg = "esc"
        elif res == dlg.ID_vcancel:
            msg = "cancel"
        else:
            msg = "why ?"
        log.debug("rc={} msg={}".format(res, msg))
        self.info.DialogFree(dlg.hDlg)
