
"""
This is a work-in-progress library which tries
to make writing FAR plugins as simple as possible by hiding
all of the C API behind pythonic abstractions.

Everything that is not underscored is for public usage,
but until majority of FAR API will be accessible via
this library, interfaces will be changed frequently.
"""

__author__ = 'Yaroslav Yanovsky'

import logging
import os

from far2l.plugin import PluginBase

_log = logging.getLogger(__name__)


class _File:
    """
    Class that represents file on FAR panel.
    Do not create directly, only use properites.
    """
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
    def full_file_name(self):
        """
        Tries to provide full file name if possible.
        """
        return os.path.join(self._panel.directory, self.file_name)

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


class _Panel:
    """
    Class that represents FAR panel.
    Do not create directly, only use properites.
    """
    def __init__(self, plugin, is_active):
        self._plugin = plugin
        self._handle = plugin.ffi.cast('HANDLE', -1 if is_active else -2)

        PPI = (self._get_control_sized, 'struct PluginPanelItem *')
        STR = (self._get_control_str, None)
        PI = (self._get_control_fixed, 'struct PanelInfo *')
        c = plugin.ffic
        self._CONTROL_RETURN_TYPES = {
            c.FCTL_GETPANELITEM: PPI,
            c.FCTL_GETCURRENTPANELITEM: PPI,
            c.FCTL_GETSELECTEDPANELITEM: PPI,
            c.FCTL_GETPANELINFO: PI,
            c.FCTL_GETPANELDIR: STR,
            c.FCTL_GETPANELFORMAT: STR,
            c.FCTL_GETPANELHOSTFILE: STR,
        }

    @property
    def directory(self):
        """
        Returns current panel directory.
        """
        return self._get_control(self._plugin.ffic.FCTL_GETPANELDIR)

    @property
    def cursor(self):
        """
        Returns index of file under cursor.
        """
        return self._get_info().CurrentItem

    @cursor.setter
    def cursor(self, index):
        """
        Changes file cursor - ``index`` is a file index in the panel.
        """
        if index < 0 or index >= len(self):
            raise IndexError('Out of range')
        redraw = self._plugin.ffi.new('struct PanelRedrawInfo *')
        redraw.CurrentItem = index
        redraw.TopPanelItem = -1
        self._plugin.info.Control(self._handle, 
                                  self._plugin.ffic.FCTL_REDRAWPANEL, 0,
                                  self._plugin.ffi.cast('LONG_PTR', redraw))

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
        item = self._get_control(self._plugin.ffic.FCTL_GETPANELITEM, index)
        f = _File(self, item.FindData, index)
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
            item = self._get_control(
                self._plugin.ffic.FCTL_GETSELECTEDPANELITEM, i)
            res.append(_File(self, item.FindData, is_selected=True))
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

    def refresh(self):
        """
        Updates and redraws this panel.
        """
        p = self._plugin
        p.info.Control(self._handle, p.ffic.FCTL_UPDATEPANEL, 0, 0)
        p.info.Control(self._handle, p.ffic.FCTL_REDRAWPANEL, 0, 0)

    def _get_control_str(self, _, command, arg=0):
        p = self._plugin
        char_count = p.info.Control(self._handle, command, 0, 0)
        data = p.ffi.new('wchar_t []', char_count)
        p.info.Control(self._handle, command, char_count, 
                       p.ffi.cast('LONG_PTR', data))
        return p.f2s(data)
        
    def _get_control_sized(self, cast_type, command, arg=0):
        p = self._plugin
        size = p.info.Control(self._handle, command, arg, 0)
        if size:
            data = p.ffi.new('char []', size)
            p.info.Control(self._handle, command, arg, 
                           p.ffi.cast('LONG_PTR', data))
            return p.ffi.cast(cast_type, data)
        return None

    def _get_control_fixed(self, cast_type, command, arg=0):
        p = self._plugin
        data = p.ffi.new(cast_type)
        if p.info.Control(self._handle, command, arg, 
                          p.ffi.cast('LONG_PTR', data)):
            return data
        return None

    def _get_control(self, command, arg=0):
        func, cast_type = self._CONTROL_RETURN_TYPES[command]
        result = func(cast_type, command, arg)
        if result is None:
            _log.error('Control({}, {}, {}, ...) failed, expecting {}'.format(
                self._handle, command, arg, cast_type))
        return result

    def _get_info(self):
        return self._get_control(self._plugin.ffic.FCTL_GETPANELINFO)


class FarPlugin(PluginBase):
    """
    More simplistic FAR plugin interface. Provides functions that
    hide away lower level Plugin C API and allow to write "pythonic" plugins.
    """
    def __init__(self, *args, **kwargs):
        PluginBase.__init__(self, *args, **kwargs)
        # type, first call returns size

    def get_panel(self, is_active=True):
        """
        Returns active or passive panel.
        """
        return _Panel(self, is_active)

    def notice(self, body, title=None):
        """
        Simple message box.
        ``body`` is a string or a sequence of strings.
        """
        return self._popup(body, title, 0)

    def error(self, body, title=None):
        """
        Same as .notice(), but message box type is error
        """
        return self._popup(body, title, self.ffic.FMSG_WARNING)

    def _popup(self, body, title, flags):
        items = [self.s2f(title)] if title else [self.s2f('')]
        if isinstance(body, str):
            if '\n' in body:
                body = body.split('\n')
            else:
                items.append(self.s2f(body))
        if not isinstance(body, str):
            items.extend(self.s2f(s) for s in body)
        # note: needs to be at least 2 items, otherwise message 
        # box is not shown
        citems = self.ffi.new('wchar_t *[]', items)
        self.info.Message(
            self.info.ModuleNumber,  # GUID
            flags | self.ffic.FMSG_MB_OK,  # Flags
            self.ffi.NULL,  # HelpTopic
            citems, len(citems), 1,  # ButtonsNumber
        )

    def editor(self, fn, title=None, line=1, column=1, encoding=65001):
        """
        Invokes Far editor on file ``fn``. If ``title`` is given, that
        text is used as editor title, otherwise edited file name is used.

        ``line`` and ``column`` indicate where cursor position will be
        placed after opening.

        Returns True if file was changed, False if it was not and None
        in case of error.
        """
        if title is None:
            title = fn
        fn = self.s2f(fn)
        title = self.s2f(title)
        result = self.info.Editor(fn, title, 0, 0, -1, -1, 
                                  self.ffic.EF_DISABLEHISTORY, line, column, 
                                  encoding)
        if result == self.ffic.EEC_MODIFIED:
            return True
        elif result == self.ffic.EEC_NOT_MODIFIED:
            return False
        return None

    def menu(self, names, title=''):
        """
        Simple menu. ``names`` is a list of items. Optional
        ``title`` can be provided.
        """
        items = self.ffi.new('struct FarMenuItem []', len(names))
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

