import io
import logging
from . import fardialogsizer as sizer
from .fardialog import Dialog


log = logging.getLogger(__name__)


class Element(sizer.Window):
    no = 0
    varname = None

    def __init__(
        self, varname=None, focus=0, param=None, flags=0, default=0, maxlength=0
    ):
        self.varname = varname
        self.focus = focus
        self.param = param or {"Selected": 0}
        self.flags = flags
        self.default = default
        self.maxlength = maxlength
        self.data = None

    def makeID(self, dlg):
        self.no = Element.no
        Element.no += 1
        if self.varname is not None:
            setattr(dlg, "ID_" + self.varname, self.no)

    def makeItem(self, dlg):
        w, h = self.get_best_size()
        if self.data is None:
            self.data = dlg.ffi.NULL
        item = [
            getattr(dlg.ffic, self.dit),
            self.pos[0],
            self.pos[1],
            self.pos[0] + w - 1,
            self.pos[1] + h - 1,
            self.focus,
            self.param,
            self.flags,
            self.default,
            self.data,
            self.maxlength,
        ]
        dlg.dialogItems.append(item)
        # if self.varname is not None:
        #    dlg.id2item[getattr(dlg, "ID_"+self.varname)] = [item, None]


class Spacer(Element):
    def __init__(self, width=1, height=1):
        super().__init__()
        self.width = width
        self.height = height

    def makeID(self, dlg):
        # non countable ID
        pass

    def get_best_size(self):
        return (self.width, self.height)

    def makeItem(self, dlg):
        pass

class TEXT(Element):
    dit = "DI_TEXT"

    def __init__(self, varname, text, **kwargs):
        super().__init__(varname, **kwargs)
        self.text = text

    def get_best_size(self):
        return (len(self.text.replace('&','')), 1)

    def makeItem(self, dlg):
        self.data = dlg.s2f(self.text)
        super().makeItem(dlg)


class EDIT(Element):
    dit = "DI_EDIT"

    def __init__(self, varname, width, mask=None, height=1, **kwargs):
        if "history" in kwargs:
            history = kwargs.pop("history")
        else:
            history = None
        super().__init__(varname, **kwargs)
        self.width = width
        self.height = height
        self.mask = None
        self.history = history

    def get_best_size(self):
        return (self.width, self.height)

    def makeItem(self, dlg):
        self.data = dlg.ffi.NULL
        if self.mask is not None:
            self.param = {"Mask": dlg.s2f(self.mask)}
            self.flags = dlg.ffic.DIF_MASKEDIT
        elif self.history is not None:
            self.param = {"History": dlg.s2f(self.history)}
            self.flags = dlg.ffic.DIF_HISTORY
        super().makeItem(dlg)


class PASSWORD(EDIT):
    dit = "DI_PSWEDIT"


class MASKED(EDIT):
    dit = "DI_FIXEDIT"

    def __init__(self, varname, mask, **kwargs):
        super().__init__(varname, len(mask), mask=mask, maxlength=len(mask), **kwargs)


class MEMOEDIT(EDIT):
    # dit = "DI_MEMOEDIT"

    def __init__(self, varname, width, height, maxlength=None):
        raise NotImplemented("MEMOEDIT")


class BUTTON(Element):
    dit = "DI_BUTTON"

    def __init__(self, varname, text, **kwargs):
        super().__init__(varname, **kwargs)
        self.text = text

    def get_best_size(self):
        # [ text ]
        return (4 + len(self.text.replace('&','')), 1)

    def makeItem(self, dlg):
        self.data = dlg.s2f(self.text)
        self.default = dlg.ffic.DIF_DEFAULT if self.default else 0
        super().makeItem(dlg)


class CHECKBOX(Element):
    dit = "DI_CHECKBOX"

    def __init__(self, varname, text, checked=0, **kwargs):
        super().__init__(varname, **kwargs)
        self.text = text
        self.param = {"Selected": checked}

    def get_best_size(self):
        # [?] text
        return (4 + len(self.text.replace('&','')), 1)

    def makeItem(self, dlg):
        self.data = dlg.s2f(self.text)
        super().makeItem(dlg)


class RADIOBUTTON(Element):
    dit = "DI_RADIOBUTTON"

    def __init__(self, varname, text, selected=0, **kwargs):
        super().__init__(varname, **kwargs)
        self.text = text
        self.param = {"Selected": selected}

    def get_best_size(self):
        # (?) text
        return (4 + len(self.text.replace('&','')), 1)

    def makeItem(self, dlg):
        self.data = dlg.s2f(self.text)
        super().makeItem(dlg)


class COMBOBOX(Element):
    dit = "DI_COMBOBOX"

    def __init__(self, varname, selected, items, **kwargs):
        if "width" in kwargs:
            width = kwargs.pop("width")
        else:
            width = max([len(s) for s in items])
        super().__init__(varname, **kwargs)
        self.selected = selected
        self.items = items
        self.width = width

    def get_best_size(self):
        return (2 + self.width, 1)

    def makeItem(self, dlg):
        self.data = dlg.ffi.NULL
        s2f = []
        fitems = dlg.ffi.new("struct FarListItem []", len(self.items))
        for i in range(len(self.items)):
            fitems[i].Flags = dlg.ffic.LIF_SELECTED if i == self.selected else 0
            s = dlg.s2f(self.items[i])
            s2f.append(s)
            fitems[i].Text = s

        flist = dlg.ffi.new("struct FarList *")
        flist.ItemsNumber = len(self.items)
        flist.Items = fitems

        self.flist = flist
        self.fitems = fitems
        self.s2f = s2f

        self.param = {"ListItems": flist}
        super().makeItem(dlg)


class LISTBOX(Element):
    dit = "DI_LISTBOX"

    def __init__(self, varname, selected, items, **kwargs):
        if "height" in kwargs:
            height = kwargs.pop("height")
        else:
            height = len(items) + 2
        super().__init__(varname, **kwargs)
        self.selected = selected
        self.items = items
        self.width = max([len(s) for s in self.items])
        self.height = height

    def get_best_size(self):
        return (4 + self.width, self.height)

    def makeItem(self, dlg):
        self.data = dlg.ffi.NULL
        s2f = []
        fitems = dlg.ffi.new("struct FarListItem []", len(self.items))
        for i in range(len(self.items)):
            fitems[i].Flags = dlg.ffic.LIF_SELECTED if i == self.selected else 0
            s = dlg.s2f(self.items[i])
            s2f.append(s)
            fitems[i].Text = s

        flist = dlg.ffi.new("struct FarList *")
        flist.ItemsNumber = len(self.items)
        flist.Items = fitems

        self.flist = flist
        self.fitems = fitems
        self.s2f = s2f

        self.param = {"ListItems": flist}
        super().makeItem(dlg)


class USERCONTROL(Element):
    dit = "DI_USERCONTROL"

    def __init__(self, varname, width, height, **kwargs):
        super().__init__(varname, **kwargs)
        self.width = width
        self.height = height

    def get_best_size(self):
        return (self.width, self.height)

    def makeItem(self, dlg):
        self.data = dlg.ffi.NULL
        super().makeItem(dlg)


class HLine(Element):
    dit = "DI_TEXT"

    def __init__(self):
        super().__init__()

    def get_best_size(self):
        return (1, 1)

    def makeItem(self, dlg):
        self.data = dlg.ffi.NULL
        self.flags = dlg.ffic.DIF_SEPARATOR
        dlg.dialogItems.append(
            (
                getattr(dlg.ffic, self.dit),
                0,
                self.pos[1],
                0,
                self.pos[1],
                self.focus,
                self.param,
                self.flags,
                self.default,
                self.data,
                self.maxlength,
            )
        )


class HSizer(sizer.HSizer):
    def __init__(self, *controls, border=(0, 0, 1, 0), center=False):
        super().__init__()
        self.controls = controls
        self.center = center
        for control in controls:
            self.add(control, border)

    def makeID(self, dlg):
        for control in self.controls:
            control.makeID(dlg)

    def makeItem(self, dlg):
        for control in self.controls:
            control.makeItem(dlg)


class VSizer(sizer.VSizer):
    def __init__(self, *controls, border=(0, 0, 0, 0)):
        super().__init__()
        self.controls = controls
        for control in controls:
            self.add(control, border)

    def makeID(self, dlg):
        for control in self.controls:
            control.makeID(dlg)

    def makeItem(self, dlg):
        for control in self.controls:
            control.makeItem(dlg)


class FlowSizer(sizer.FlowSizer):
    def __init__(self, cols, *controls, border=(0, 0, 0, 0)):
        super().__init__(cols)
        self.controls = controls
        for control in controls:
            self.add(control, border)

    def makeID(self, dlg):
        for control in self.controls:
            control.makeID(dlg)

    def makeItem(self, dlg):
        for control in self.controls:
            control.makeItem(dlg)


class DialogBuilder(sizer.HSizer):
    def __init__(
        self, plugin, dialogProc, title, helptopic, flags, contents, border=(2, 1, 2, 1)
    ):
        super().__init__(border)
        self.plugin = plugin
        self.dialogProc = dialogProc
        self.title = title
        self.helptopic = helptopic
        self.flags = flags
        self.contents = contents
        self.add(contents, border)

    def build(self, x, y):

        dlg = Dialog(self.plugin)

        w, h = self.get_best_size()
        w = max(w + 1, len(self.title) + 2)

        dlg.width = w
        dlg.height = h

        # for building dlg.ID_<varname>
        Element.no = 1
        self.contents.makeID(dlg)
        self.size(3, 1, w, h)

        dlg.dialogItems.insert(0,
            (
                dlg.ffic.DI_DOUBLEBOX,
                3,
                1,
                w,
                h,
                0,
                {"Selected": 0},
                0,
                0,
                dlg.s2f(self.title),
                0,
            )
        )
        self.contents.makeItem(dlg)

        # for item in dlg.dialogItems:
        #    log.debug('{}'.format(item))

        dlg.fdi = dlg.ffi.new("struct FarDialogItem []", dlg.dialogItems)
        dlg.hDlg = dlg.info.DialogInit(
            dlg.info.ModuleNumber,
            x,
            y,
            w + 4,
            h + 2,
            dlg.s2f(self.helptopic),
            dlg.fdi,
            len(dlg.fdi),
            0,
            self.flags,
            self.dialogProc,
            0,
        )
        return dlg

    def build_nobox(self, x, y, w, h):

        dlg = Dialog(self.plugin)

        dlg.width = w
        dlg.height = h

        # for building dlg.ID_<varname>
        Element.no = 0
        self.contents.makeID(dlg)
        self.size(0, 0, w, h)

        self.contents.makeItem(dlg)

        dlg.fdi = dlg.ffi.new("struct FarDialogItem []", dlg.dialogItems)
        dlg.hDlg = dlg.info.DialogInit(
            dlg.info.ModuleNumber,
            x,
            y,
            w,
            h,
            dlg.s2f(self.helptopic),
            dlg.fdi,
            len(dlg.fdi),
            0,
            self.flags,
            self.dialogProc,
            0,
        )
        return dlg
