#!/usr/bin/env vpython3
import io
from . import fardialogsizer as sizer
from .fardialog import Dialog

class Element(sizer.Window):
    no = 0
    varname = None
    def __init__(self, varname=None):
        Element.no += 1
        self.varname = varname
        self.no = Element.no

    def makeID(self, dlg):
        if self.varname is not None:
            setattr(dlg, "ID_"+self.varname, self.no)

    def makeItem(self, dlg):
        pass


class Spacer(Element):
    def __init__(self, width=1, height=1):
        super().__init__()
        # non countable ID
        Element.no -= 1
        self.width = width
        self.height = height

    def get_best_size(self):
        return (self.width, self.height)


class TEXT(Element):
    dit = "DI_TEXT"

    def __init__(self, text):
        super().__init__()
        self.text = text

    def get_best_size(self):
        return (len(self.text), 1)

    def makeItem(self, dlg):
        w, h = self.get_best_size()
        dlg.dialogItems.append((
            getattr(dlg.ffic, self.dit),
            self.pos[0],
            self.pos[1],
            self.pos[0] + w - 1,
            self.pos[1] + h - 1,
            0,
            {'Selected': 0},
            0,
            0,
            dlg.s2f(self.text),
            0,
        ))


class EDIT(Element):
    dit = "DI_EDIT"

    def __init__(self, varname, width, maxlength=None):
        super().__init__(varname)
        self.width = width
        self.height = 1
        self.maxlength = maxlength
        self.mask = None

    def get_best_size(self):
        return (self.width+1, self.height)

    def makeItem(self, dlg):
        w, h = self.get_best_size()

        param = {'Selected':0}
        flags = 0
        if self.mask is not None:
            param = {'Mask': dlg.s2f(self.mask)}
            flags = dlg.ffic.DIF_MASKEDIT

        dlg.dialogItems.append((
            getattr(dlg.ffic, self.dit),
            self.pos[0],
            self.pos[1],
            self.pos[0] + self.width,
            self.pos[1] + h - 1,
            0,
            param,
            flags,
            0,
            dlg.ffi.NULL,
            self.maxlength or 0,
        ))


class PASSWORD(EDIT):
    dit = "DI_PSWEDIT"


class MASKED(EDIT):
    dit = "DI_FIXEDIT"

    def __init__(self, varname, mask):
        super().__init__(varname, len(mask), maxlength=len(mask))
        self.mask = mask


class MEMOEDIT(EDIT):
    #dit = "DI_MEMOEDIT"

    def __init__(self, varname, width, height, maxlength=None):
        super().__init__(varname, width, maxlength)
        self.height = height


class BUTTON(Element):
    dit = "DI_BUTTON"

    def __init__(self, varname, text, default=False, flags=0):
        super().__init__(varname)
        self.text = text
        self.default = default
        self.flags = flags

    def get_best_size(self):
        # [ text ]
        return (2 + len(self.text) + 2, 1)

    def makeItem(self, dlg):
        w, h = self.get_best_size()
        dlg.dialogItems.append((
            getattr(dlg.ffic, self.dit),
            self.pos[0],
            self.pos[1],
            self.pos[0] + w,
            self.pos[1] + h - 1,
            0,
            {'Selected':0},
            self.flags,
            dlg.ffic.DIF_DEFAULT if self.default else 0,
            dlg.s2f(self.text),
            0,
        ))


class CHECKBOX(Element):
    dit = "DI_CHECKBOX"

    def __init__(self, varname, text, checked=0):
        super().__init__(varname)
        self.text = text
        self.checked = checked

    def get_best_size(self):
        # [?] text
        return (1 + 1 + 1 + 1 + len(self.text), 1)

    def makeItem(self, dlg):
        w, h = self.get_best_size()
        dlg.dialogItems.append((
            getattr(dlg.ffic, self.dit),
            self.pos[0],
            self.pos[1],
            self.pos[0] + w,
            self.pos[1] + h - 1,
            0,
            {'Selected': self.checked},
            0,
            0,
            dlg.s2f(self.text),
            0,
        ))


class RADIOBUTTON(Element):
    dit = "DI_RADIOBUTTON"

    def __init__(self, varname, text, selected=0, flags=0):
        super().__init__(varname)
        self.text = text
        self.selected = selected
        self.flags = flags

    def get_best_size(self):
        # (?) text
        return (1 + 1 + 1 + 1 + len(self.text), 1)

    def makeItem(self, dlg):
        w, h = self.get_best_size()
        dlg.dialogItems.append((
            getattr(dlg.ffic, self.dit),
            self.pos[0],
            self.pos[1],
            self.pos[0] + w,
            self.pos[1] + h - 1,
            0,
            {'Selected': self.selected},
            self.flags,
            0,
            dlg.s2f(self.text),
            0,
        ))


class COMBOBOX(Element):
    dit = "DI_COMBOBOX"

    def __init__(self, varname, selected, *items):
        super().__init__(varname)
        self.selected = selected
        self.items = items
        self.maxlen = max([len(s) for s in self.items])
        self.flist = None
        self.fitems = None
        self.s2f = None

    def get_best_size(self):
        return (2 + self.maxlen, 1)

    def makeItem(self, dlg):
        w, h = self.get_best_size()

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

        param = {'ListItems':flist}
        dlg.dialogItems.append((
            getattr(dlg.ffic, self.dit),
            self.pos[0],
            self.pos[1],
            self.pos[0] + w - 2,
            self.pos[1] + h - 1,
            0,
            param,
            0,
            0,
            dlg.ffi.NULL,
            0,
        ))


class LISTBOX(Element):
    dit = "DI_LISTBOX"

    def __init__(self, varname, selected, *items):
        super().__init__(varname)
        self.selected = selected
        self.items = items
        self.maxlen = max([len(s) for s in self.items])
        self.flist = None
        self.fitems = None
        self.s2f = None

    def get_best_size(self):
        return (4 + self.maxlen, len(self.items))

    def makeItem(self, dlg):
        w, h = self.get_best_size()

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

        param = {'ListItems':flist}
        dlg.dialogItems.append((
            getattr(dlg.ffic, self.dit),
            self.pos[0],
            self.pos[1],
            self.pos[0] + w - 1,
            self.pos[1] + h - 1,
            0,
            param,
            dlg.ffic.DIF_LISTNOBOX,
            0,
            dlg.ffi.NULL,
            0,
        ))


class USERCONTROL(Element):
    dit = "DI_USERCONTROL"

    def __init__(self, varname, width, height):
        super().__init__(varname)
        self.width
        self.height

    def get_best_size(self):
        return (self.width, self.height)

    def makeItem(self, dlg):
        w, h = self.get_best_size()
        dlg.dialogItems.append((
            getattr(dlg.ffic, self.dit),
            self.pos[0],
            self.pos[1],
            self.pos[0] + w,
            self.pos[1] + h - 1,
            0,
            {'Selected':0},
            0,
            0,
            dlg.ffi.NULL,
            0,
        ))


class HLine(Element):
    dit = "DI_TEXT"

    def get_best_size(self):
        return (1, 1)

    def makeItem(self, dlg):
        dlg.dialogItems.append((
            getattr(dlg.ffic, self.dit),
            0,
            self.pos[1],
            0,
            self.pos[1],
            0,
            {'Selected':0},
            dlg.ffic.DIF_SEPARATOR,
            0,
            dlg.ffi.NULL,
            0,
        ))


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


class DialogBuilder(sizer.HSizer):
    def __init__(self, plugin, dialogProc, title, helptopic, flags, contents, border=(2, 1, 2, 1)):
        super().__init__(border)
        self.plugin = plugin
        self.dialogProc = dialogProc
        self.title = title
        self.helptopic = helptopic
        self.flags = flags
        self.contents = contents
        self.add(contents, border)

    def build(self, x, y):
        # for building dlg.ID_<varname>
        Element.no = 0

        dlg = Dialog(self.plugin)

        w, h = self.get_best_size()
        w = max(w + 1, len(self.title) + 2)

        dlg.width = w
        dlg.height = h

        self.contents.makeID(dlg)
        self.size(3, 1, w, h)

        dlg.dialogItems.append(
            (dlg.ffic.DI_DOUBLEBOX,  3, 1, w, h,  0, {'Selected':0},  0, 0, dlg.s2f(self.title), 0)
        )
        self.contents.makeItem(dlg)

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
            0,
            self.dialogProc,
            0
        )
        return dlg
