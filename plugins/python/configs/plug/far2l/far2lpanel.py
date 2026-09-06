class Panel:
    def __init__(self, plugin):
        self.info = plugin.info
        self.ffic = plugin.ffic
        self.ffi = plugin.ffi
        self.s2f = plugin.s2f
        self.f2s = plugin.f2s

    def _handle(self, active):
        return self.ffi.cast("HANDLE", -1 if active else -2)

    def SetViewMode(self, mode, active=True):
        return self.info.Control(self._handle(active), self.ffic.FCTL_SETVIEWMODE, mode, 0)

    def SetSortMode(self, mode, active=True):
        return self.info.Control(self._handle(active), self.ffic.FCTL_SETSORTMODE, mode, 0)

    def SetNumericSort(self, mode, active=True):
        return self.info.Control(self._handle(active), self.ffic.FCTL_SETNUMERICSORT, mode, 0)

    def SetCaseSensitiveSort(self, mode, active=True):
        return self.info.Control(self._handle(active), self.ffic.FCTL_SETCASESENSITIVESORT, mode, 0)

    def GetPanelPluginHandle(self, active=True):
        result = self.ffi.new("HANDLE *")
        self.info.Control(self._handle(active), self.ffic.FCTL_GETPANELPLUGINHANDLE, 0, self.ffi.cast("LONG_PTR", result))
        return result

    def SetPanelLocation(self, location, active=True):
        # FarPanelLocation *
        return self.info.Control(self._handle(active), self.ffic.FCTL_SETPANELLOCATION, 0, self.ffi.cast("LONG_PTR", result))

    def SetSortOrder(self, ascending, active=True):
        return self.info.Control(self._handle(active), self.ffic.FCTL_SETSORTORDER, 1 if ascending else 0, 0)

    def SetDirectoriesFirst(self, on, active=True):
        return self.info.Control(self._handle(active), self.ffic.FCTL_SETDIRECTORIESFIRST, 1 if on else 0, 0)

    def SetExecutablesFirst(self, on, active=True):
        return self.info.Control(self._handle(active), self.ffic.FCTL_SETEXECUTABLESFIRST, 1 if on else 0, 0)

    def ClosePlugin(self, arg, active=True):
        # wchar_t *
        return self.info.Control(self._handle(active), self.ffic.FCTL_CLOSEPLUGIN, 0, self.ffi.cast("LONG_PTR", arg))

    def GetPanelInfo(self, active=True):
        pnl = self.ffi.new("struct PanelInfo *")
        if self.info.Control(self._handle(active), self.ffic.FCTL_GETPANELINFO, 0, self.ffi.cast("LONG_PTR", pnl)):
            return pnl
        return None

    def GetPanelHostFile(self, active=True):
        nb = self.info.Control(self._handle(active), self.ffic.FCTL_GETPANELHOSTFILE, 0, 0)
        data = self.ffi.new("wchar_t []", nb)
        self.info.Control(self._handle(active), self.ffic.FCTL_GETPANELHOSTFILE, nb, self.ffi.cast("LONG_PTR", data))
        return self.f2s(data)

    def GetPanelFormat(self, active=True):
        nb = self.info.Control(self._handle(active), self.ffic.FCTL_GETPANELFORMAT, 0, 0)
        data = self.ffi.new("wchar_t []", nb)
        self.info.Control(self._handle(active), self.ffic.FCTL_GETPANELFORMAT, nb, self.ffi.cast("LONG_PTR", data))
        return self.f2s(data)

    def GetPanelDir(self, active=True):
        nb = self.info.Control(self._handle(active), self.ffic.FCTL_GETPANELDIR, 0, 0)
        data = self.ffi.new("wchar_t []", nb)
        self.info.Control(self._handle(active), self.ffic.FCTL_GETPANELDIR, nb, self.ffi.cast("LONG_PTR", data))
        return self.f2s(data)

    def GetColumnTypes(self, active=True):
        nb = self.info.Control(self._handle(active), self.ffic.FCTL_GETCOLUMNTYPES, 0, 0)
        data = self.ffi.new("wchar_t []", nb)
        self.info.Control(self._handle(active), self.ffic.FCTL_GETCOLUMNTYPES, nb, self.ffi.cast("LONG_PTR", data))
        return self.f2s(data)

    def GetColumnWidths(self, active=True):
        nb = self.info.Control(self._handle(active), self.ffic.FCTL_GETCOLUMNWIDTHS, 0, 0)
        data = self.ffi.new("wchar_t []", nb)
        self.info.Control(self._handle(active), self.ffic.FCTL_GETCOLUMNWIDTHS, nb, self.ffi.cast("LONG_PTR", data))
        return self.f2s(data)

    def GetPanelItem(self, no, active=True):
        # get buffer size
        rc = self.info.Control(self._handle(active), self.ffic.FCTL_GETPANELITEM, no, 0)
        if rc:
            # allocate buffer
            data = self.ffi.new("char []", rc)
            rc = self.info.Control(self._handle(active), self.ffic.FCTL_GETPANELITEM, no, self.ffi.cast("LONG_PTR", data))
            # cast buffer to PluginPanelItem *
            pnli = self.ffi.cast("struct PluginPanelItem *", data)
            return pnli, data
        return None, None

    def GetSelectedPanelItem(self, no, active=True):
        # get buffer size
        rc = self.info.Control(self._handle(active), self.ffic.FCTL_GETSELECTEDPANELITEM, no, 0)
        if rc:
            # allocate buffer
            data = self.ffi.new("char []", rc)
            rc = self.info.Control(self._handle(active), self.ffic.FCTL_GETSELECTEDPANELITEM, no, self.ffi.cast("LONG_PTR", data))
            # cast buffer to PluginPanelItem *
            pnli = self.ffi.cast("struct PluginPanelItem *", data)
            return pnli, data
        return None, None

    def GetCurrentPanelItem(self, active=True):
        # get buffer size
        rc = self.info.Control(self._handle(active), self.ffic.FCTL_GETCURRENTPANELITEM, 0, 0)
        if rc:
            # allocate buffer
            data = self.ffi.new("char []", rc)
            rc = self.info.Control(self._handle(active), self.ffic.FCTL_GETCURRENTPANELITEM, 0, self.ffi.cast("LONG_PTR", data))
            # cast buffer to PluginPanelItem *
            pnli = self.ffi.cast("struct PluginPanelItem *", data)
            return pnli, data
        return None, None

    def BeginSelection(self, active=True):
        return self.info.Control(self._handle(active), self.ffic.FCTL_BEGINSELECTION, 0, 0)

    def SetSelection(self, no, on, active=True):
        on = 1 if on else 0
        return self.info.Control(self._handle(active), self.ffic.FCTL_SETSELECTION, no, self.ffi.cast("LONG_PTR", on))

    def ClearSelection(self, no, active=True):
        return self.info.Control(self._handle(active), self.ffic.FCTL_CLEARSELECTION, no, 0)

    def EndSelection(self, no, active=True):
        return self.info.Control(self._handle(active), self.ffic.FCTL_ENDSELECTION, 0, 0)

    def UpdatePanel(self, keepselection=True, active=True):
        keepselection = 1 if keepselection else 0
        return self.info.Control(self._handle(active), self.ffic.FCTL_UPDATEPANEL, keepselection, 0)

    def RedrawPanel(self, panelinfo=0, active=True):
        return self.info.Control(self._handle(active), self.ffic.FCTL_REDRAWPANEL, 0, self.ffi.cast("LONG_PTR", panelinfo))

    def SetPanelDir(self, paneldir, active=True):
        paneldir = self.s2f(paneldir)
        return self.info.Control(self._handle(active), self.ffic.FCTL_SETPANELDIR, 0, self.ffi.cast("LONG_PTR", paneldir))
