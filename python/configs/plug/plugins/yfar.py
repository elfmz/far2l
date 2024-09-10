
__author__ = 'Yaroslav Yanovsky'

from far2l.plugin import PluginBase


class File:
    def __init__(self, panel, data, index=None, is_selected=None):
        """
        ``data`` is a FAR_FIND_DATA to build file info around.
        ``index`` is a position of the file on panel.
        """
        self._panel = panel
        self.creation_time = data.ftCreationTime
        self.last_access_time = data.ftLastAccessTime
        self.last_write_time = data.ftLastWriteTime
        self.physical_size = data.nPhysicalSize
        self.file_size = data.nFileSize
        self.file_attributes = data.dwFileAttributes
        self.unix_mode = data.dwUnixMode
        self.file_name = panel._plugin.f2s(data.lpwszFileName)
        self._index = index
        self._is_selected = is_selected

    @property
    def index(self):
        """
        Returns file position on the panel.
        """
        if self._index is not None:
            return self._index
        # index not provided on creation - scan through panel to find it.
        for f in self._panel:
            if f.file_name == self.file_name:
                self._index = f.index
                return f.index
        raise ValueError('Unable to determine index for file {!r}'.format(
            self.file_name))

    @property
    def is_selected(self):
        """
        Returns whether file is selected or not.
        """
        if self._is_selected is None:
            # selection status  not provided on creation - fetch this 
            # info from file panel
            self._is_selected = (self.file_name 
                                 in self._panel._get_selection_dict())
        return self._is_selected


class Panel:
    def __init__(self, plugin, is_active):
        self._plugin = plugin
        self._handle = plugin.ffi.cast('HANDLE', -1 if is_active else -2)

    def _get_info(self):
        return self._plugin._get_control(self._handle, 
                                         self._plugin.ffic.FCTL_GETPANELINFO)

    @property
    def cursor(self):
        return self._get_info().CurrentItem

    @cursor.setter
    def cursor(self, value):
        redraw = self._plugin.ffi.new('struct PanelRedrawInfo *')
        redraw.CurrentItem = value
        redraw.TopPanelItem = -1
        self._plugin.info.Control(self._handle, 
                                  self._plugin.ffic.FCTL_REDRAWPANEL, 0,
                                  self._plugin.ffi.cast("LONG_PTR", redraw))

    @property
    def current(self):
        """
        Returns current file.
        """
        return self[self.cursor]
    
    def __len__(self):
        """
        Returns total amount of files on the panel.
        """
        return self._get_info().ItemsNumber

    def __getitem__(self, index):
        """
        Returns panel file at given ``index``.
        """
        if index < 0 or index >= len(self):
            raise IndexError('Out of range')
        return self._get_item_impl(index)

    def _get_item_impl(self, index, selection_dict=None):
        item = self._plugin._get_control(
            self._handle, self._plugin.ffic.FCTL_GETPANELITEM, index)
        f = File(self, item.FindData, index)
        if selection_dict is not None:
            f._is_selected = f.file_name in selection_dict
        return f

    @property
    def selected(self):
        """
        Returns list of selected files on the panel.
        """
        res = []
        for i in range(self._get_info().SelectedItemsNumber):
            item = self._plugin._get_control(
                self._handle, self._plugin.ffic.FCTL_GETSELECTEDPANELITEM, i)
            res.append(File(self, item.FindData, is_selected=True))
        return res

    def _get_selection_dict(self):
        return {item.file_name: item for item in self.selected}

    def __iter__(self):
        """
        Iterate over all files present on current panel.
        """
        selection = self._get_selection_dict()
        for i in range(len(self)):
            yield self._get_item_impl(i, selection)


class FarPlugin(PluginBase):
    def __init__(self, *args, **kwargs):
        PluginBase.__init__(self, *args, **kwargs)
        # type, first call returns size
        PPI = ('struct PluginPanelItem *', True)
        self._control_return_types = {
            self.ffic.FCTL_GETPANELITEM: PPI,
            self.ffic.FCTL_GETCURRENTPANELITEM: PPI,
            self.ffic.FCTL_GETSELECTEDPANELITEM: PPI,
            self.ffic.FCTL_GETPANELINFO: ('struct PanelInfo *', False),
        }

    def get_panel(self, is_active=True):
        """
        Returns active or passive panel.
        """
        return Panel(self, is_active)

    def alert(self, body, title=''):
        """
        Simple message box.
        ``body`` is a string or a sequence of strings.
        """
        items = [self.s2f(title)] if title else [self.s2f('')]
        if isinstance(body, str):
            if '\n' in body:
                body = body.split('\n')
            else:
                items.append(self.s2f(body))
        if not isinstance(body, str):
            items.extend(self.s2f(s) for s in body)
        # note: needs to be at least 2 items, otherwise message box is not shown
        citems = self.ffi.new("wchar_t *[]", items)
        self.info.Message(
            self.info.ModuleNumber,  # GUID
            self.ffic.FMSG_WARNING | self.ffic.FMSG_MB_OK,  # Flags
            self.ffi.NULL,  # HelpTopic
            citems, len(citems), 1,  # ButtonsNumber
        )

    def _get_control(self, handle, command, arg=0):
        cast_type, reports_size = self._control_return_types[command]
        if reports_size:
            size = self.info.Control(handle, command, arg, 0)
            if size:
                data = self.ffi.new("char []", size)
                self.info.Control(handle, command, arg, 
                                  self.ffi.cast("LONG_PTR", data))
                return self.ffi.cast(cast_type, data)
        else:
            data = self.ffi.new(cast_type)
            if self.info.Control(handle, command, arg, 
                                 self.ffi.cast("LONG_PTR", data)):
                return data
        log.error('Control({}, {}, {}, ...) failed, expecting {}'.format(
            handle, command, arg, cast_type))
        return None

    def menu(self, names, title=''):
        """
        Simple menu. ``names`` is a list of items. Optional
        ``title`` can be provided.
        """
        items = self.ffi.new("struct FarMenuItem []", len(names))
        refs = []
        for i, name in enumerate(names):
            item = items[i]
            item.Selected = item.Checked = item.Separator = 0
            item.Text = txt = self.s2f(name)
            refs.append(txt)
        items[0].Selected = 1
        title = self.s2f(title)
        NULL = self.ffi.NULL
        return self.info.Menu(self.info.ModuleNumber, -1, -1, 0, 
                              self.ffic.FMENU_AUTOHIGHLIGHT
                              | self.ffic.FMENU_WRAPMODE, title,
                              NULL, NULL, NULL, NULL, items, len(items));

