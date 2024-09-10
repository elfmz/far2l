from yfar import FarPlugin

class Plugin(FarPlugin):
    label = "Jump Between Selected Files"
    openFrom = ["PLUGINSMENU", 'FILEPANEL']

    def OpenPlugin(self, _):
        panel = self.get_panel()
        option = self.menu(('Jump to &Previous Selected File',
                            'Jump to &Next Selected File'), self.label)
        if option == 0:  # move up
            for f in reversed(panel.selected):
                if f.index < panel.cursor:
                    panel.cursor = f.index
                    break
        elif option == 1:  # move down
            for f in panel.selected:
                if f.index > panel.cursor:
                    panel.cursor = f.index
                    break
