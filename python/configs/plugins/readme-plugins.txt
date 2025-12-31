tool-show-clipboard.py
    A tool that allows you to display the contents of the Gnome clipboard - this is not a plugin

Below is a list of plugins and a brief description of their functionality:

uadb.py
    A plugin that supports ADB (Android Debug Bridge). It allows you to
    display directories from a device that supports the ADB protocol
    (Android Debug Bridge) to support copying (F5) files (to/from),
    deleting (F8) files/directories, and creating (F7) directories.

    The plugin is loaded with the command: py:load uadb

    After loading the plugin and pressing Alt+F1/F2,
    select "Python ADB" from the list of plugins.

    To configure the plugin (after loading) in the command line, execute:
        py:uadb config
    or press Alt+Shift+F9 and select "Python ADB" from the list.
    The selection in the shell field determines which shell and which regexp
    expression embedded in the program will be used to analyze the ls command results.

ucharmapold.py
    Old version of ucharmap plugin.

ucharmap.py
    Plugin that displays the entire UTF-8 character set.
    Activation: F11 + "Python Character Map" - display its dialog
    Offset - position in the character set
    Goto - change position - decimal or hexadecimal number or 1 character is allowed,
        valid range is <0x0000-0xffff>
    Keys:
        Home = goto first visible offset
        PgUp = prevoius page
        PgDn = next page
        Left = prevoius character in row
        Right = next character in row
        Up = prevoius row
        Down = next row
        ENTER/OK = copy character at affset into clipboard
    Mouse:
        Click - select character at pointer position

uclipset.py
    Plugin that copies selected files from the current panel to the clipboard
    in a format understood by Gnome library programs (eg.: Nautilus file manager)
    Activation: F11 + "Python Clip SET"

uclipget.py
    Plugin that copies selected files from the clipboard to the current FAR2L
    panel in a format understood by Gnome library programs.

udebug.py
    Here is a more detailed description: read-udebug-en.txt, read-udebug-pl.txt

udialog.py
    Simple dialog to edit one field and run python commands inside the plugin.
    - run dialog by executing in shell window: py:dialog
    - run signe python command by executing py:exec ...

udocker.py uses udockerio.py
    The plugin displays available Docker containers and allows you to manage
    files in the selected container without having to export/import it.
    It also allows you to start and stop the container. If the Python runlike
    package is installed, it allows you to view the container instance's startup
    parameters.

uedindent.py
    Indent/Dedent selected block in the FAR2l editor.
    A tutorial showing how to use ProcessEditorInput:
        ALT|CTRL + TAB=indent
        ALT|CTRL+SHIFT+TAB=indent

uedreplace.py
    Example of a search/replace dialog with partial functionality.

uedsort.py
    The prototype of the standard esdort plugin.

ueduniq.py
    The plugin allows you to remove duplicate rows in the selected block.

ugtkhello.py
    The plugin shows how to open a window from the gtk library.

uhexedit.py
    The prototype of the standard hexitor plugin.

uminer.py
    FAR2L also has its own game.

upanel.py
    The first plugin ever - something like the tmppanel plugin

uprogressDialog.py
    Example of a dialogue with a function running in the background.

uprogressMessage.py
    Example of a plugin running in the foreground but interruptible -
    another form of progress dialog.

usizer.py
    An example of a dialogue using almost all possible controls.

usizerflow.py
    Another sizer - flowsizer in which you can determine how many
    elements can be in one row without the need to divide them
    into vertical and horizontal boxes

usqlite.py
    A plugin demonstrating how to open and display the contents of
    an SQLite database. F4 = edit the highlighted row,
    F6/F7 = jump to the next row group (offset in the table).
    Activation occurs after CTRL+PGDN on the highlighted file in the
    standard file list. FAR2L calls the OpenFilePlugin method of
    active plugins, and the first one to respond becomes active.

utranslate.py
    The plugin allows you to translate and check text for correctness
    using Google Translate.
        1. py:spell [--lang=text source language(en)] [--text="some text"]
            result in /tmp/far2l-py.log
        2. py:translate [--from=source language(en)] [--to=destination language(en)] [--text="text to translate"]

    If the plugin is opened from the editor, it will display a dialog,
    otherwise, it will display information on how to launch it.

uuuidgen.py
    The plugin generates a random GUID and places the result in the clipboard.

uvtlog.py
    The plugin copies the contents of the screen available after Ctrl+O
    in the file manager to the editor.

ybatchrename.py
    Look inside the file, at the beginning there is a short introduction to how it works.

yfar.py
    Look inside the file, at the beginning there is a short introduction to how it works.

yjumpsel.py
    Look inside the file, at the beginning there is a short introduction to how it works.

yjumpword.py
    Look inside the file, at the beginning there is a short introduction to how it works.
