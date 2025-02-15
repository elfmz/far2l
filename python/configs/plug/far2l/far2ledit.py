class Edit:
    def __init__(self, plugin, info, ffi, ffic):
        self.plugin = plugin
        self.info = info
        self.ffi = ffi
        self.ffic = ffic

    def GetFileName(self):
        nb = self.info.EditorControl(self.ffic.ECTL_GETFILENAME, 0)
        if nb:
            data = self.ffi.new("wchar_t []", nb)
            self.info.EditorControl(self.ffic.ECTL_GETFILENAME, self.ffi.cast("LONG_PTR", data))
            return self.plugin.f2s(data)
        return None
