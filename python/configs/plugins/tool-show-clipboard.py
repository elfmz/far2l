import gi

gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, Gdk

clipboard = Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD)
for mime in (
    "text/plain",
    "text/plain;charset=utf-8",
    "text/html",
    "text/uri-list",
    "image/png",
    'x-special/gnome-copied-files',
    'SAVE_TARGETS',
    'TIMESTAMP',
    'TARGETS',
):
    text = clipboard.wait_for_contents(Gdk.Atom.intern(mime, True))
    if text:
        print(mime, text.get_data())
