import logging

from far2lc import CheckForEscape


log = logging.getLogger(__name__)
PROGRESS_WIDTH = 30


class ProgressMessage(object):
    def __init__(self, plugin, title, message, maxvalue):
        super().__init__()
        self.info = plugin.info
        self.ffi = plugin.ffi
        self.ffic = plugin.ffic

        self.visible = False
        self.title = title
        self.message = message
        self.maxvalue = maxvalue
        self.setbar(0)

    def s2f(self, s):
        return self.ffi.new("wchar_t []", s)

    def f2s(self, s):
        return self.ffi.string(self.ffi.cast("wchar_t *", s))

    def adv(self, cmd, par):
        self.info.AdvControl(
            self.info.ModuleNumber,
            cmd,
            self.ffi.cast("void *", par),
            self.ffi.NULL
        )

    def show(self):
        if not self.visible:
            self.adv(self.ffic.ACTL_SETPROGRESSSTATE, self.ffic.PGS_INDETERMINATE)
            self.visible = True
        msg = [ self.s2f(self.title), self.s2f(self.message), self.s2f(self.bar) ]
        cmsg = self.ffi.new("wchar_t *[]", msg)
        self.info.Message(
            self.info.ModuleNumber,
            0,
            self.ffi.NULL,
            cmsg,
            len(msg),
            0
        )

    def hide(self):
        if not self.visible:
            return
        self.adv(self.ffic.ACTL_PROGRESSNOTIFY, 0)
        self.adv(self.ffic.ACTL_SETPROGRESSSTATE, self.ffic.PGS_NOPROGRESS)
        self.visible = False

        PANEL_ACTIVE    =self.ffi.cast("HANDLE", -1)
        PANEL_PASSIVE   =self.ffi.cast("HANDLE", -2)
        self.info.Control(PANEL_ACTIVE, self.ffic.FCTL_REDRAWPANEL, 0, 0)
        self.info.Control(PANEL_PASSIVE, self.ffic.FCTL_REDRAWPANEL, 0, 0)

    def setbar(self, value):
        assert type(value) == int
        assert value <= self.maxvalue

        percent = value * 100 // self.maxvalue

        pv = self.ffi.new('struct PROGRESSVALUE *', (percent, 100))
        self.adv(self.ffic.ACTL_SETPROGRESSVALUE, pv)

        nb = int(percent*PROGRESS_WIDTH // 100)
        self.bar =  '\u2588'*nb + '\u2591'*(PROGRESS_WIDTH-nb)

    def update(self, value):
        self.setbar(value)
        self.show()

    def aborted(self):
        return CheckForEscape()

    def close(self):
        self.hide()
