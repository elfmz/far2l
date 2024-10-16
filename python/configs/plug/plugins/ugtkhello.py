from far2l.plugin import PluginBase

import gi

gi.require_version("Gtk", "3.0")
from gi.repository import Gtk


import logging
log = logging.getLogger(__name__)

class MyWindow(Gtk.Window):
    def __init__(self):
        super().__init__(title="Hello World")

        self.button = Gtk.Button(label="Click Here")
        self.button.connect("clicked", self.on_button_clicked)
        self.add(self.button)

        self.connect("destroy", self.onQuit)
        self.show_all()

    def on_button_clicked(self, widget):
        log.debug("Hello World")

    def onQuit(self, window):
        Gtk.main_quit()

class Plugin(PluginBase):
    label = "Python GTK"
    openFrom = ["PLUGINSMENU", "EDITOR"]

    def OpenPlugin(self, OpenFrom):
        if OpenFrom == 5:
            # EDITOR
            win = MyWindow()
            Gtk.main()
        return -1
