class Editor:
    def __init__(self, plugin):
        self.info = plugin.info
        self.ffic = plugin.ffic
        self.ffi = plugin.ffi
        self.s2f = plugin.s2f
        self.f2s = plugin.f2s

    def GetFileName(self):
        nb = self.info.EditorControl(self.ffic.ECTL_GETFILENAME, self.ffi.NULL)
        if nb:
            data = self.ffi.new("wchar_t []", nb)
            self.info.EditorControl(self.ffic.ECTL_GETFILENAME, self.ffi.cast("PVOID", data))
            return self.f2s(data)
        return None
