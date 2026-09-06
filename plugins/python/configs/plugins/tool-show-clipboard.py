import gi

gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, Gdk

def show_clipboard():
    clipboard = Gdk.Display().get_default().get_clipboard()
    print("Clipboard:")
    print('str:', clipboard.get_formats().to_string())
#show_clipboard()

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
