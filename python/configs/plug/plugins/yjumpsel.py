
"""
First of all this plugin is an example of how easy writing plugins
for FAR should be when having proper pythonic library.

yfar is a work-in-progress attempt to write such library.

This plugin allows to jump between selected files on the panel.
This was very helpful if you have thousands or even tens of thousands 
files in the directory, select some of them by mask and want to figure 
out which files were actually selected. For convenience bind
them to hotkeys likes Alt+Up / Alt+Down.
"""

__author__ = 'Yaroslav Yanovsky'

from yfar import FarPlugin


class Plugin(FarPlugin):
    label = 'Jump Between Selected Files'
    openFrom = ['PLUGINSMENU', 'FILEPANEL']

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
                