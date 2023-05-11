#!/usr/bin/env vpython3
import random
import logging
from far2l.plugin import PluginBase
from far2l.fardialogbuilder import (
    TEXT, BUTTON, USERCONTROL,
    HLine,
    HSizer, VSizer,
    DialogBuilder
)


log = logging.getLogger(__name__)


class Miner:
    def __init__(self, rows, cols, bombs):
        self.score = 0
        self.rows = rows
        self.cols = cols
        self.bombs = bombs
        self.gamemap = [[0 for col in range(self.cols)] for row in range(self.rows)]
        self.usermap = [['-' for col in range(self.cols)] for row in range(self.rows)]
        self.GenerateMap()
        self.DisplayMap(self.gamemap)

    def GenerateMap(self):
        mmap = self.gamemap
        rows = self.rows
        cols = self.cols
        def mark(y, x):
            if 0 <= y < rows and 0 <= x < cols:
                if mmap[y][x] != 'X':
                    mmap[y][x] += 1
        for num in range(self.bombs):
            while True:
                y = random.randint(0, rows-1)
                x = random.randint(0, cols-1)
                if mmap[y][x] != 'X':
                    break
            mmap[y][x] = 'X'
            mark(y-1, x-1)
            mark(y-1, x)
            mark(y-1, x+1)
            mark(y, x-1)
            mark(y, x+1)
            mark(y+1, x-1)
            mark(y+1, x)
            mark(y+1, x+1)

    def DisplayMap(self, map):
        s = ''.join([str(1+x%9) for x in range(self.cols)])
        print(' ', s)
        i = 0
        for row in map:
            print(1+i%9, "".join(str(cell) for cell in row))
            i += 1

    def CheckWon(self):
        bombs = self.bombs
        for row in self.usermap:
            for cell in row:
                if cell == '-':
                    bombs -= 1
        return bombs == 0


    def OpenMap(self, y, x):
        pmap = self.usermap
        mmap = self.gamemap
        pmap[y][x] = mmap[y][x]
        self.score += 1
        checked = {}
        stack = [(y, x)]
        def mark(y, x):
            if 0 <= y < self.rows and 0 <= x < self.cols:
                if mmap[y][x] != 'X' and pmap[y][x] != mmap[y][x]:
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
            mark(y-1, x-1)
            mark(y-1, x)
            mark(y-1, x+1)
            mark(y, x-1)
            mark(y, x+1)
            mark(y+1, x-1)
            mark(y+1, x)
            mark(y+1, x+1)

    def Game(self):
        print('Find {} bombs.'.format(self.bombs))
        while True:
            self.DisplayMap(self.usermap)
            if self.CheckWon():
                return True
            print("Enter your cell you want to open :")
            try:
                y = input("Y (1 to {}) :".format(self.rows))
                y = int(y) - 1
                assert y in range(0, self.rows)
                x = input("X (1 to {}) :".format(self.cols))
                x = int(x) - 1
                assert x in range(0, self.cols)
            except (ValueError, AssertionError):
                print('Wrong value, try again')
                continue
            if self.gamemap[y][x] == 'X':
                return False
            else:
                self.OpenMap(y, x)

def main():
    GameStatus = True
    levels = {
        'b': (6, 5, 3),
        'i': (6, 6, 8),
        'h': (9, 9, 12),
    }
    while GameStatus:
        level = input("Select your level ({}):".format(', '.join(levels.keys()))).lower()
        if level not in levels.keys():
            print('Unknown level, try again.')
            continue
        cls = Miner(*levels[level])
        won = cls.Game()
        cls.DisplayMap(cls.gamemap)

        msg = ["Game Over!", "You have Won!"][won]
        print("{}, your score: {}".format(msg, cls.score))
        isContinue = input("Do you want to try again? (y/n) :")
        GameStatus = isContinue == 'y'

if __name__ == "__main__":
    main()

class Plugin(PluginBase):
    label = "Python uminer"
    openFrom = ["PLUGINSMENU"]

    def Rebuild(self, dlg, usermap=True):
        self.info.SendDlgMessage(dlg.hDlg, self.ffic.DM_ENABLEREDRAW, 0, 0)
        if usermap:
            mapa = self.game.usermap
        else:
            mapa = self.game.gamemap
        for row in range(self.game.rows):
            for col in range(self.game.cols):
                did = getattr(dlg, 'ID_{}_{}'.format(row, col))
                ch = str(mapa[row][col])
                dlg.SetText(did, ch)
        self.info.SendDlgMessage(dlg.hDlg, self.ffic.DM_ENABLEREDRAW, 1, 0)
        dlg.SetText(dlg.ID_status, 'Score: {} Bombs:{}'.format(self.game.score, self.game.bombs))

    def OpenPlugin(self, OpenFrom):
        self.rows = 9
        self.cols = 16
        self.bombs = 20
        self.game = Miner(self.rows, self.cols, self.bombs)
        self.gamedone = False
        dlg = None

        @self.ffi.callback("FARWINDOWPROC")
        def DialogProc(hDlg, Msg, Param1, Param2):
            if Msg == self.ffic.DN_INITDIALOG:
                self.Rebuild(dlg)
            elif self.gamedone:
                if Param1 == dlg.ID_vrestart:
                    self.gamedone = False
                    self.game = Miner(self.rows, self.cols, self.bombs)
                    self.Rebuild(dlg)
                    return True
            elif Msg == self.ffic.DN_BTNCLICK:
                if Param1 == dlg.ID_vshow:
                    self.Rebuild(dlg, False)
                    return True
                elif Param1 == dlg.ID_vrestart:
                    self.gamedone = False
                    self.game = Miner(self.rows, self.cols, self.bombs)
                    self.Rebuild(dlg)
                    return True
            elif Msg == self.ffic.DN_MOUSECLICK:
                no = Param1 - 1
                row, col = divmod(no, self.game.cols)
                ch = self.game.gamemap[row][col]
                if ch == 'X':
                    self.gamedone = True
                    self.Rebuild(dlg, False)
                    dlg.SetText(dlg.ID_status, 'Game Over')
                    #self.info.SendDlgMessage(dlg.hDlg, self.ffic.DM_CLOSE, 0, 0)
                    return True
                else:
                    self.game.OpenMap(row, col)
                    if self.game.CheckWon():
                        self.gamedone = True
                        dlg.SetText(dlg.ID_status, 'Game Won')
                        #self.info.SendDlgMessage(dlg.hDlg, self.ffic.DM_CLOSE, 1, 0)
                        return True
                self.Rebuild(dlg)
            return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)

        vs = []
        for row in range(self.game.rows):
            hs = []
            for col in range(self.game.cols):
                hs.append(TEXT(' ', '{}_{}'.format(row, col)))
            vs.append(HSizer(*hs))
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
                TEXT(' '*20, 'status'),
                HSizer(
                    BUTTON('vshow', "Show Me", flags=self.ffic.DIF_CENTERGROUP),
                    BUTTON('vrestart', "Again", flags=self.ffic.DIF_CENTERGROUP),
                    BUTTON('vcancel', "Cancel", flags=self.ffic.DIF_CENTERGROUP),
                ),
            ),
        )
        dlg = b.build(-1, -1)

        res = self.info.DialogRun(dlg.hDlg)
        if res == -1:
            msg = 'esc'
        elif res == dlg.ID_vcancel:
            msg = 'cancel'
        elif res == dlg.ID_vshow:
            msg = 'ok'
        elif res == 0:
            msg = 'game over'
        elif res == 1:
            msg = 'game won'
        else:
            msg = 'why ?'
        log.debug('rc={} msg={}'.format(res, msg))
        self.info.DialogFree(dlg.hDlg)
