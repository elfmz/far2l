class PluginBase:

    menu = None
    conf = None
    area = None

    def __init__(self, parent, info, ffi, ffic):
        self.parent = parent
        self.info = info
        self.ffi = ffi
        self.ffic = ffic

    def s2f(self, s):
        return self.ffi.new("wchar_t []", s)

    def f2s(self, s):
        return self.ffi.string(self.ffi.cast("wchar_t *", s))

    def HandleCommandLine(self, line):
        pass
