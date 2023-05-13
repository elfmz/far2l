import logging


log = logging.getLogger(__name__)


class Dialog:
    def __init__(self, plugin):
        self.info = plugin.info
        self.ffic = plugin.ffic
        self.ffi = plugin.ffi
        self.s2f = plugin.s2f
        self.f2s = plugin.f2s

        self.width = 0
        self.height = 0
        self.dialogItems = []
        # self.id2item = {}
        self.fdi = None
        self.hDlg = None

    def Close(self, exitcode):
        self.info.SendDlgMessage(self.hDlg, self.ffic.DM_CLOSE, exitcode, 0)

    def EnableRedraw(self, on):
        self.info.SendDlgMessage(
            self.hDlg, self.ffic.DM_ENABLEREDRAW, 1 if on else 0, 0
        )

    def RedrawDialog(self):
        self.info.SendDlgMessage(self.hDlg, self.ffic.DM_REDRAW, 0, 0)

    def GetDlgData(self):
        return self.info.SendDlgMessage(self.hDlg, self.ffic.DM_GETDLGDATA, 0, 0)

    def SetDlgData(self, Data):
        self.info.SendDlgMessage(self.hDlg, self.ffic.DM_SETDLGDATA, 0, Data)

    def GetDlgItemData(self, ID):
        return self.info.SendDlgMessage(self.hDlg, self.ffic.DM_GETITEMDATA, ID, 0)

    def SetDlgItemData(self, ID, Data):
        self.info.SendDlgMessage(self.hDlg, self.ffic.DM_SETITEMDATA, ID, Data)

    def GetFocus(self, hDlg):
        return self.info.SendDlgMessage(self.hDlg, self.ffic.DM_GETFOCUS, 0, 0)

    def SetFocus(self, ID):
        self.info.SendDlgMessage(self.hDlg, self.ffic.DM_SETFOCUS, ID, 0)

    def Enable(self, ID):
        self.info.SendDlgMessage(self.hDlg, self.ffic.DM_ENABLE, ID, 1)

    def Disable(self, ID):
        self.info.SendDlgMessage(self.hDlg, self.ffic.DM_ENABLE, ID, 0)

    def IsEnable(self, ID):
        return self.info.SendDlgMessage(self.hDlg, self.ffic.DM_ENABLE, ID, -1)

    def GetTextLength(self, ID):
        return self.info.SendDlgMessage(self.hDlg, self.ffic.DM_GETTEXTPTR, ID, 0)

    def GetText(self, ID):
        sptr = self.info.SendDlgMessage(self.hDlg, self.ffic.DM_GETCONSTTEXTPTR, ID, 0)
        return self.f2s(sptr)

    def SetText(self, ID, Str):
        sptr = self.s2f(Str)
        # preserve item.data ?
        # self.id2item[ID][1] = sptr
        self.info.SendDlgMessage(
            self.hDlg, self.ffic.DM_SETTEXTPTR, ID, self.ffi.cast("LONG_PTR", sptr)
        )

    def GetCheck(self, ID):
        return self.info.SendDlgMessage(self.hDlg, self.ffic.DM_GETCHECK, ID, 0)

    def SetCheck(self, ID, State):
        self.info.SendDlgMessage(self.hDlg, self.ffic.DM_SETCHECK, ID, State)

    def AddHistory(self, ID, Str):
        self.info.SendDlgMessage(self.hDlg, self.ffic.DM_ADDHISTORY, ID, Str)

    def AddString(self, ID, Str):
        self.info.SendDlgMessage(self.hDlg, self.ffic.DM_LISTADDSTR, ID, Str)

    def GetCurPos(self, ID):
        return self.info.SendDlgMessage(self.hDlg, self.ffic.DM_LISTGETCURPOS, ID, 0)

    # def SetCurPos(self, ID, NewPos):
    #    struct FarListPos LPos={NewPos, -1}
    #    self.info.SendDlgMessage(self.hDlg, self.ffic.DM_LISTSETCURPOS, ID, (LONG_PTR)&LPos)

    def ClearList(self, ID):
        self.info.SendDlgMessage(self.hDlg, self.ffic.DM_LISTDELETE, ID, 0)

    # def DeleteItem(self, ID, Index):
    #    struct FarListDelete FLDItem={Index, 1}
    #    self.info.SendDlgMessage(self.hDlg, self.ffic.DM_LISTDELETE, ID, (LONG_PTR)&FLDItem)

    def SortUp(self, ID):
        self.info.SendDlgMessage(self.hDlg, self.ffic.DM_LISTSORT, ID, 0)

    def SortDown(self, ID):
        self.info.SendDlgMessage(self.hDlg, self.ffic.DM_LISTSORT, ID, 1)

    def GetItemData(self, ID, Index):
        return self.info.SendDlgMessage(self.hDlg, self.ffic.DM_LISTGETDATA, ID, Index)

    # def SetItemStrAsData(self, ID, Index, Str):
    #    struct FarListItemData FLID{Index, 0, Str, 0}
    #    self.info.SendDlgMessage(self.hDlg, self.ffic.DM_LISTSETDATA, ID, (LONG_PTR)&FLID)
