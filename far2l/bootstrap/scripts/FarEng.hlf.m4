m4_include(`farversion.m4')m4_dnl
.Language=English,English
.Options CtrlColorChar=\
.Options CtrlStartPosChar=^<wrap>

@Contents
$^#File and archive manager#
`$^#'FULLVERSIONNOBRACES`#'
`$^#©1996-2000 Eugene Roshal, ©2000-2016 FAR Group,' ©COPYRIGHTYEARS `FAR People'#
   ~FAR2L features - Getting Started~@Far2lGettingStarted@

   ~Help file index~@Index@
   ~How to use help~@Help@

   ~About FAR2L~@About@
   ~License~@License@

   ~UI backends~@UIBackends@
   ~Command line switches~@CmdLine@
   ~Keyboard reference~@KeyRef@
   ~Plugins support~@Plugins@
   ~Overview of plugin capabilities~@PluginsReviews@
   ~Terminal~@Terminal@

   ~Panels:~@Panels@  ~File panel~@FilePanel@
            ~Tree panel~@TreePanel@
            ~Info panel~@InfoPanel@
            ~Quick view panel~@QViewPanel@
            ~Drag and drop files~@DragAndDrop@
            ~Customizing file panel view modes~@PanelViewModes@
            ~Selecting files~@SelectFiles@

   ~Menus:~@Menus@   ~Left and right menus~@LeftRightMenu@
            ~Files menu~@FilesMenu@
            ~Commands menu~@CmdMenu@
            ~Options menu~@OptMenu@

   ~Find file~@FindFile@
   ~History~@History@
   ~Find folder~@FindFolder@
   ~Compare folders~@CompFolders@
   ~User menu~@UserMenu@
   ~Location menu~@DriveDlg@

   ~File associations~@FileAssoc@
   ~Operating system commands~@OSCommands@
   ~Bookmarks~@Bookmarks@
   ~Filters menu~@FiltersMenu@
   ~Screens switching~@ScrSwitch@
   ~Task list~@TaskList@
   ~Hotplug devices list~@HotPlugList@

   ~System settings~@SystemSettings@
   ~Panel settings~@PanelSettings@
   ~Interface settings~@InterfSettings@
   ~Input settings~@InputSettings@
   ~Dialog settings~@DialogSettings@
   ~Menu settings~@VMenuSettings@
   ~Command line settings~@CmdlineSettings@
   ~Locations menu settings~@ChangeLocationConfig@

   ~Files highlighting and sort groups~@Highlight@
   ~File descriptions~@FileDiz@
   ~Viewer settings~@ViewerSettings@
   ~Editor settings~@EditorSettings@

   ~Copying, moving, renaming and creating links~@CopyFiles@

   ~Internal viewer~@Viewer@
   ~Internal editor~@Editor@

   ~File masks~@FileMasks@
   ~Keyboard macro commands (macro command)~@KeyMacro@


@Help
$ # FAR2L: how to use help#
    Help screens may have reference items on them that lead to another help
screen. Also, the main page has the "~Help Index~@Index@", which lists all the
topics available in the help file and in some cases helps to find the needed
information faster.

    You may use #Tab# and #Shift-Tab# keys to move the cursor from one
reference item to another, then press #Enter# to go to a help screen describing
that item. With the mouse, you may click a reference to go to the help screen
about that item.

    If text does not completely fit in the help window, a scroll bar is
displayed. In such case #cursor keys# can be used to scroll text.

    You may press #Alt-F1# or #BS# to go back to a previous help screen and
#Shift-F1# to view the help contents.

    Press #F7# to search in Help (will show help topics containing the searched text fragment).

    Press #Shift-F2# for ~plugins~@Plugins@ help.

    #Help# is shown by default in a reduced windows, you can maximize it by
pressing #F5# "#Zoom#", pressing #F5# again will restore the window to the
previous size.

   #Ctrl-Alt-Shift#   Temporarily hide help window (as long as these keys are held down).

@About
$ # FAR2L: about#
    #FAR2L# is a text mode file and archive manager that provides a wide
set of file and folder operations.

    #FAR2L# is #freeware# and #open source# software distributed under the
GNU GPL v2 ~license~@License@.

    #FAR2L# does transparent #archive# processing. Files in the archive are
handled similarly as in a folder: when you operate with the archive, FAR2L
transforms your commands into the corresponding external archiver calls.

    #FAR2L# offers a number of service functions as well.

  - FAR2L official site
    ~http://github.com/elfmz/far2l~@http://github.com/elfmz/far2l@
  - Original FAR Manager official site
    ~http://www.farmanager.com~@http://www.farmanager.com@

  ~FAR2L features - Getting Started~@Far2lGettingStarted@

@Far2lGettingStarted
$ # FAR2L features - Getting Started#
    FAR2L is Linux port FAR Manager v2 (see ~About FAR2L~@About@)
    FAR2L official site: ~https://github.com/elfmz/far2l~@https://github.com/elfmz/far2l@


 #UI Backends#
    FAR2L has 3 base UI Backends (see details in ~UI backends~@UIBackends@):
        - #GUI#: uses wxWidgets, works in graphics mode, ideal UX, requires a lot of X11 dependencies;
        - #TTY|Xi#: works in terminal mode, requires a dependency on pair X11 libraries
(to access clipboard and to get state of all keyboard modifiers), almost perfect UX;
        - #TTY|X#: works in terminal mode, uses X11 to access clipboard, all keyboard works via terminal;
        - #TTY#: plain terminal mode, no X11 dependencies, UX with some restrictions
(works fully when running in the terminal emulators,
which provide clipboard access and has their advanced keyboard-protocols, see list below).
    You can see FAR2L version and currently used backend in window title or by ~pseudo-command~@SpecCmd@ #far:about#.
    #TTY|Xi# does not work under Wayland due to Wayland's security restriction,
when it starts, far2l switches to #TTY|X# without i.
    Far2l running and selecting backend:
        - if you have FAR2L-GUI installed, then when you run FAR2L it will try to use GUI mode;
        - to force run in terminal mode TTY|Xi use in command line: #far2l --tty#;
        - to force run in terminal mode TTY|X use in command line: #far2l --tty --nodetect=xi --ee#;
        - to force run in plain mode TTY use in command line: #far2l --tty --nodetect=x --ee#;
        - run FAR2L-GUI from command line in background without blocking terminal: #far2l --notty &#
    (see details in ~Command line switches~@CmdLine@ or #far2l --help#).


 #Keyboard shortcuts are exclusively captured by desktop environment and terminals#
    Some keyboard shortcuts #Alt-F1#, #Alt-F2#, #Alt-F7#, #Ctrl-arrows# etc. are exclusively used in desktop environment GNOME, KDE, Xfce, macOS etc. To work with these keys in FAR2L, you need to release keyboard shortcuts in the environment settings.
    Terminal emulators also do not often pass some of the key combinations to applications, or do not distinguish pressing various combinations of modifiers (#Ctrl#, #Alt# etc.).
    Also you can use FAR2L lifehacks:
        - ~Sticky controls~@MiscCmd@ via #Ctrl-Space# or #Alt-Space#;
        - Exclusively handle hotkeys option in the ~Input settings~@InputSettings@ (only in GUI backend mode).


 #macOS workaround# if far2l in macOS regularly asks permission to folders
    After command #sudo codesign --force --deep --sign - /Applications/far2l.app# it is enough to confirm permission only once.


 #Changing font for FAR2L-GUI#
    - Menu(#F9#)->Options->Interface settings->[ Change font ]


 #Pasting feature in terminals#
    The keyboard shortcut of the #terminal pasting# (terminal simulates keyboard input) and #FAR2L pasting# (FAR2L itself does paste) are different. Note that pasting keyboard shortcut in different terminals is various (and may overlap the standard FAR2L's pasting #Shift-Ins# or #Ctrl-V#).
    In FAR2L without TTY|X (and without enabled OSC 52 both in FAR2L and in terminal) FAR2L's pasting uses its #internal clipboard# (because FAR2L does not access the system clipboard), terminal pasting uses #system clipboard#.


 #FAR2L command line shell & bash#
    FAR2L internal command line work fully only via #bash#.
    You can change shell by Menu(#F9#)->Options->~Command line settings~@CmdlineSettings@->#Use shell# but command line will work with significant restrictions/bugs especially with native shell commands.
    If you system has not #bash# recommend installing it and using only bash in FAR2L.
    If your system's default shell is not bash, you may be convenient to set your environments variables, aliases etc. in bash startup files also.


 #Access to remote FAR2L#
    When the session is terminated, remote FAR2L does not die, but it remains to wait for reconnection (the behavior changed by ~command line switches~@CmdLine@ #--immortal# and #--mortal#), and the next time FAR2L runs, it will find the previous instance and try to reconnect.
    To transfer extended keyboard shortcuts and the clipboard to the remote FAR2L, you must initiate the connection from clients that can do this (see list below).


 #Special options for configuring FAR2L running in terminal emulators#
    - Menu(#F9#)->Options->Interface settings->#Use OSC52 to set clipboard data#
(shown in the dialog only if FAR2L run in TTY/TTY|X mode and all other options for clipboard access are unavailable).
You can run #far2l --tty --nodetect# to force not use others clipboard options.
    - Menu(#F9#)->Options->Interface settings->#Override base colors palette#
(shown in the dialog only if FAR2L run in TTY/TTY|X mode) allows far2l to adjust terminal palette colors.
If your terminal doesn't support OSC4 sequence you may turn it off to avoid show artifacts sequence in terminal after exit from far2l.


 #Full-functional work with the system clipboard in a plain terminal version FAR2L TTY#
    To interact with the system clipboard you must not forget to enable #OSC 52# in both the #FAR2L settings#
(see details above),
and in #terminal settings# option #OSC 52 must be allowed#
(by default, OSC 52 is disabled in some terminals for security reasons; OSC 52 in many terminals is implemented only for the copy mode, and paste from the terminal goes by bracketed paste mode).


 #Terminals and ssh-clients supporting extended FAR2L keyboard shortcuts for plain terminal version FAR2L TTY#
    - Internal terminal in FAR2L-GUI (Linux, macOS, *BSD),
see ~UI backends~@UIBackends@ and in help of #NetRocks plugin# section #Command line and remote FAR2L#
(~TTY|F backend~@UIBackends@: keys and clipboard by FAR2L TTY extensions support)

    - kovidgoyal's kitty (Linux, macOS, *BSD): ~https://github.com/kovidgoyal/kitty~@https://github.com/kovidgoyal/kitty@ & ~https://sw.kovidgoyal.net/kitty~@https://sw.kovidgoyal.net/kitty@
(~TTY|k backend~@UIBackends@: keys by kovidgoyal's kitty keyboard protocol;
for clipboard need turn on OSC 52)

    - Wez's Terminal Emulator (Linux, FreeBSD, Windows): ~https://github.com/wez/wezterm~@https://github.com/wez/wezterm@ & ~https://wezfurlong.org/wezterm~@https://wezfurlong.org/wezterm@
(~TTY|k backend~@UIBackends@: keys in Linux, FreeBSD by kovidgoyal's kitty keyboard protocol;
~TTY|w backend~@UIBackends@: keys in Windows by win32-input-mode which enable by default;
for clipboard need turn on OSC 52)
[in macOS & in Windows in wezterm the kitty keyboard protocol support not working]

    - iTerm2 (macOS): ~https://gitlab.com/gnachman/iterm2~@https://gitlab.com/gnachman/iterm2@ & ~https://iterm2.com~@https://iterm2.com@
(~TTY|a backend~@UIBackends@: keys by iTerm2 "raw keyboard" protocol;
for clipboard need turn on OSC 52)

    - Windows Terminal
(~TTY|w backend~@UIBackends@: keys by win32-input-mode; for clipboard need turn on OSC 52; has mouse bug: ~https://github.com/microsoft/terminal/issues/15083~@https://github.com/microsoft/terminal/issues/15083@)

    - putty4far2l (Windows ssh-client): ~https://github.com/ivanshatsky/putty4far2l/releases~@https://github.com/ivanshatsky/putty4far2l/releases@ & ~https://github.com/unxed/putty4far2l~@https://github.com/unxed/putty4far2l@
(~TTY|F backend~@UIBackends@: keys and clipboard by FAR2L TTY extensions support)

    - cyd01's KiTTY (Windows ssh-client): ~https://github.com/cyd01/KiTTY~@https://github.com/cyd01/KiTTY@ & ~https://www.9bis.net/kitty~@https://www.9bis.net/kitty@
(~TTY|F backend~@UIBackends@: keys and clipboard by FAR2L TTY extensions support)

    - putty-nd (Windows ssh-client): ~https://sourceforge.net/projects/putty-nd~@https://sourceforge.net/projects/putty-nd@ & ~https://github.com/noodle1983/putty-nd~@https://github.com/noodle1983/putty-nd@
(~TTY|F backend~@UIBackends@: keys and clipboard by FAR2L TTY extensions support)


 #Location of FAR2L settings and history#
    - FAR2L by default works with settings located in #~~/.config/far2l/# or in #$XDG_CONFIG_HOME/far2l/#
    - command line switch #-u# (or #$FARSETTINGS# environment variable) allows to specify arbitrary settings location:
        #-u <path>#: in #path/.config/# (if path or $FARSETTINGS is full path)
        #-u <identity>#: in #~~/.config/far2l/custom/identity/# or in #$XDG_CONFIG_HOME/far2l/custom/identity/#
    - some settings files (may be missing):
        - #settings/config.ini# - general config
        - #settings/colors.ini# - ~files highlighting and sort groups~@Highlight@
        - #settings/key_macros.ini# - ~keyboard macro commands~@KeyMacro@
        - #settings/user_menu.ini# - main ~user menu~@UserMenu@ (the format is different from local user FarMenu.ini)
        - #settings/associations.ini# - ~file associations~@FileAssoc@
        - #settings/bookmarks.ini# - ~bookmarks~@Bookmarks@ to fast access to frequently used directories by RCtrl-0...9 or Ctrl-Alt-0...9
        - #favorites# - additional items in ~location menu~@DriveDlg@ by Alt-F1/F2
        - #cp# - forced setting of OEM and ANSI encodings (see ~ANSI and OEM codepage setting~@CodePagesSet@)
        - #plugins# - plugins
            - #plugins/state.ini# - plugins cache
            - #plugins/NetRocks/sites.cfg# - NetRocks sites
            - #plugins/multiarc/custom.ini# - customization by extend command line archivers
        - #clipboard# - bash-script (must be chmod +x) for workaround to access to clipboard if other FAR2L tools do not work


    See also:
    ~About FAR2L~@About@
    ~License~@License@

    ~Help file index~@Index@
    ~How to use help~@Help@

    ~UI backends~@UIBackends@
    ~Command line switches~@CmdLine@ or #far2l --help#
    ~ANSI and OEM codepage setting~@CodePagesSet@

@License
$ # FAR2L: License#
GNU GPL v2 and some source files originally licensed under 3-clause BSD license
See LICENSE.txt and LICENSE.Far2.txt in sources tree for details

@CmdLine
$ # FAR2L: command line switches#
  Actual list see via #far2l -h# or #far2l --help#.

  You can specify following switches in the command line.


 #FAR2L backend-specific options:#
  #--tty#
  Runs far2l with ~TTY backend~@UIBackends@ instead of autodetecting GUI/TTY mode. While GUI
backed is preferred from user experience point of view, sometimes it may be necessary
to avoid attempt to use GUI mode.

  #--notty#
  Don't fallback to TTY backend if ~GUI backend~@UIBackends@ was failed to initialize.

  #--nodetect#
  By default far2l tries to detect if it runs inside of terminal of another far2l and in such case
it uses TTY backend with far2l extensions. In case of far2l extensions unavailable far2l checks for
availability of X11 session and uses it to improve user experience if compiled with TTYX/TTYXI option.
This switch disables all this functionality, forcing plain terminal mode for TTY backend.

  #--mortal#
  This argument applies only to far2l that runs with TTY backend. By default when terminal closed
TTY-based far2l continues to work 'in background' and next start of another TTY-backend far2l will
suggest to activate such 'backgrounded' far2l instance instead of spawning new. This command line
switch completely disables this functionality, so closing terminal will close TTY-backed far2 that
runs inside.

  #--primary-selection#
  Use PRIMARY selection instead of CLIPBOARD X11 selection. This argument applies only to far2l
that runs with WX backend.

  Backend-specific options also can be set via the #FAR2L_ARGS# environment variable
(for example: #export FAR2L_ARGS="--tty --nodetect --ee"# and then simple #far2l# to force start only TTY backend).


 #FAR2L command-line options:#
  #-a#
  Disable display of characters with codes 0 - 31 and 255. May be useful when
executing FAR2L under telnet.

  #-ag#
  Disable display of pseudographics with codes > 127.

  #-an#
  Disable display of pseudographics characters completely.

  #-e[<line>[:<pos>]] <filename>#
  Edit the specified file. After -e you may optionally specify editor start line
and line position.
  For example: #far2l -e70:2 readme#.

  #-p[<path>]# [Unsupported in far2l]
  Search for "main" plugins in the folder given in <path>.
  Several search paths may be given separated by ';'.

  #-co#
  Forces FAR2L to load plugins from cache only. Plugins are loaded faster this way,
but new or changed plugins are not discovered. Should be used ONLY with a stable
list of plugins. After adding, replacing or deleting a plugin FAR2L should be loaded
without this switch. If the cache is empty, no plugins will be loaded.

  Remarks about switches -p and -co:

  - ^<wrap>if -p is empty, then FAR2L will be loaded with no plugins;
  - ^<wrap>if -p is given with a <path>, then only plugins from <path> will be loaded;
  - ^<wrap>if only the -co switch is given and plugins cache is not empty, then plugins
will be loaded from cache;
  - ^<wrap>-co is ignored, if -p is given;
  - ^<wrap>if -p and -co are not given, then plugins will be loaded from the main folder,
and from the path given at the "~Path for personal plugins~@PluginsManagerSettings@" parameter.

  #-m#
  FAR2L will not load macros from config file when started.

  #-ma#
  Macros with the "Run after FAR2L start" option set will not be run when FAR2L is started.

  #-u <identity># or #-u <path>#
  Allows to specify separate settings identity or FS location.
  #-u <path>#: in path/.config/ (if path is full path)
  #-u <identity>#: in ~~/.config/far2l/custom/identity/ or in $XDG_CONFIG_HOME/far2l/custom/identity/

  #-v <filename>#
  View the specified file.
  #-v - <command line>#
  Executes given command line and opens viewer with its output.
  For example, #far2l -v - ls# will view ls command output.

  #-w# [Unsupported in far2l]
  Stretch to console window instead of console buffer.

  #-x# [Unsupported in far2l]
  Disable exception handling. This option has been designed for plugin developers,
and it is not recommended to specify it during normal operation.

  #-set:<parameter>=<value>#
  Override the configuration parameter, see ~far:config~@FarConfig@ for details.
  Example: far2l -set:Language.Main=English -set:Screen.Clock=0 -set:XLat.Flags=0x10001 -set:System.FindFolders=false


  It is possible to specify at most two paths (to folders, files or archives) or
two commands with plugin prefix in the command line. The first path applies to the
active panel, the second path - to the passive one:
  - ^<wrap>if a folder or archive is specified, FAR2L will show its contents;
  - ^<wrap>if a file is specified, FAR2L will change to the folder where it
resides and place the cursor on the file, if it exists;
  - ^<wrap>when prefixes specified (simultaneous use with common paths allowed)
passive command executes first (passive panel activates temporary). Односимвольные префиксы игнорируются.
  Example: far ma:Far20.7z "macro:post MsgBox(\\"FAR2L\\",\\"Successfully started\\")"


@KeyRef
$ #Keyboard reference#

 ~Panel control~@PanelCmd@

 ~Command line~@CmdLineCmd@

 ~File management and service commands~@FuncCmd@

 ~Mouse: wheel support~@MsWheel@

 ~Menu control commands~@MenuCmd@

 ~Miscellaneous~@MiscCmd@

 ~Special commands~@SpecCmd@

@MenuCmd
$ #Menu control commands#
 #Common menu and drop-down list commands#

  Filter menu or list items                               #Ctrl-Alt-F#
   (shows only items containing the typing text)

  Lock filter                                             #Ctrl+Alt+L#

  Horizontally scroll long item  #Alt-Left#,#Alt-Right#,#Alt-Home#,#Alt-End#
   (work only with non-numpad)

  See also the list of ~macro keys~@KeyMacroMenuList@, available in the menus.

@PanelCmd
$ #Panel control commands  #
    #Common panel commands#

  Change active panel                                            #Tab#
  Swap panels                                                 #Ctrl-U#
  Re-read panel                                               #Ctrl-R#
  Toggle info panel                                           #Ctrl-L#
  Toggle ~quick view panel~@QViewPanel@                                     #Ctrl-Q#
  Toggle tree panel                                           #Ctrl-T#
  Hide/show both panels                                       #Ctrl-O#
  Temporarily hide both panels                        #Ctrl-Alt-Shift#
    (as long as these keys are held down)
  Hide/show inactive panel                                    #Ctrl-P#
  Hide/show left panel                                       #Ctrl-F1#
  Hide/show right panel                                      #Ctrl-F2#
  Change panels height                             #Ctrl-Up,Ctrl-Down#
  Change current panel height          #Ctrl-Shift-Up,Ctrl-Shift-Down#
  Change panels width                           #Ctrl-Left,Ctrl-Right#
    (when the command line is empty)
  Restore default panels width                          #Ctrl-Numpad5#
  Restore default panels height                     #Ctrl-Alt-Numpad5#
  Show/Hide functional key bar at the bottom line.            #Ctrl-B#

    #File panel commands#

  Select/deselect file                        #Ins, Shift-Cursor keys#
  Select group                                                #Gray +#
  Deselect group                                              #Gray -#
  Invert selection                                            #Gray *#
  Select files with the same extension as the          #Ctrl-<Gray +>#
    current file
  Deselect files with the same extension as the        #Ctrl-<Gray ->#
    current file
  Invert selection including folders                   #Ctrl-<Gray *>#
    (ignore command line state)
  Select files with the same name as the current file   #Alt-<Gray +>#
  Deselect files with the same name as the current      #Alt-<Gray ->#
    file
  Select all files                                    #Shift-<Gray +>#
  Deselect all files                                  #Shift-<Gray ->#
  Restore previous selection                                  #Ctrl-M#

  Scroll long names and descriptions              #Alt-Left,Alt-Right#
                                                    #Alt-Home,Alt-End#

  Set brief view mode                                     #LeftCtrl-1#
  Set medium view mode                                    #LeftCtrl-2#
  Set full view mode                                      #LeftCtrl-3#
  Set wide view mode                                      #LeftCtrl-4#
  Set detailed view mode                                  #LeftCtrl-5#
  Set descriptions view mode                              #LeftCtrl-6#
  Set long descriptions view mode                         #LeftCtrl-7#
  Set file owners view mode                               #LeftCtrl-8#
  Set file links view mode                                #LeftCtrl-9#
  Set alternative full view mode                          #LeftCtrl-0#

  Toggle hidden and system files displaying                   #Ctrl-H#
  Toggle long/short file names view mode                      #Ctrl-N#
  Toggle hide/show/align file name ~highlighting markers~@Highlight@   #Ctrl-Alt-M#

  Hide/Show left panel                                       #Ctrl-F1#
  Hide/Show right panel                                      #Ctrl-F2#

  Sort files in the active panel by name                     #Ctrl-F3#
  Sort files in the active panel by extension                #Ctrl-F4#
  Sort files in the active panel by last write time          #Ctrl-F5#
  Sort files in the active panel by size                     #Ctrl-F6#
  Keep files in the active panel unsorted                    #Ctrl-F7#
  Sort files in the active panel by creation time            #Ctrl-F8#
  Sort files in the active panel by access time              #Ctrl-F9#
  Sort files in the active panel by description             #Ctrl-F10#
  Sort files in the active panel by file owner              #Ctrl-F11#
  Display ~sort modes~@PanelCmdSort@ menu                                   #Ctrl-F12#
  Use group sorting                                        #Shift-F11#
  Show selected files first                                #Shift-F12#

  Create a ~folder shortcut~@Bookmarks@              #Ctrl-Shift-0# to #Ctrl-Shift-9#
  Jump to a folder shortcut               #RightCtrl-0# to #RightCtrl-9#

      If the active panel is a ~quick view panel~@QViewPanel@, a ~tree panel~@TreePanel@ or
    an ~information panel~@InfoPanel@, the directory is changed not on the
    active, but on the passive panel.

  Copy the names of selected files to the clipboard         #Ctrl-Ins#
   (if the command line is empty)
  Copy the files to clipboard                                 #Ctrl-C#
   (ignore command line state)
  Copy the names of selected files to the clipboard   #Ctrl-Shift-Ins#
   (ignore command line state)
  Copy full names of selected files to the clipboard   #Alt-Shift-Ins#
   (ignore command line state)
  Copy network (UNC) names of selected files to the     #Ctrl-Alt-Ins#
   clipboard (ignore command line state)

  See also the list of ~macro keys~@KeyMacroShellList@, available in the panels.

  Notes:

  1. ^<wrap>If "Allow reverse sort modes" option in ~Panel settings~@PanelSettings@
dialog is enabled, pressing the same sort key second time toggles the sort direction
from ascending to descending and vice versa;

  2. ^<wrap>If #Alt-Left# and #Alt-Right# combinations, used to scroll long names
and descriptions, work only with non-numpad #Left# and #Right# keys. This is due to
the fact that when #Alt# is pressed, numpad cursor keys are used to enter characters
via their decimal codes.

  3. ^<wrap>The key combination #Ctrl-Alt-Ins# puts the following text into the clipboard:
       ^<wrap>* for network drives - the network (UNC) name of the file object;
       ^<wrap>* for local drives - the local name of the file taking into account
~symbolic links~@HardSymLink@.

  4. ^<wrap>If #Ctrl-Ins#, #Alt-Shift-Ins# or #Ctrl-Alt-Ins# is pressed when the cursor
is on the file "#..#", the name of the current folder is copied.


@PanelCmdSort
$ #Sort modes#
    The sort modes menu is called by #Ctrl-F12# and applies to the currently
active panel. The following sort modes are available:

  Sort files by name                                         #Ctrl-F3#
  Sort files by extension                                    #Ctrl-F4#
  Sort files by last write time                              #Ctrl-F5#
  Sort files by size                                         #Ctrl-F6#
  Keep files unsorted                                        #Ctrl-F7#
  Sort files by creation time                                #Ctrl-F8#
  Sort files by access time                                  #Ctrl-F9#
  Sort files by description                                 #Ctrl-F10#
  Sort files by file owner                                  #Ctrl-F11#

  Клавиша #+# устанавливает прямую сортировку.
  Клавиша #-# устанавливает обратную сортировку.
  Клавиша #*# меняет сортировку на обратную.

  Use group sorting                                        #Shift-F11#
  Show selected files first                                #Shift-F12#
  Use numeric sort
  Use case sensitive sort.

  #Remarks on the numeric sort#

    The sorting algorithm which is used by the operating system to sort file
lists was changed in Windows XP. A numeric, not a string sort is used. FAR2L also
allows to use a numeric sort as in Windows XP - leading zeros in a file name
are ignored. The following example shows how the files are sorted:

    Numeric sort (Windows XP)    String sort (Windows 2000)

    Ie4_01                       Ie4_01
    Ie4_128                      Ie4_128
    Ie5                          Ie401sp2
    Ie6                          Ie5
    Ie401sp2                     Ie501sp2
    Ie501sp2                     Ie6
    5.txt                        11.txt
    11.txt                       5.txt
    88.txt                       88.txt

    See also: common ~menu~@MenuCmd@ keyboard commands.

@FastFind
$ #Fast find in panels#
    To locate a file quickly, you can use the #fast find# operation and enter
the starting characters of the file name. In order to use that, hold down the
#Alt# (or #Alt-Shift#) keys and start typing the name of the needed file, until
the cursor is positioned to it.

    By pressing #Ctrl-Enter#, you can cycle through the files matching the part
of the filename that you have already entered. #Ctrl-Shift-Enter# allows to
cycle backwards.

    Besides the filename characters, you can also use the wildcard characters
'*' and '?'.

    Insertion of text, pasted from clipboard (#Ctrl-V# or #Shift-Ins#), to the
fast find dialog will continue as long as there is a match found.

    It is possible to use the transliteration function while entering text in
the search field. If used the entered text will be transliterated and a new
match corresponding to the new text will be searched. See TechInfo##10 on how
to set the hotkey for the transliteration.

   See also the list of ~macro keys~@KeyMacroSearchList@, available in fast find.

@CmdLineCmd
$ #Command line commands#
 #Common command line commands#

  Character left                                         #Left,Ctrl-S#
  Character right                                       #Right,Ctrl-D#
  Word left                                                #Ctrl-Left#
  Word right                                              #Ctrl-Right#
  Start of line                                            #Ctrl-Home#
  End of line                                               #Ctrl-End#
  Delete char                                                    #Del#
  Delete char left                                                #BS#
  Delete to end of line                                       #Ctrl-K#
  Delete word left                                           #Ctrl-BS#
  Delete word right                                         #Ctrl-Del#
  Copy to clipboard                                         #Ctrl-Ins#
  Paste from clipboard                                     #Shift-Ins#
  Previous command                                            #Ctrl-E#
  Next command                                                #Ctrl-X#
  Clear command line                                          #Ctrl-Y#

 #Insertion commands#

  Insert current file name from the active panel   #Ctrl-J,Ctrl-Enter#

     In the ~fast find~@FastFind@ mode, #Ctrl-Enter# does not insert a
     file name, but instead cycles the files matching the
     file mask entered in the fast find box.

  Insert current file name from the passive panel   #Ctrl-Shift-Enter#
  Insert full file name from the active panel                 #Ctrl-F#
  Insert full file name from the passive panel                #Ctrl-;#
  Insert network (UNC) file name from the active panel    #Ctrl-Alt-F#
  Insert network (UNC) file name from the passive panel   #Ctrl-Alt-;#

  Insert path from the left panel                             #Ctrl-[#
  Insert path from the right panel                            #Ctrl-]#
  Insert network (UNC) path from the left panel           #Ctrl-Alt-[#
  Insert network (UNC) path from the right panel          #Ctrl-Alt-]#

  Insert path from the active panel                     #Ctrl-Shift-[#
  Insert path from the passive panel                    #Ctrl-Shift-]#
  Insert network (UNC) path from the active panel        #Alt-Shift-[#
  Insert network (UNC) path from the passive panel       #Alt-Shift-]#

  Notes:

  1. ^<wrap>If the command line is empty, #Ctrl-Ins# copies selected file names
from a panel to the clipboard like #Ctrl-Shift-Ins# (see ~Panel control commands~@PanelCmd@);

  2. ^<wrap>#Ctrl-End# pressed at the end of the command line, replaces its current contents
with a command from ~history~@History@ beginning with the characters that are in the command line,
if such a command exists. You may press #Ctrl-End# again to go to the next such command.

  3. ^<wrap>Most of the described above commands are valid for all edit strings including edit
controls in dialogs and internal editor.

  4. ^<wrap>#Alt-Shift-Left#, #Alt-Shift-Right#, #Alt-Shift-Home# and #Alt-Shift-End# select
the block in the command line also when the panels are on.

  5. ^<wrap>For local drives, the commands to insert the network (UNC) name of a file object
insert the local name of the file with ~symbolic links~@HardSymLink@ expanded.

  6. ^<wrap>About hotkeys and other tricks of built-in terminal emulator: ~read here~@Terminal@

    See also ~Special commands~@SpecCmd@.

@FuncCmd
$ #Panel control commands - service commands#
  Online help                                                     #F1#

  Show ~user menu~@UserMenu@                                                  #F2#

  View                                   #Ctrl-Shift-F3, Numpad 5, F3#

    If pressed on a file, #Numpad 5# and #F3# invoke ~internal~@Viewer@,
external or ~associated~@FileAssoc@ viewer, depending upon the file type and
~external viewer settings~@ViewerSettings@.
    #Ctrl-Shift-F3# always calls internal viewer ignoring file associations.
    If pressed on a folder, calculates and shows the size of selected folders.

  Edit                                             #Ctrl-Shift-F4, F4#

    #F4# invokes ~internal~@Editor@, external or ~associated~@FileAssoc@
editor, depending upon the file type and ~external editor settings~@EditorSettings@.
    #Ctrl-Shift-F4# always calls internal editor ignoring file associations.
    #F4# and #Ctrl-Shift-F4# for directories invoke the change file
~attributes~@FileAttrDlg@ dialog.

  ~Copy~@CopyFiles@                                                            #F5#

    Copies files and folders. If you wish to create the destination folder
before copying, terminate the name with a backslash.

  ~Rename or move~@CopyFiles@                                                  #F6#

    Moves or renames files and folders. If you wish to create the destination
folder before moving, terminate the name with a backslash.

  ~Create new folder~@MakeFolder@                                               #F7#

  ~Delete~@DeleteFile@                                     #Shift-Del, Shift-F8, F8#

  ~Wipe~@DeleteFile@                                                       #Alt-Del#

  Show ~menus~@Menus@ bar                                                  #F9#

  Quit FAR2L                                                     #F10#

  Show ~plugin~@Plugins@ commands                                           #F11#

  Change the current location for left panel                  #Alt-F1#

  Change the current location for right panel                 #Alt-F2#

  Internal/external viewer                                    #Alt-F3#

    If the internal viewer is used by default, invokes the external viewer
specified in the ~settings~@ViewerSettings@ or the ~associated viewer program~@FileAssoc@
for the file type. If the external viewer is used by default, invokes the
internal viewer.

  Internal/external editor                                    #Alt-F4#

    If the internal editor is used by default, invokes the external editor
specified in the ~settings~@EditorSettings@ or the ~associated editor program~@FileAssoc@
for the file type. If the external editor is used by default, invokes the
internal editor.

  Print files                                                 #Alt-F5#

    If the "Print Manager" plugin is installed then the printing of
    the selected files will be carried out using that plugin,
    otherwise by using internal facilities.

  Create ~file links~@HardSymLink@ (NTFS only)                               #Alt-F6#

    Using hard file links you may have several different file names referring
to the same data.

  Perform ~find file~@FindFile@ command                                   #Alt-F7#

  Display ~commands history~@History@                                    #Alt-F8#

  Toggles the size of the FAR2L console window                #Alt-F9#

    In the windowed mode, toggles between the current size and the maximum
possible size of a console window. In the fullscreen mode, #Alt-F9# toggles the
screen height between 25 and 50 lines. See TechInfo##38 for details.

  Configure ~plugin~@Plugins@ modules.                             #Alt-Shift-F9#

  Perform ~find folder~@FindFolder@ command                                #Alt-F10#

  Display ~view and edit history~@HistoryViews@                              #Alt-F11#

  Display ~folders history~@HistoryFolders@                                    #Alt-F12#

  Add files to archive                                      #Shift-F1#
  Extract files from archive                                #Shift-F2#
  Perform archive managing commands                         #Shift-F3#
  Edit ~new file~@FileOpenCreate@                                             #Shift-F4#

    When a new file is opened, the same code page is used as in the last opened
editor. If the editor is opened for the first time in the current FAR2L session,
the default code page is used.

  Copy file under cursor                                    #Shift-F5#
  Rename or move file under cursor                          #Shift-F6#

    For folders: if the specified path (absolute or relative) points to an
existing folder, the source folder is moved inside that folder. Otherwise the
folder is renamed/moved to the new path.
    E.g. when moving #/folder1/# to #/folder2/#:
    - if #/folder2/# exists, contents of #/folder1/# is
moved into #/folder2/folder1/#;
    - otherwise contents of #/folder1/# is moved into the
newly created #/folder2/#.

  ~Delete file~@DeleteFile@ under cursor                                  #Shift-F8#
  Save configuration                                        #Shift-F9#
  Selects last executed menu item                          #Shift-F10#

  Execute, change folder, enter to an archive                  #Enter#
  Execute in the separate window                         #Shift-Enter#
  Execute as administrator                            #Ctrl-Alt-Enter#

    Pressing #Shift-Enter# on a directory invokes the system GUI file browser and
shows the selected directory. To show a root directory in the GUI file browser, you
should press #Shift-Enter# on the required path in the ~location menu~@DriveDlg@.
Pressing #Shift-Enter# on "#..#" opens the current directory in the GUI file browser.

  Change to the root folder                                           #Ctrl-\\#

  Change to the home directory (~~)                                    #Ctrl-`#

  Change folder, enter an archive (also a SFX archive),    #Ctrl-[Shift-]PgDn#

    If the cursor points to a directory, pressing #Ctrl-PgDn# changes to that
directory. If the cursor points to a file, then, depending on the file type,
an ~associated command~@FileAssoc@ is executed or the archive is opened.
    #Ctrl-Shift-PgDn# always opens the archive, regardless of the associated
command configuration.

  For symlink jump to target symlink                         #Ctrl-Shift-PgDn#
   (for others files a la #Ctrl-PgDn#)

  Change to the parent folder                                      #Ctrl-PgUp#

    If the option "~Use Ctrl-PgUp for location menu~@InterfSettings@" is enabled,
pressing #Ctrl-PgUp# in the root directory switches to the network plugin or
shows the ~location menu~@DriveDlg@.

  Revert to symlink                                          #Ctrl-Shift-PgUp#
   (only if before was jump by #Ctrl-Shift-PgDn# to target symlink)

  Create bookmark to the current folder              #Ctrl-Shift-0..9#

  Use folder bookmark                                #RightCtrl-0..9#

  Set ~file attributes~@FileAttrDlg@                                         #Ctrl-A#
  ~Apply command~@ApplyCmd@ to selected files                             #Ctrl-G#
  ~Describe~@FileDiz@ selected files                                     #Ctrl-Z#


@DeleteFile
$ #Deleting and wiping files and folders#
    The following hotkeys are used to delete or wipe out files and folders:

    #F8#         - if any files or folders are selected in the panel
                 then the selected group will be deleted, otherwise
                 the object currently under cursor will be deleted;

    #Shift-F8#   - delete only the file under cursor
                 (with no regard to selection in the panel);

    #Shift-Del#  - delete selected objects, skipping the Recycle Bin;

    #Alt-Del#    - Wipe out files and folders.


    Remarks:

    1. ^<wrap>In accordance to ~System Settings~@SystemSettings@ the hotkeys #F8# and
#Shift-F8# do or do not move the deleted files to the Recycle Bin. The
#Shift-Del# hotkey always deletes, skipping the Recycle Bin.

    2. ^<wrap>Before file deletion its data is overwritten with zeroes (you can
specify other overwrite characters - see TechInfo##29), after which the file
is truncated to a zero sized file, renamed to a temporary name and then
deleted.


@ErrCannotExecute
$ #Error: Cannot execute#
    The program you tries to execute is not recognized as an internal or
external command, operable program or batch file.

    When executing the contents of the command line, FAR2L searches for the
executable in the following sequence (sequentially substituting all extensions
listed in the environment variable %PATHEXT%):

  1. The current directory
  2. The directories that are listed in the PATH environment variable


@MiscCmd
$ #Panel control commands - miscellaneous#
  Screen grabber                                             #Alt-Ins#

    Screen grabber allows to select and copy to the clipboard any screen area.
Use #arrow# keys or click the #left mouse button# to move the cursor. To select
text use #Shift-arrow# keys or drag the mouse while holding the #left mouse#
#button#. #Enter#, #Ctrl-Ins#, #right mouse button# or #doubleclick# copy
selected text to the clipboard, #Ctrl-<Gray +># appends it to the clipboard
contents, #Esc# leaves the grabbing mode. #Ctrl-U# - deselect block.

  Record a ~keyboard macro~@KeyMacro@                                   #Ctrl-<.>#

  History in dialog edit controls                 #Ctrl-Up, Ctrl-Down#

    In dialog edit control history you may use #Enter# to copy the current item
to the edit control and #Ins# to mark or unmark an item. Marked items are not
pushed out of history by new items, so you may mark frequently used strings so
that you will always have them in the history.

  Clear history in dialog edit controls                          #Del#

  Delete the current item in a dialog edit line history
  list (if it is not locked)                               #Shift-Del#

  Set the dialog focus to the default element                   #PgDn#

  Insert a file name under cursor to dialog              #Shift-Enter#

  Insert a file name from passive panel to dialog   #Ctrl-Shift-Enter#

    This key combination is valid for all edit controls except the command
line, including dialogs and the ~internal editor~@Editor@.

    Pressing #Ctrl-Enter# in dialogs executes the default action (pushes the
default button or does another similar thing).

    In dialogs, when the current control is a check box:

  - turn on (#[x]#)                                             #Gray +#
  - turn off (#[ ]#)                                            #Gray -#
  - change to undefined (#[?]#)                                 #Gray *#
    (for three-state checkboxes)

    #Left clicking# outside the dialog works the same as pressing #Esc#.

    #Right clicking# outside the dialog works the same as pressing #Enter#.

    Clicking the #middle mouse button# in the ~panels~@PanelCmd@ has the same
effect as pressing the #Enter# key with the same modifiers (#Ctrl#, #Alt#,
#Shift#). If the ~command line~@CmdLineCmd@ is not empty, its contents will be
executed.

    FAR2L also supports the ~mouse wheel~@MsWheel@.

    You can move a dialog (window) by dragging it with mouse or by pressing
#Ctrl-F5# and using #arrow# keys.

    #Sticky controls# if your environment doesn't allow you to use some hotkeys
due to TTY backend limitations or same hotkey used by other app you can following
trick to achieve 'sticky' control keys behaviour. That means control key kept
virtually pressed until next non-control key press:
    #Ctrl+SPACE# gives sticky CONTROL key
    #Alt+SPACE# gives sticky ALT key
    #RCtrl+SPACE# gives sticky RCONTROL key
    #RAlt+SPACE# gives sticky RALT key
    Another way to achieve working hotkeys may be changing settings
of desktop environment or external applications (in order to release needed hotkey combinations)
or using exclusive handle hotkeys option
in the ~Input Settings~@InputSettings@ (only in GUI backend mode).

@SpecCmd
$ #Special commands#
 Special FAR pseudo-command usually starting with a prefix and a colon are processed
in the far2l ~internal command line~@CmdLineCmd@ and
in ~associated commands~@FileAssoc@, ~user menu~@UserMenu@ and the ~apply command~@ApplyCmd@.

   #far:about#  - Far information, list and information about plugins.

   #far:config# - ~Configuration editor~@FarConfig@.

   #view:file# or #far:view:file# or #far:view file# - open in viewer existing #file#.
   #view:<command# or #far:view:<command# or #far:view < command# - open in viewer result of #command# output in temporary file.

   #edit:file# or #far:edit:file# or #far:edit file# - open in editor #file# (if #file# not exist will be open empty).
   #edit:# or #far:edit:# or #far:edit# - open in editor new empty file.
   #edit:<command# or #far:edit:<command# or #far:edit < command# - open in editor result of #command# output in temporary file.

   #exit#       - reset shell in build-in ~Terminal~@Terminal@.

   #exit far#   - close far2l.

 Plugins can define their own command prefixes, see for each available plugin list of Command Prefixes via #far:about#.

@FarConfig
$ #Configuration editor#
 Starts with the ~pseudo-command~@SpecCmd@ #far:config# in the far2l internal command line.

 Allows to view and edit all Far Manager’s options.

 Most options can be changed from the ~Options menu~@OptMenu@,
however some options are available only here or in configuration ini-files.

 The options are displayed in a list with four fields per item:
  #-# The name in the SectionName.ParamName format (for example, Editor.TabSize)
  #-# The type (boolean, integer, dword, string, binary or unknown)
  #-# Whether the option is saved when Far configuration is saved (s) or not (-)
  #-# The value (for integer or dword types the hexadecimal representation additionally displayed).
 If current value of an option is other than the default, the option is marked with the ‘*’ character to the left of the name
(‘?’ character marked items without default value).

 Besides the list navigation keys, the following key combinations are supported:

 #Enter# or #F4#       Edit the value.

 #Del#               Reset the item to its default value.

 #Ctrl-H#            Toggle display of all or only changed items.

 #Ctrl-A#            Toggle column name arranging by left or by dot.

 #Ctrl-Alt-F#        Toggle quick filtering mode.

 #Esc# or #F10#        Close.

    See also: common ~menu~@MenuCmd@ keyboard commands.

@MsWheel
$ #Mouse: wheel support#

   #Panels#    Rotating the wheel scrolls the file list without
             changing the cursor position on the screen. Pressing
             the #middle button# has the same effect as pressing
             #Enter#.

   #Editor#    Rotating the wheel scrolls the text without changing
             the cursor position on the screen (similar to #Ctrl-Up#/
             #Ctrl-Down#).

   #Viewer#    Rotating the wheel scrolls the text.

   #Help#      Rotating the wheel scrolls the text.

   #Menus#     Wheel scrolling works as #Up#/#Down# keys.
             Pressing the #middle button# has the same effect as
             pressing #Enter#. It is possible to choose items without
             moving the cursor.

   #Dialogs#   In dialogs, when the wheel is rotated at an edit line
             with a history list or a combo box, the drop-down list
             is opened. In the drop-down list scrolling works the
             same as in menus.

    You can specify the number of lines to scroll at a time in the panels,
editor and viewer (see TechInfo##33).


@Plugins
$ #Plugins support#
    External DLL modules (plugins) may be used to implement new FAR2L commands
and emulate file systems. For example, archives support, FTP client, temporary
panel and network browser are plugins that emulate file systems.

    All plugins are stored in separate folders within the 'Plugins' folder,
which is in the same folder as far2l file. When detecting a new plugin FAR2L
saves information about it and later loads the plugin only when necessary,
so unused plugins do not require additional memory. But if you are sure that
some plugins are useless for you, you may remove them to save disk space.

    Plugins may be called either from ~location menu~@DriveDlg@ or from
#Plugin commands# menu, activated by #F11# or by corresponding item of
~Commands menu~@CmdMenu@. #F4# in ~"Plugin commands"~@PluginCommands@ menu allows to assign hot
keys to menu items (this makes easier to call them from ~keyboard macros~@KeyMacro@).
This menu is accessible from file panels, dialogs and (only by #F11#) from the
internal viewer and editor. Only specially designed plugins will be shown when
calling the menu from dialogs, the viewer or the editor.

    You may set plugin parameters using ~Plugin configuration~@PluginsConfig@
command from ~Options menu~@OptMenu@.

    File processing operations like copy, move, delete, edit or ~Find file~@FindFile@
work with plugins, which emulate file systems, if these plugins provide
necessary functionality. Search from the current folder in the "Find file"
command requires less functionality than search from the root folder, so try to
use it if search from the root folder does not work correctly.

    The modules have their own message and help files. You can get a list of
available help on the modules by pressing

    #Shift-F2# - anywhere in the FAR2L help system

    #Shift-F1# - in the list of plugins (context-dependent help).

    If the plugin has no help file, then context-dependent help will not pop
out.

    If the active panel shows a plugin emulated file system, the command "CD"
in the command line may be used to change the plugin file system folder. Unlike
"CD", "CHDIR" command always treats the specified parameter as a real folder
name regardless a file panel type.

    Use #Alt-Shift-F9# to ~configure plugins~@PluginsConfig@.


@PluginCommands
$ #Plugin commands#
    This menu provides user with ability to use plugins functionality (other
ways are listed in ~"Plugins support"~@Plugins@).
The contents of the menu and actions triggered on menu items selection are
controlled by plugins.

    The menu can be invoked in the following ways:

  - #F11# at file panels or #Plugins# item at ~commands menu~@CmdMenu@, herewith
    the commands intended for use from file panels are shown;
  - #F11# in viewer or editor, herewith the commands intended for use from
    viewer and editor accordingly are shown.

    Each item of plugin commands menu can be assigned a hotkey with #F4#, this
possibility is widely used in ~key macros~@KeyMacro@. The assigned hotkey is
displayed left to the item. The #A# symbol in leftmost menu column means that
the corresponding plugin is written for Far 1.7x and it does not support all
possibilities available in Far 2 (these are, in particular, Unicode characters
in filenames and in editor).

    #Plugin commands# menu hotkeys:

    #Shift-F1#    - help on use for selected menu item. The text of the help
                  is taken from HLF file, associated with the plugin
                  that owns the menu item.
    #F4#          - assign a hotkey for selected menu item. If #Space# is
                  entered, then Far sets the hotkey automatically.
    #Shift-F9#    - settings of the selected plugin.
    #Alt-Shift-F9# - open ~"Plugins configuration"~@PluginsConfig@ menu.

    See also:
 
    ~Plugins support~@Plugins@.
    Common ~menu~@MenuCmd@ keyboard commands.

@PluginsConfig
$ #Plugins configuration#
    You can configure the installed ~plugin modules~@Plugins@ using the command
#"Plugins configuration"# from the ~Options menu~@OptMenu@ or by pressing
#Alt-Shift-F9# in the ~location menu~@DriveDlg@ or plugins menu.

    To get the help on the currently selected plugin, press #Shift-F1# -
context-sensitive help on plugin configuration. If the plugin doesn't have a
help file, the context-sensitive help will not be shown.

    When the context-sensitive help is invoked, FAR2L will try to show the topic
#Config#. If such a topic does not exist in the plugin help file, the main help
topic for the selected plugin will be shown.

    Each item of plugins configuration menu can be assigned a hotkey with #F4#,
this possibility is widely used in ~key macros~@KeyMacro@. The assigned hotkey is
displayed left to the item. The #A# symbol in leftmost menu column means that
the corresponding plugin is written for Far 1.7x and it does not support all
possibilities available in Far 2 (these are, in particular, Unicode characters
in filenames and in editor).

    See also: common ~menu~@MenuCmd@ keyboard commands.

@PluginsReviews
$ #Overview of plugin capabilities#
    The FAR2L manager is so tightly integrated with its plugins that it is simply
meaningless to talk about FAR2L and not to mention the plugins. Plugins present
an almost limitless expansion of the features of FAR2L.

@Panels
$ #Panels #
    Normally FAR2L shows two panels (left and right windows), with different
information. If you want to change the type of information displayed in the
panel, use the ~panel menu~@LeftRightMenu@ or corresponding ~keyboard commands~@KeyRef@.

    See also the following topics to obtain more information:

      ~File panel~@FilePanel@                 ~Tree panel~@TreePanel@
      ~Info panel~@InfoPanel@                 ~Quick view panel~@QViewPanel@

      ~Drag and drop files~@DragAndDrop@
      ~Selecting files~@SelectFiles@
      ~Customizing file panel view modes~@PanelViewModes@


@FilePanel
$ #Panels: file panel#
    The file panel displays the current folder. You may select or deselect
files and folders, perform different file and archive operations. Read
~Keyboard reference~@KeyRef@ for commands list.

    Default view modes of the file panel are:

 #Brief#         File names are displayed within three columns.

 #Medium#        File names are displayed within two columns.

 #Full#          Name, size, date and time of the file are displayed.

 #Wide#          File names and sizes are displayed.

 #Detailed#      File names, sizes, physical sizes, last write,
               creation, access time and attributes are displayed.
               Fullscreen mode.

 #Descriptions#  File names and ~file descriptions~@FileDiz@

 #Long#          File names, sizes and descriptions.
 #descriptions#  Fullscreen mode.

 #File owners#   File names, sizes and owners.

 #File links#    File names, sizes and number of hard links.

 #Alternative#   File name, size (formatted with commas) and date
 #full#          of the file are displayed.

    You may ~customize file panel view modes~@PanelViewModes@.

    Physical size indicates storage space actually used by file. In most
cases it will be equal to logical file size rounded up by cluster size, but
for compressed or sparsed files this size can be smaller than logical size.


    If you wish to change the panel view mode, choose it from the
~panel menu~@LeftRightMenu@. After the mode change or location menu use,
if the initial panel type differs it will be automatically set to the file
panel.

    ~Speed search~@FastFind@ action may be used to point to the required file
by the first letters of its name.

    See also the list of ~macro keys~@KeyMacroShellList@, available in the panels.

@TreePanel
$ #Panels: tree panel#
    The tree panel displays the folder structure of file system as a tree.
Within tree mode you may change to a folder quickly and perform folder
operations.

    !! Windows legacy (not relevant on Linux/*BSD/Mac) !!

    FAR stores folder tree information in the file named #Tree.Far# at root
folder of each drive. For read-only drives this information is stored in the
hidden folder Tree.Cache within the folder containing FAR.EXE. The Tree.FAR
file doesn't exist by default. It will be automatically created after the first
use of the #Tree Panel# or the #Find Folder# command. If that file exists, FAR
updates it with the changes to the tree structure it is aware of. If such
changes were made outside of FAR and Tree.far is no longer current, it can be
refreshed by pressing #Ctrl-R#.

    !! Windows legacy end !!

    You can find a folder quickly with the help of #speed search# action. Hold
the Alt key and type the folder name until you point to the right folder.
Pressing #Ctrl-Enter# keys simultaneously will select the next match.

    #Gray +# and #Gray -# keys move up and down the tree to the next branch
on the same level.

    See also the list of ~macro keys~@KeyMacroTreeList@, available in the folder tree panel.

@InfoPanel
$ #Panels: info panel#
    The information panel contains the following data:

 - ^<wrap>#network# names of the computer and the current user (see ~Info panel settings~@InfoPanelSettings@);

 - ^<wrap>type of the file system of the #current disk#, network
name, total and free space, disk volume label and serial number;

 - ^<wrap>#memory# load percentage (100% means all of available memory is used),
size of the installed physical memory if possible, total and free size of the physical
memory (available for OS), virtual memory and paging file;

 - #folder description# file

    You may view the contents of the folder description file in full screen by
pressing the #F3# key or clicking the #left mouse button#. To edit or create the
description file, press #F4# or click the #right mouse button#. You can also use
many of the ~viewer commands~@Viewer@ (search, code page selection and so on)
for viewing the folder description file.

    A list of possible folder description file names may be defined using
"Folder description files" command in the ~Options menu~@OptMenu@.

    !! Windows legacy (not relevant on Linux/*BSD/Mac) !!

    FAR will attempt to determine the type of each of the CD drives available
in the system. Known types are as follows: CD-ROM, CD-RW, CD-RW/DVD, DVD-ROM,
DVD-RW and DVD-RAM. This function is available only for users either with
administrative privileges or all local users, when it's stated explicitly in
the Local Policy Editor (to do this, run a #secpol.msc# from the command
prompt, and set the '#Local Policies/Security Options/Devices: Restrict#
#CD-ROM access to locally logged-on user only#' setting to '#Enabled#')

    For virtual devices (SUBST-disk) the parameters of the primary disk are
shown.

    !! Windows legacy end !!

    See also the list of ~macro keys~@KeyMacroInfoList@, available in the info panel.

@QViewPanel
$ #Panels: quick view panel#
    The quick view panel is used to show information about the selected item in
the ~file panel~@FilePanel@ or ~tree panel~@TreePanel@.

    If the selected item is a file then the contents of the file is displayed.
Many of the ~internal viewer~@Viewer@ commands can be used with the file
displayed in the panel. For files of registered types the type is shown
as well.

    For folders, the quick view panel displays total size, total compressed
size, number of files and subfolders in the folder, current disk cluster size,
real files size, including files slack (sum of the unused cluster parts).
Compressed size has meaning for NTFS drives only.

    When viewing reparse points, the path to the source folder is also displayed.

    For folders, the total size value may not match the actual value:

    1. ^<wrap>If the folder or its subfolders contain symbolic links and the option
"Scan symbolic links" in the ~System parameters dialog~@SystemSettings@ is
enabled.

    2. ^<wrap>If the folder or its subfolders contain multiple hard links to the same
file.

    See also the list of ~macro keys~@KeyMacroQViewList@, available in the quick view panel.

@DragAndDrop
$ #Copying: drag and drop files#
    It is possible to perform #Copy# and #Move# file operations using #drag and
drop#. Press left mouse button on the source file or folder, drag it to the
another panel and release the mouse button.

    If you wish to process a group of files or folders, select them before
dragging, click the left mouse button on the source panel and drag the files
group to the other panel.

    You may switch between copy and move operations by pressing the right mouse
button while dragging. Also to move files you can hold the #Shift# key while
pressing the left mouse button.


@Menus
$ #Menus #
    To choose an action from the menu you may press F9 or click on top of the
screen.

    When the menu is activated by pressing #F9#, the menu for the active panel
is selected automatically. When the menu is active, pressing #Tab# switches
between the menus for the left and right panel. If the menus "Files",
"Commands" or "Options" are active, pressing #Tab# switches to the menu of the
passive panel.

    Use the #Shift-F10# key combination to select the last used menu command.

    Read the following topics for information about a particular menu:

     ~Left and right menus~@LeftRightMenu@          ~Files menu~@FilesMenu@

     ~Commands menu~@CmdMenu@                 ~Options menu~@OptMenu@

    See also the list of ~macro keys~@KeyMacroMainMenuList@, available in the main menu.

@LeftRightMenu
$ #Menus: left and right menus#
    The #Left# and #Right# menus allow to change left and right panel settings
respectively. These menus include the following items:

   #Brief#                Display files within three columns.

   #Medium#               Display files within two columns.

   #Full#                 Display file name, size, date and time.

   #Wide#                 Display file name and size.

   #Detailed#             Display file name, size, physical size,
                        last write, creation and access time,
                        attributes. Fullscreen mode.

   #Descriptions#         File name and ~file description~@FileDiz@.

   #Long descriptions#    File name, size and description.
                        Fullscreen mode.

   #File owners#          File name, size and owner.

   #File links#           File name, size and number of hard links.

   #Alternative full#     File name, formatted size and date.

   #Info panel#           Change panel type to ~info panel~@InfoPanel@.

   #Tree panel#           Change panel type to ~tree panel~@TreePanel@.

   #Quick view#           Change panel type to ~quick view~@QViewPanel@.

   #Sort modes#           Show available sort modes.

   #Show long names#      Show long/short file names.

   #Panel On/Off#         Show/hide panel.

   #Re-read#              Re-read panel.

   #Location#             Change current location.

    See also: common ~menu~@MenuCmd@ keyboard commands.

@FilesMenu
$ #Menus: files menu#
   #View#               ~View files~@Viewer@, count folder sizes.

   #Edit#               ~Edit~@Editor@ files.

   #Copy#               ~Copy~@CopyFiles@ files and folders.

   #Rename or move#     ~Rename or move~@CopyFiles@ files and folders.

   #Make folder#        ~Create~@MakeFolder@ new folder.

   #Delete#             Delete files and folders.

   #Wipe#               Wipe files and folders. Before file deletion
                      its data are overwritten with zeroes, after
                      which the file is truncated and renamed to
                      a temporary name.

   #Add to archive#     Add selected files to an archive.

   #Extract files#      Extract selected files from an archive.

   #Archive commands#   Perform archive managing commands.

   #File attributes#    ~Change file attributes~@FileAttrDlg@ and time.

   #Apply command#      ~Apply command~@ApplyCmd@ to selected files.

   #Describe files#     Add ~descriptions~@FileDiz@ to the selected files.

   #Select group#       ~Select~@SelectFiles@ a group of files with a wildcard.

   #Deselect group#     ~Deselect~@SelectFiles@ a group of files with a wildcard.

   #Invert selection#   ~Invert~@SelectFiles@ current file selection.

   #Restore selection#  ~Restore~@SelectFiles@ previous file selection after file
                      processing or group select operation.

   Some commands from this menu are also described in the
~File management and service commands~@FuncCmd@ topic.

    See also: common ~menu~@MenuCmd@ keyboard commands.

@CmdMenu
$ #Menus: commands menu#
   #Find file#            Search for files in the folders tree,
                        wildcards may be used.
                        See ~Find file~@FindFile@ for more info.

   #History#              Display the previous commands.
                        See ~History~@History@ for more info.

   #Video mode#           Switch between 25 and 50 lines on the screen.

   #Find folder#          Search for a folder in the folders
                        tree. See ~Find folder~@FindFolder@ for more info.

   #File view history#    Display files ~view and edit history~@HistoryViews@.

   #Folders history#      Display folders ~changing history~@HistoryFolders@.

                        Items in "Folders history" and "File view
                        history" are moved to the end of list after
                        selection. Use #Shift-Enter# to select item
                        without changing its position.

   #Swap panels#          Swap left and right panels.

   #Panels On/Off#        Show/hide both panels.

   #Compare folders#      Compare contents of folders.
                        See ~Compare folders~@CompFolders@ for the
                        detailed description.

   #Edit user menu#       Allows to edit main or local ~user menu~@UserMenu@.
                        You may press #Ins# to insert, #Del# to delete
                        and #F4# to edit menu records.

   #Edit associations#    Displays the list of ~file associations~@FileAssoc@.
                        You may press #Ins# to insert, #Del# to delete
                        and #F4# to edit file associations.

   #Bookmarks#     Displays current ~Bookmarks~@Bookmarks@.

   #File panel filter#    Allows to control file panel contents.
                        See ~filters menu~@FiltersMenu@ for the detailed
                        description.

   #Plugin commands#      Show ~plugin commands~@Plugins@ list

   #Screens list#         Show open ~screens list~@ScrSwitch@

   #Task list#            Shows ~active tasks list~@TaskList@.

   #Hotplug devices list# Show ~hotplug devices list~@HotPlugList@.

    See also: common ~menu~@MenuCmd@ keyboard commands.

@OptMenu
$ #Menus: options menu#
   #System settings#       Shows ~system settings~@SystemSettings@ dialog.

   #Panel settings#        Shows ~panel settings~@PanelSettings@ dialog.

   #Interface settings#    Shows ~interface settings~@InterfSettings@ dialog.

   #Input settings#        Shows ~input settings~@InputSettings@ dialog.

   #Dialog settings#       Shows ~dialog settings~@DialogSettings@ dialog.

   #Menu settings#         Shows ~menu settings~@VMenuSettings@ dialog.

   #Command line settings# Shows ~command line settings~@CmdlineSettings@ dialog.

   #Languages#             Select main and help language.
                         Use "Save setup" to save selected languages.

   #Plugins#               Configure ~plugin~@Plugins@ modules.
   #configuration#

   #Confirmation#          Switch on or off ~confirmations~@ConfirmDlg@ for
                         some operations.

   #File panel modes#      ~Customize file panel view modes~@PanelViewModes@ settings.

   #File descriptions#     ~File descriptions~@FileDiz@ list names and update mode.

   #Folder description#    Specify names (~wildcards~@FileMasks@ are allowed) of
   #files#                 files displayed in the ~Info panel~@InfoPanel@ as folder
                         descriptions.

   #Viewer settings#       External and internal ~viewer settings~@ViewerSettings@.

   #Editor settings#       External and internal ~editor settings~@EditorSettings@.

   #Colors#                Allows to select colors for different
                         interface items, to change the entire FAR2L
                         colors palette to black and white or to set
                         the colors to default.

   #Files highlighting#    Change ~files highlighting and sort groups~@Highlight@
   #and sort groups#       settings.

   #Save setup#            Save current configuration, colors and
                         screen layout.

   See also: common ~menu~@MenuCmd@ keyboard commands.

@Terminal
$ #Terminal#
    #FAR2L# contains built-in terminal emulator, allowing to execute command line applications see their output and control functionality.
In order to keep usual shell experience far2l first launches supported user's shell in interactive mode and sends it commands typed
in its own command line.
    #Autocomplete# FAR2L has two independent command line autocompletion mechanisms. First is original FAR's driven-based autocomplete and works
by default by giving options while you're typing command. Second is driven by bash autocompletion and can be activated by pressing
#SHIFT+double-TAB# (quickly press TAB twice while keeping SHIFT pressed).
    #'exit' command behaviour:# typing 'exit' command will cause back shell to exit but will not close whole far2l application to close and next
command execution request will spawn new back shell instance. This allows to 'reset' shell environment from exported variables and other settings.
In case you want to exit far2l by typing command: type 'exit far' ~pseudo-command~@SpecCmd@ - it will be recognized by far2l as whole app close request.
    #Hotkeys and scrolling during running command:# you can use #Ctrl+Shift+F3# to open history of output in built-in viewer or
#Ctrl+Shift+F4# to open it in built-in editor. This allows efficient commands output investigation, including scrolling possibility, using
built-in viewer and editor capabilities. You can also open history viewer by scrolling mouse wheel up, following scroll til bottom of output
- will hide that viewer. #Ctrl+C, Ctrl+Z# hotkeys trigger usual signals, however in case hard stuck of command line application you can hard kill
it and everything in shell by pressing #Ctrl+Alt+C#. Note that its not recommended to use that hotkey without real need cuz it may cause corruption
or lost of unsaved data in killed applications. You can also use #Ctrl+Alt+Z# to put command execution to background.
You may return to background'ed command from ~Screens switching menu~@ScrSwitch@ (#F12# in panels).
    #Hotkeys and scrolling when NOT running command:# while #Ctrl+Shift+F3/F4# still functioning in such mode you can also use simple #F3/F4# to get history
opened in viewer/editor respectively. Also you can press #F8# key to cleanup history and screen. You can switch between panels and terminal by pressing #Ctrl+O#
or clicking top left corner.
    #FAR2L terminal extensions# while FAR2L itself is TUI application, it may run in ~GUI or TTY backends modes~@UIBackends@. While TTY backend may function in usual
terminal like xterm or SSH session but it may also run inside of terminal of GUI-mode far2l gaining capabilities inachievable under 'usual' terminals,
like live full keyboard keys recognition with with keydown/up reaction. Also 'host' far2l may provide shared clipboard access and desktop notifications.
You can use this functionality by running TTY far2l inside of ssh client session opened in 'host' far2l or, what is more easy, by using SSH-capable plugin,
like NetRocks SFTP/SCP protocols to execute remote commands.

  Text selected with mouse automatically copied to clipboard.

  Previous command                                          #Up, Ctrl-E#
  Next command                                            #Down, Ctrl-X#
  Clear command line                                            #Ctrl-Y#
    (see also ~Keyboard reference of Command line~@CmdLineCmd@)

  Autocomplete (FAR2L mechanisms)                                  #Tab#
  Autocomplete (bash mechanisms)                         #Shift-Tab-Tab#

  Hide/show both panels                                         #Ctrl-O#
  Hide/show left panel                                         #Ctrl-F1#
  Hide/show right panel                                        #Ctrl-F2#

  Insert current file name from the active panel            #Ctrl-Enter#
  Insert current file name from the passive panel     #Ctrl-Shift-Enter#
  Insert full file name from the active panel                   #Ctrl-F#
  Insert full file name from the passive panel                  #Ctrl-;#

  Terminal->Viewer                                   #F3, Ctrl+Shift+F3#
    (all terminal output history in built-in Viewer - useful for scrolling of long output)

  Terminal->Editor                                   #F4, Ctrl+Shift+F4#
    (all terminal output history in built-in Editor)

  Cleanup terminal history and screen                               #F8#

  Usual signals                                         #Ctrl+C, Ctrl+Z#

  Hard kill everything in shell                             #Ctrl+Alt+C#
    (not recommended, it may cause corruption or lost of unsaved data)

  Put far2l instance to background                          #Ctrl+Alt+Z#
    (only if far2l works in TTY backend)

  See also: ~pseudo-commands~@SpecCmd@

@UIBackends
$ #UI Backends#
    Depending on build options and available platform features #FAR2L# can render
its interface using different so-called backends:

    - #GUI backend:# renders into own GUI window, providing most complete keyboard hotkeys support.
    - #TTY backend:# renders into plain TTY terminal. Its a most compatible way but also providing
most lame UX: some hotkeys may not work, clipboard is not shared with host etc.
    - #TTY|X backend:# renders into TTY terminal and uses X11 to access clipboard and to get state of
keyboard modifiers. It provides better UX than plain TTY, but still some key combinations may be inaccessible.
    - #TTY|Xi backend:# renders into TTY terminal and uses X11 with Xi extension to access clipboard
and to get state of all keyboard keys. It provides much better UX than plain TTY.
    - #TTY|F backend:# renders into TTY terminal hosted by another far2l instance. It provides UX
identical to GUI backend (if terminal hosted by GUI far2l).

    If you want to run far2l remotely with best UX its recommended to run it within NetRocks
connection that allows to use TTY|F backend. If this is not wanted for some reason - you also may
consider running over ssh session with trusted X forwarding and compression (ssh -Y -C ...) that
allows using TTY|Xi or at least TTY|X backend. However its highly recommended to
#do not use trusted X forwarding when connecting to untrusted servers#,
because X forwarding opens uncontrollable ability for remote code
to listen all your keystrokes, grab clipboard content, get windows snapshots etc. So, TTY|F backend
is the only secure way to run far2l remotely on untrusted server while supporting all usual far2l
hotkeys and other UX conveniences.

    - Terminal emulators specific Backends (uses these terminal extensions to get state of all keyboard keys;
in pure TTY| to access clipboard you must turn on OSC 52 in both the FAR2L settings and the terminal settings;
TTY|X uses X11 to access clipboard):
        - #TTY|a# or #TTY|Xa backend:# renders into Apple iTerm2 terminal.
        - #TTY|k# or #TTY|Xk backend:# renders into kovidgoyal's Kitty (and any terminals with kovidgoyal's kitty keyboard protocol).
        - #TTY|w# or #TTY|Xw backend:# renders into Windows Terminal (and any terminals with win32 input mode).
    List and links to supported terminals see in ~FAR2L features - Getting Started~@Far2lGettingStarted@.

@ConfirmDlg
$ #Confirmations#
    In the #Confirmations# dialog you can switch on or off confirmations for
following operations:

    - overwrite destination files when performing file copying;

    - overwrite destination files when performing file moving;

    - overwrite and delete files with "read only" attribute;

    - ~drag and drop~@DragAndDrop@ files;

    - delete files;

    - delete folders;

    - interrupt operation;

    - ~unmounting~@DisconnectDrive@ from the Location menu;

    - clear terminal screen and history by pressing F8;

    - removal of USB storage devices from the Location menu;

    - ~reloading~@EditorReload@ a file in the editor;

    - clearing the view/edit, folder and command history lists;

    - exit from FAR2L.


@PluginsManagerSettings
$ #Plugins manager#

  #Path for personal plugins#
  Enter here the full path, where FAR2L will search for "personal" plugins in addition to the "main"
plugins. Several search paths may be given separated by ';'. Environment variables can be entered in the
search path. Personal plugins will not be loaded, if the switches -p or -co are given in the
~command line~@CmdLine@.

@ChoosePluginMenu
$ #Plugin selection menu#

    See also: common ~menu~@MenuCmd@ keyboard commands.

@MakeFolder
$ #Make folder#
    This function allows you to create folders. You can use environment
variables in the input line, which are expanded to their values before creating
the folder. Also you can create multiple nested subfolders at the same time:
simply separate the folder names with the backslash character. For example:

    #%USERDOMAIN%\\%USERNAME%\\Folder3#

    If the option "#Process multiple names#" is enabled, it is possible to
create multiple folders in a single operation. In this case, folder names
should be separated with the character "#;#" or "#,#". If the option is enabled
and the name of the folder contains a character "#;#" (or "#,#"), it must be
enclosed in quotes. For example, if the user enters
#/Foo1;"/foo,2;";/foo3#, folders called "#/Foo1#", "#/foo,2;#"
and "#/foo3#" will be created.


@FindFile
$ #Find file #
    This command allows to locate in the folder tree one or more files and
folders matching one or more ~wildcards~@FileMasks@ (delimited with commas). It
may also be used with file systems emulated by ~plugins~@Plugins@.

    Optional text string may be specified to find only files containing this
text. If the string is entered, the option #Case sensitive# selects case
sensitive comparison.

    The option #Whole words# will let to find only the text that is separated
from other text with spaces, tab characters, line breaks or standard
separators, which by default are: #!%^&*()+|{}:"<>?`-=\\[];',./#.

    By checking the #Search for hex# option you can search for the files
containing hexadecimal sequence of the specified bytes. In this case #Case#
#sensitive#, #Whole words#, #Using code page# and #Search for folders#
options are disabled and their values doesn't affect the search process.

    Выпадающий список #Используя кодовую страницу# позволяет выбрать конкретную
кодовую страницу, применяемую для поиска текста. Если в выпадающем списке выбрать
пункт #Все кодовые страницы#, то FAR2L будет использовать для поиска все стандартные
и #Любимые# кодовые страницы (список #Любимых# кодовых страниц можно настроить в
меню выбора кодовой страницы редактора или программы просмотра). Если перечень
кодовых страниц, поиск по которым производится при выборе пункта #Все кодовые#
#страницы#, является для вас избыточным, то вы можете, при помощи клавиш #Ins# и
#Space#, выбрать из списка стандартных и #Любимых# кодовых страниц только те кодовые
страницы, по которым вам необходимо осуществлять поиск.

    If the option #Search in archives# is set, FAR2L also performs the search in
archives with known formats. However, using this option significantly decreases
the performance of the search. FAR2L cannot search in nested archives.

    The #Search for folders# option includes in search list those folders, that
match the wildcards. Also the counter of found files takes account of found
folders.

    The #Search in symbolic links# option allows searching files in
~symbolic links~@HardSymLink@ along with normal sub-folders.

    Search may be performed:

    - in all non-removable drives;

    - in all local drives, except removable and network;

    - in all folders specified in the %PATH% environment variable
      (not including subfolders).

    - in all folders from one of folders in Location menu,
      in the find dialog one can change folder of the search. ;

    - from the current folder;

    - in the current folder only or in selected folders
      (the current version of FAR2L does not search in
      directories that are ~symbolic links~@HardSymLink@).

    The search parameters is saved in the configuration.

    Check the #Use filter# checkbox to search for files according to user
defined conditions. Press the #Filter# button to open the ~filters menu~@FiltersMenu@.

    The #Advanced# button invokes the ~find file advanced options~@FindFileAdvanced@
dialog that can be used to specify extended set of search properties.


@FindFileAdvanced
$ #Find file advanced options#
    The text string that is entered in #Containing text# (or #Containing hex#)
field can be searched not only in the whole file, but also inside a specified
range at the beginning of the file, defined by the #Search only in the first#
property. If the specified value is less than the file size, the rest of the
file will be ignored even if the required sequence exists there.

    The following file size suffixes can be used:
    B - for bytes (no suffix also means bytes);
    K - for kilobytes;
    M - for megabytes;
    G - for gigabytes;
    T - for terabytes;
    P - for petabytes;
    E - for exabytes.


    #Обрабатывать альтернативные потоки данных# - помимо основного потока данных
(представляющего собой непосредственно содержимое файла)
производить поиск также в альтернативных именованных потоках, поддерживаемых некоторыми
файловыми системами (например, #NTFS#).


  - #Column types# - позволяет задавать формат вывода результатов поиска.
Column types are encoded as one or several characters, delimited with commas.
Allowed column types are:

    S[C,T,F,E] - file size
    P[C,T,F,E] - packed file size
    G[C,T,F,E] - size of file streams
                 where: C - format file size;
                        T - use 1000 instead of 1024 as a divider;
                        F - show file sizes similar to Windows
                            Explorer (i.e. 999 bytes will be
                            displayed as 999 and 1000 bytes will
                            be displayed as 0.97 K);
                        E - economic mode, no space between file
                            size and suffix will be shown
                            (i.e. 0.97K);

    D          - file last write date
    T          - file last write time

    DM[B,M]    - file last write date and time
    DC[B,M]    - file creation date and time
    DA[B,M]    - file last access date and time
    DE[B,M]    - file change date and time
                 where: B - brief (Unix style) file time format;
                        M - use text month names;

    A          - file attributes
    Z          - file descriptions

    O[L]       - file owner
                 where: L - show domain name;

    LN         - number of hard links

    F          - number of streams


    Атрибуты файла имеют следующие обозначения:

       #R#         - Только для чтения
       #S#         - Системный
       #H#         - Скрытый
       #A#         - Архивный
       #L#         - Связь или символическая ссылка
       #C# или #E#   - Сжатый или Зашифрованный
       #$#         - Разрежённый (sparse)
       #T#         - Временный
       #I#         - Неиндексируемый по содержимому
       #O#         - Автономный (offline)
       #V#         - Виртуальный

    Порядок отображения атрибутов - RSHALCTIOV. Символ атрибута
"Разрежённый" применяется только для файлов и ставится вместо 'L'.
Символ атрибута "Зашифрованный" ставится вместо 'C', т.к. файл/каталог не
могут иметь одновременно оба атрибута ("Сжатый" и "Зашифрованный"). По
умолчанию размер колонки атрибутов - 6 символов. Для отображения
дополнительных атрибутов 'T', 'I', 'O' и 'V' необходимо явно указать
размер колонки в 10 символов.

    #Ширина колонок# - позволяет изменить ширину колонок результатов поиска.
Если ширина равна 0, то используется значение по умолчанию.

    Для использования 12-часового формата времени надо увеличить
на единицу стандартную ширину колонки времени файла или колонки
времени и даты файла. После дальнейшего увеличения в этих колонках
также будут показаны секунды и миллисекунды.

    Для показа года в 4-х символьном формате нужно увеличить ширину
колонки даты на 2.

    В отличии от режимов панелей, результат поиска может содержать только
одну колонку. Имя файла присутствует всегда - добавляется последней колонкой
автоматически.

    При указании колонок, отвечающих за показ ссылок и потоков (G, LN, и F) время
поиска увеличивается.

    Чтобы в результатах поиска отображать только имена файловых объектов без
дополнительных атрибутов, оставьте поле "Типы колонок" пустым.

    По умолчанию значения колонок равны:
    "Типы колонок"   - D,S,A
    "Ширина колонок" - 14,13,0


@FindFileResult
$ #Find file: control keys#
    While ~search~@FindFile@ is in progress or when it is finished, you may use
the cursor keys to scroll the files list and the buttons to perform required
actions.

    During or after search the following buttons are available:

   #New search#      Start new search session.

   #Go to#           Breaks current search, changes panel folder
                   and moves cursor to the selected file.

   #View#            View selected file. If search is not completed,
                   it will continue in the background while the file
                   is viewed.

   #Panel#           Create a temporary panel and fill it with the
                   results of the last file search.

   #Stop#            Break current search. Available while search
                   is in progress.

   #Cancel#          Close the search dialog.

    #F3# and #F4# may be used to view and edit found files. Also viewing and
editing is supported for plugin emulated file systems. Note, that saving editor
changes by #F2# key in emulated file system will perform #SaveAs# operation,
instead of common #Save#.


@FindFolder
$ #Find folder#
    This command allows a quick look for the required folder in the folders
tree.

    To select a folder you may use the cursor keys or type first characters of
the folder.

    Press #Enter# to switch to the selected folder.

    #Ctrl-R# and #F2# force the rescan of the folders tree.

    #Gray +# and #Gray -# should move up and down the tree to the next branch
on the same level.

    #F5# allows to maximize the window, pressing #F5# again will restore the
window to the previous size.

    By pressing #Ctrl-Enter#, you can cycle through the folders matching the
part of the filename that you have already entered. #Ctrl-Shift-Enter# allows
to cycle backwards.

    See also the list of ~macro keys~@KeyMacroFindFolderList@, available in the find folder dialog.

@Filter
$ #Filter#
    Operations filter is used to process certain file groups according to
rules specified by the user, if those rules are met by some file it
will be processed by a corresponding operation. A filter may have several
rule sets.

    The filter dialog consists of the following elements:

   #Filter name#     A title that will be shown in the filters menu.
                   This field can be empty.

                   Filter name is not available if the filter was
                   opened from ~Files highlighting and sort groups~@Highlight@.


   #Mask#            One or several ~file masks~@FileMasks@.

                   Filter conditions are met if file mask analysis
                   is on and its name matches any of the given file
                   masks. If mask analysis is off, file name is
                   ignored.


   #Size#            Minimum and maximum files size. The following
                   file size suffixes can be used:
                   B - for bytes (no suffix also means bytes);
                   K - for kilobytes;
                   M - for megabytes;
                   G - for gigabytes;
                   T - for terabytes;
                   P - for petabytes;
                   E - for exabytes.

                   Filter conditions are met if file size analysis
                   is on, and it is inside the given range.
                   If nothing is specified (empty line) for one
                   or both range boundaries then file size for that
                   boundary is not limited.

                   Example:
                   >= 1K - select files greater than or equal to 1 kilobyte
                   <= 1M - to less than or equal to 1 megabyte


   #Date/time#       Starting and ending file date/time.
                   You can specify the date/time of last file
                   #write#, file #creation#, last #access#
                   and #change#.

                   The button #Current# allows to fill the date/time
                   fields with the current date/time after which you
                   can change only the needed part of the date/time
                   fields, for example only the month or minutes.
                   The button #Blank# will clear the date/time
                   fields.

                   Filter conditions are met if file date/time
                   analysis is on and its date/time is in the
                   given range and corresponds to the given
                   type. If one or both of the date/time limit
                   fields are empty then the date/time for that
                   type is not limited.

                   Example:
                   <= 31.01.2010 - select files up to 31 numbers
                   >= 01.01.2010 - but after Jan. 1, 2010

                   Option #Relative# allows you to switch
                   to work with the date in relative time.
                   The logic at work this option is similar to
                   arithmetic with negative numbers.

                   Example:
                   <= 0 - select files in the period from the "Today"
                   >= 30 - and 30-days ago, including


   #Attributes#      Inclusion and exclusion attributes.

                   Filter conditions are met if file attributes
                   analysis is on and it has all of the inclusion
                   attributes and none of the exclusion attributes:
                   #[x]# - inclusion attribute - the file must have
                         this attribute.
                   #[ ]# - exclusion attribute - the file must
                         not have this attribute.
                   #[?]# - ignore this attribute.

                   The #Compressed#, #Encrypted#, #Not indexed#,
                   #Sparse#, #Temporary# and #Offline# attributes
                   are used only on disks with the NTFS file system.
                   #Virtual# attribute is not used in Windows
                   2000/XP/2003.


    To quickly disable one or several conditions, uncheck the corresponding
checkboxes. The #Reset# button will clear all of the filter conditions.

@HistoryCmd
$ #Common history list commands#

  Clear the commands history                                      #Del#

  Delete the current history item                           #Shift-Del#

  Lock/unlock a history item                                      #Ins#
   (locked item is not deleted by #Del# or #Shift-Del#)

  Copy the text of the current command to the clipboard        #Ctrl-C#
  without closing the list                                or #Ctrl-Ins#

  Toggle history view:                                         #Ctrl-T#
             * with date lines + time column
             * with date lines (as in far3)
             * plain history (as in far2)

  See also: common ~menu~@MenuCmd@ keyboard commands.

@History
$ #History#
    The commands history shows the list of previously executed commands.
Besides the cursor control keys, the following keyboard shortcuts are
available:

  Re-execute a command                                          #Enter#

  Re-execute a command in a new window                    #Shift-Enter#

  Re-execute a command as administrator                #Ctrl-Alt-Enter#

  Copy a command to the command line                       #Ctrl-Enter#

  Clear the commands history                                      #Del#

  Delete the current history item                           #Shift-Del#

  Lock/unlock a history item                                      #Ins#

  Copy the text of the current command to the clipboard        #Ctrl-C#
  without closing the list                                or #Ctrl-Ins#

  Toggle history view:                                         #Ctrl-T#
             * with date lines + time-path column
             * with date lines (as in far3)
             * plain history (as in far2)

  Change path width in time-path column          #Ctrl-Left,Ctrl-Right#

  Show additional information                                      #F3#

  Quick jump in panel to directory of command                #Ctrl-F10#

    To go to the previous or next command directly from the command line, you
can press #Ctrl-E# or #Ctrl-X# respectively.

    For choosing a command, besides the cursor control keys and #Enter#, you can
use the highlighted shortcut letters.

    If you want to save the commands history after exiting FAR2L, use the
respective option in the ~system settings dialog~@SystemSettings@.

    Locked history items will not be deleted when the history is cleared.

    Remove duplicates method can be chosen in the ~system settings dialog~@SystemSettings@.

    For automatic exclusion from history, see ~dialog AutoComplete & History~@AutoCompleteSettings@.

    See also: common ~menu~@MenuCmd@ keyboard commands.
              common ~history~@HistoryCmd@ keyboard commands.

@HistoryViews
$ #History: file view and edit#
    The file view history shows the list of files that have been recently
viewed or edited. Besides the cursor control keys, the following keyboard
shortcuts are available:

  Reopen a file for viewing or editing                          #Enter#

  Copy the file name to the command line                   #Ctrl-Enter#

  Clear the history list                                          #Del#

  Delete the current history item                           #Shift-Del#

  Lock/unlock a history item                                      #Ins#

  Refresh list and remove non-existing entries                 #Ctrl-R#

  Copy the text of the current history item to the             #Ctrl-C#
  clipboard without closing the list                      or #Ctrl-Ins#

  Open a file in the ~editor~@Editor@                                        #F4#

  Open a file in the ~viewer~@Viewer@                                        #F3#
                                                          or #Numpad 5#

  Toggle history view:                                         #Ctrl-T#
             * with date lines + time column
             * with date lines (as in far3)
             * plain history (as in far2)

  Quick jump in panel to directory and file                  #Ctrl-F10#

    For choosing a history item, besides the cursor control keys and #Enter#,
you can use the highlighted shortcut letters.

    Items of the view and edit history are moved to the end of the list after
they are selected. You can use #Shift-Enter# to select an item without changing
its position.

    If you want to save the view and edit history after exiting FAR2L, use the
respective option in the ~system settings dialog~@SystemSettings@.

  Remarks:

  1. ^<wrap>List refresh operation (#Ctrl-R#) can take a considerable amount
of time if a file was located on a currently unavailable remote resource.

  2. ^<wrap>Заблокированные пункты не будут удаляться при очистке или обновлении истории.

    See also: common ~menu~@MenuCmd@ keyboard commands.
              common ~history~@HistoryCmd@ keyboard commands.

@HistoryFolders
$ #History: folders#
    The folders history shows the list of folders that have been recently
visited. Besides the cursor control keys, the following keyboard shortcuts are
available:

  Go to the current folder in the list                          #Enter#

  Open the selected folder in the passive panel      #Ctrl-Shift-Enter#

  Copy the folder name to the command line                 #Ctrl-Enter#

  Clear the history list                                          #Del#

  Delete the current history item                           #Shift-Del#

  Lock/unlock a history item                                      #Ins#

  Refresh list and remove non-existing entries                 #Ctrl-R#

  Copy the text of the current history item to the             #Ctrl-C#
  clipboard without closing the list                      or #Ctrl-Ins#

  Toggle history view:                                         #Ctrl-T#
             * with date lines + time column
             * with date lines (as in far3)
             * plain history (as in far2)

  Quick jump in panel to directory (here #Enter# analog)       #Ctrl-F10#

    For choosing a history item, besides the cursor control keys and #Enter#,
you can use the highlighted shortcut letters.

    Items of the folders history are moved to the end of the list after they
are selected. You can use #Shift-Enter# to select an item without changing its
position.

    If you want to save the folders history after exiting FAR2L, use the
respective option in the ~system settings dialog~@SystemSettings@.

  Remarks:

  1. ^<wrap>List refresh operation (#Ctrl-R#) can take a considerable amount
of time if a folder was located on a currently unavailable remote resource.

  2. ^<wrap>Заблокированные пункты не будут удаляться при очистке или обновлении истории.

    See also: common ~menu~@MenuCmd@ keyboard commands.
              common ~history~@HistoryCmd@ keyboard commands.

@TaskList
$ #Task list#
    The task list displays active tasks by using #htop# (if available).

@HotPlugList
$ #Hotplug devices list#
    Hotplug devices list displays PC Card boards and other analogue devices
which are installed and work in the computer.

    To remove a device you need to select it in the list and press the #Del#
key. After that OS will prepare the device for safe removal and a
notification will be displayed when it is safe to remove the device.

    #Ctrl-R# allows to refresh the list of connected devices.

    See also: common ~menu~@MenuCmd@ keyboard commands.

@CompFolders
$ #Compare folders#
    The compare folders command is applicable only when two ~file panels~@FilePanel@
are displayed. It compares the contents of folders displayed in the two panels.
Files existing in one panel only, or those which have a date more recent than
files with the same name in the other panel, become marked.

    Subfolders are not compared. Files are compared only by name, size and
time, and file contents have no effect on the operation.

    See option #Case sensitive when compare or select# in ~Panel settings~@PanelSettings@.


@UserMenu
$ #User menu#
    The user menu exists to facilitate calls of frequently used operations. It
contains a number of user defined commands and command sequences, which may be
executed when invoking the user menu. The menu may contain submenus.
~Special symbols~@MetaSymbols@ are supported both in the commands and in the command
titles. Note, that !?<title>?<init>! symbol may be used to enter additional
parameters directly before executing commands.

    You may reorder menu items by pressing #Ctrl-Up# and #Ctrl-Down#.

    With the #Edit user menu# command from the ~Commands menu~@CmdMenu@, you
may edit or create main or local user menu. There may only be one main user
menu. The main user menu is called if no local menu for the current folder is
available. The local menu may be placed in any folder. You may switch between
the main menu and the user menu by pressing #Shift-F2#. Also you may call the
user menu of the parent folder by pressing #BS#.

    You may add command separators to the user menu. To do this, you should add
a new menu command and define "#--#" as "hot key". To delete a menu separator, you
must switch to file mode with #Ctrl-F4# key.

    To execute a user menu command, select it with cursor keys and press #Enter#.
You may also press the hot key assigned to the required menu item.

    You may delete a submenu or menu item with the #Del# key, insert new
submenu or menu item with the #Ins# key or edit an existing submenu or menu
item with the #F4# key. Press #Ctrl-F4# to edit the menu in text file form.

    It is possible to use digits, letters and function keys (#F1#..#F24#) as
hot keys in user menu. If #F1# or #F4# is used, its original function in user
menu is overridden. However, you still may use #Shift-F4# to edit the menu.

    When you edit or create a menu item, you should enter the hot key for fast
item access, the item title which will be displayed in the menu and the command
sequence to execute when this item will be selected.

    When you edit or create a submenu, you should enter the hot key and the
item title only.

    Local user menus are stored in the text files #FarMenu.Ini#.
    The main menu is stored in profile in #~~/.config/far2l/settings/user_menu.ini#
(the format is different from FarMenu.ini).
If you create a local menu in the FAR2L folder, it will be used instead of
the main menu saved in the profile.

    To close the menu even if submenus are open use #Shift-F10#.

    See also:
      ~Special commands~@SpecCmd@.
      The list of ~macro keys~@KeyMacroUserMenuList@, available in the user menu.
      Common ~menu~@MenuCmd@ keyboard commands.

@FileAssoc
$ #File associations #
    FAR2L supports file associations, that allow to associate various
actions to running, viewing and editing files with a specified
~mask~@FileMasks@.

    You may add new associations with the #Edit associations# command in the
~Commands menu~@CmdMenu@.

    You may define several associations for one file type and select the
desired association from the menu.

    The following actions are available in the associations list:

    #Ins#        - ~add~@FileAssocModify@ a new association

    #F4#         - ~edit~@FileAssocModify@ the current association

    #Del#        - delete the current association

    #Ctrl-Up#    - move association up

    #Ctrl-Down#  - move association down

    If no execute command is associated with file and
#Use OS registered types# option in ~System settings~@SystemSettings@
is on, FAR2L tries to use OS association to execute this file type;

    See also:
      ~Special commands~@SpecCmd@.
      common ~menu~@MenuCmd@ keyboard commands.


@FileAssocModify
$ #File associations: editing#
    FAR2L allows to specify six commands associated with each file type specified
as a ~mask~@FileMasks@:

   #Execute command#               Performed if #Enter# is pressed
   #(used for Enter)#

   #Execute command#               Performed if #Ctrl-PgDn# is pressed
   #(used for Ctrl-PgDn)#

   #View command#                  Performed if #F3# is pressed
   #(used for F3)#

   #View command#                  Performed if #Alt-F3# is pressed
   #(used for Alt-F3)#

   #Edit command#                  Performed if #F4# is pressed
   #(used for F4)#

   #Edit command#                  Performed if #Alt-F4# is pressed
   #(used for Alt-F4)#

    The association can be described in the #Description of the association#
field.

    If you do not wish to switch panels off before executing the associated
program, start its command line with '#@@#' character.

    The following ~special symbols~@MetaSymbols@ may be used in the associated
command.

  Notes:

  1. ^<wrap>If no execute command is associated with file and
#Use OS registered types# option in ~System settings~@SystemSettings@
is on, FAR2L tries to use OS association to execute this file type;

  2. ^<wrap>Operating system ~commands~@OSCommands@ "IF EXIST" and "IF DEFINED"
allow to configure "smarter" associations - if you have specified several
associations for a file type, the menu will show only the associations
for which the conditions are true.


@MetaSymbols
$ #Special symbols#
    The following special symbols can be used in ~associated commands~@FileAssoc@,
~user menu~@UserMenu@ and the ~apply command~@ApplyCmd@:

    #!!#          '!' character
    #!#           File name without extension
    #!`#          Extension without file name (ext)
    #!.!#         File name with extension
    #!@@!# or #!$!#  Name of file with selected file names list
    #!&#          List of selected files
    #!/#  or #!\\#   Current path
    #!=/# or #!=\\#  Current path considering ~symbolic links~@HardSymLink@.

    #!?<title>?<init>!#
             This symbol is replaced by user input, when
             executing command. <title> and <init> - title
             and initial text of edit control.

             Several such symbols are allowed in the same line,
             for example:

             grep !?Search for:?! !?In:?*.*!|far2l -v -

             A history name for the <init> string can be supplied
             in the <title>. In such case the command has the
             following format:

             #!?$<history>$<title>?<init>!#

             for example:

             grep !?#$GrepHist$#Search for:?! !?In:?*.*!|far2l -v -

             In <title> and <init> the usage of other meta-symbols is
             allowed by enclosing them in brackets.

             (e.g. grep !?Find in (!.!):?! |far2l -v -)

    #!###       "!##" modifier specified before a file association
             symbol forces it (and all the following characters)
             to refer to the passive panel (see note 4).
             For example, !##!.! denotes the name of the current
             file on the passive panel.

    #!^#       "!^" modifier specified before a file association
             symbol forces it (and all the following characters)
             to refer to the active panel (see note 4).
             For example, !^!.! denotes a current file name on
             the active panel, !##!\\!^!.! - a file on the passive
             panel with the same name as the name of the current
             file on the active panel.

  Notes:

    1. ^<wrap>When handling special characters, FAR2L substitutes only the string
corresponding to the special character. No additional characters (for example,
quotes) are added, and you should add them yourself if it is needed. For
example, if a program used in the associations requires a file name to be
enclosed in quotes, you should specify #program "!.!"# and not
#program !.!#.

    2. ^<wrap>The following modifiers may be used with the associations !@@! and !$! :

     'Q' - enclose names containing spaces in quotes;
     'S' - use '/' instead of '\\' in pathnames;
     'F' - use full pathnames;
     'A' - use ANSI code page;
     'U' - use UTF-8 code page;
     'W' - use UTF-16 (Little endian) code page.

    For example, the association #!@@AFQ!# means "name of file with the list of
selected file names, in ANSI encoding, include full pathnames, names with
spaces will be in quotes".

    3. ^<wrap>When there are multiple associations specified, the meta-characters !@@!
and !$! are shown in the menu as is. Those characters are translated when the
command is executed.

    4. ^<wrap>The prefixes "!##" and "!^" work as toggles for associations. The effect
of these prefixes continues up to the next similar prefix. For example:

    if exist !##!\\!^!.! diff -c -p !##!\\!^!.! !\\!.!

  "If the same file exists on the passive panel as the file under
   the cursor on the active panel, show the differences between
   the file on the passive panel and the file on the active panel,
   regardless of the name of the current file on the passive panel"

    5. ^<wrap>If it is needed to pass to a program a name with an ending
backslash, use the following meta-symbol - #!.\#. For example, to
extract a rar archive to a folder with the same name

    winrar x "!.!" "!.\"

@SystemSettings
$ #Settings dialog: system#
  #Clear R/O attribute from CD files#
  Clear read-only attribute from files copied from CD.

  #Delete to Recycle Bin#
  Enables file deletion via the Recycle Bin.The operation of deleting to the Recycle
Bin can be performed only for local hard disks.

  #Delete symbolic links#
  Scan for and delete symbolic links to subfolders before deleting to Recycle Bin.

  #Use system copy routine#
  Use the file copy functions provided by the operating system instead of internal
file copy implementation. It may be useful on NTFS, because the system function
(CopyFileEx) copies file extended attributes. On the other hand, when using the system
function, the possibility to split files when ~copying~@CopyFiles@ or moving is not available.

  #Copy files opened for writing#
  Allows to copy files that are opened by other programs for writing. This mode
is handy to copy a file opened for a long time, but it could be dangerous, if a file
is being modified at the same time as copying.

  #Scan symbolic links#
  Scan ~symbolic links~@HardSymLink@ along with normal sub-folders when building the folder tree,
determining the total file size in the sub-folders.


  #Create folders in uppercase#
  If the name of a new folder contains only lowercase letters and this option is on, the folder will be created in uppercase.

  #Inactivity time#
  Terminate FAR2L after a specified interval without keyboard or mouse activity. This works only if FAR2L waits for command line
input without viewer or editor screens in the background.

  #Save commands history#
  Forces saving ~commands history~@History@ before exit and restoring after starting FAR2L.

  #Save folders history#
  Forces saving ~folders history~@HistoryFolders@ before exit and restoring after starting FAR2L.
Folders history list may be activated by #Alt-F12#.

  #Save view and edit history#
  Forces saving ~history of viewed and edited~@HistoryViews@ files before exit and restoring it after
starting FAR2L. View and edit history list may be activated by #Alt-F11#.

  #Use OS registered types#
  When this option is on and #Enter# is pressed on a file, the type of which is known to
OS and absent in the list of FAR2L ~file associations~@FileAssoc@, the OS program
registered to process this file type will be executed.

  #CD drive auto mount#
  When a CD-ROM drive is selected from the ~location menu~@DriveDlg@, FAR2L will close the open
tray of a CD drive. Turn off this option if automatic CD-ROM mounting does not work
correctly (this can happen because of bugs in the drivers of some CD-ROM drives).

  #Auto save setup#
  If checked, FAR2L will save setup automatically. The current folders for both panels will be also saved.


@PanelSettings
$ #Settings dialog: panel#
  #Show hidden and#         Enable to show files with Hidden
  #system files#            and System attributes. This option may
                          also be switched by #Ctrl-H#.

  #Highlight files#         Enable ~files highlighting~@Highlight@.

  #Show file name markings# and #Align file names by markings#
works only if #Highlight files# enabled
and may be switched by #Ctrl-Alt-M#.

  #Auto change folder#      If checked, cursor moves in the ~tree panel~@TreePanel@
                          will cause a folder change in the other
                          panel. If it is not checked, you must press
                          #Enter# to change the folder from the tree
                          panel.

  #Select folders#          Enable to select folders, using #Gray +#
                          and #Gray *#. Otherwise these keys will
                          select files only.

  #Case sensitive when#     Influence on ~Compare folders~@CompFolders@
  #compare or select#       and ~Selecting files~@SelectFiles@.

  #Sort folder names#       Apply sorting by extension not only
  #by extension#            to files, but also to folders.
                          When the option is turned on, sorting
                          by extension works the same as it did
                          in FAR2L 1.65. If the option is turned
                          off, in the extension sort mode the
                          folders will be sorted by name, as
                          they are in the name sort mode.

  #Allow reverse#           If this option is on and the current file
  #sort modes#              panel sort mode is reselected, reverse
                          sort mode will be set.

  #Disable automatic#       The mechanism for automatically updating
  #update of panels#        the panels when the state of the file
                          system changes will be disabled if the
                          count of file objects exceeds the
                          specified value.

    The auto-update mechanism works only for FAT/FAT32/NTFS file
    systems. The value of 0 means "update always". To force an
    update of the panels, press #Ctrl-R#.

  #Network drives#          This option enables panel autorefresh
  #autorefresh#             when state of filesystem on network
                          drives is being changed. It can be useful
                          to disable this option on slow network
                          connections

  #Show column titles#      Enable display of ~file panel~@FilePanel@ column titles.

  #Show status line#        Enable display of file panel status line.

  #Show total#              Enable display of total information data
  #information#             at the bottom line of file panel.

  #Show free size#          Enable display of the current disk free
                          size.

  #Show scrollbar#          Enable display of file and ~tree panel~@TreePanel@
  #in Panels#               scrollbars.

  #Show background#         Enable display of the number of
  #screens number#          ~background screens~@ScrSwitch@.

  #Show sort mode#          Indicate current sort mode in the
  #letter#                  upper left panel corner.


@InterfSettings
$ #Settings dialog: interface#
  #Clock in panels#
  Show clock at the right top corner of the screen.

  #Clock in viewer and editor#
  Show clock in viewer and editor.

  #Show key bar#
  Show the functional key bar at the bottom line.
This option also may be switched by #Ctrl-B#.

  #Always show menu bar#
  Show top screen menu bar even when it is inactive.

  #Screen saver#
  Run screen saver after the inactivity interval in minutes. When this option
is enabled, screen saver will also activate when mouse pointer is brought
to the upper right corner of FAR2L window.

  #Show total copy indicator#
  Show total progress bar, when performing a file copy operation.
This may require some additional time before starting copying
to calculate the total files size.

  #Show copying time information#
  Show information about average copying speed, copying time and
estimated remaining time in the copy dialog.

  Since this function requires some time to gather statistics, it is likely
that you won't see any information if you're copying many small files
and the option "Show total copy progress indicator" is disabled.

  #Show total delete indicator#
  Show total progress bar, when performing a file delete operation.
This may require some additional time before starting deleting
to calculate the total files count.

  #Use Ctrl-PgUp for location menu#
  Pressing #Ctrl-PgUp# in the root directory:
  - for local drives - shows the location menu;
  - for network drives - activates the Network plugin (if it is available)
or location menu (if the Network plugin is not available).

  #FAR window title#
  Information displayed in the console window title. Can contain any text
including the following variables:
  - #%State# - current FAR2L state (previously was a default window title);
  - #%Ver# - FAR2L version;
  - #%Platform# - FAR2L platform architecture;
  - #%Backend# - FAR2L UI backend;
  - #%Host# - host name of the machine where FAR2L is running;
  - #%User# - user name under wich FAR2L is running;
  - #%Admin# - name "Root", if FAR2L runs under root privileges, otherwise - empty string.

@InputSettings
$ #Settings dialog: input#
  #Mouse#
  Use the mouse.

  #Transliteration ruleset:#
  Choose here transliteration ruleset that corresponds to your usual keyboard layout.
Avalable rulesets loaded from file #xlats.ini# that defines how non-latin keys map
to latin and vice-verse, that subsequentially used in #fast file find by Alt+FILENAME#,
#dialog hotkeys navigation# and some internal functionality.

  #Exclusively handle hotkeys that include#
  This options allows to choose control keys using which in hotkey combination
will cause FAR2L to capture keyboard input exclusively, thus preventing other
application from interfering with FAR2L hotkeys that contains such control key.
Note that this options works only in GUI backend mode.


@DialogSettings
$ #Settings dialog: dialogs#
  #History in dialog#       Keep history in edit controls of some
  #edit controls#           FAR2L dialogs. The previous strings history
                          list may be activated by mouse or using
                          #Ctrl-Up# and #Ctrl-Down#. If you do not wish
                          to track such history, for example due to
                          security reasons, switch this option off.

  #Persistent blocks#       Do not remove block selection after moving
  #in edit controls#        the cursor in dialog edit controls and
                          command line.

  #Del removes blocks#      If a block is selected, pressing Del will
  #in edit controls#        not remove the character under cursor, but
                          this block.

  #AutoComplete#            Allows to use the AutoComplete function
  #in edit controls#        in edit controls that have a history list
                          or in combo boxes. When the option is
                          disabled, you may use the #Ctrl-End# key
                          to autocomplete a line. The autocomplete
                          feature is disabled while a macro is
                          being recorded or executed.

  #Backspace deletes#       If the option is on, pressing #BackSpace#
  #unchanged text#          in an unchanged edit string deletes
                          the entire text, as if #Del# had been
                          pressed.

  #Mouse click outside#     #Right/left mouse click# outside a dialog
  #a dialog closes it#      closes the dialog (see ~Miscellaneous~@MiscCmd@).
                          This option allows to switch off this
                          functionality.

   See also the list of ~macro keys~@KeyMacroDialogList@, available in dialogs.

@VMenuSettings
$ #Menu settings#
  #Left/Right/Middle mouse click outside a menu#
  You may choose action for mouse buttons, when click occures outside a menu:
  #Cancel menu#, #Execute selected item# or #Do nothing#.

@CmdlineSettings
$ #Settings dialog: command line#
  #Persistent blocks#
  Do not remove block selection after moving the cursor in command line.

  #Del removes blocks#
  If a block is selected, pressing Del will not remove the character under cursor,
but this block.

  #AutoComplete#
  Allows to use the AutoComplete function in command line. When the option is
disabled, you may use the #Ctrl-Shift-End# key to autocomplete a line. The autocomplete
feature is disabled while a macro is being recorded or executed.

  #Set command line prompt format#
  This option allows to set the default FAR2L command ~line prompt~@CommandPrompt@.

@AutoCompleteSettings
$ #Settings dialog: AutoComplete & History#
  #Show list#
  Show list with autocomplete suggestions.

  #Modal mode#
  in mode #[x] Modal mode# selected list item put in command line only after #Enter#,
  in mode #[ ] Modal mode# selected list item put in command line immediately.

  #Append first matched item#
  The first matched item is append immediately after symbols in the command line.

  #Exceptions wildcards# also affect which commands are stored in far2l history.
  For example, adding #" *"# (mandatory in quotes) excludes from adding in history
  commands that start with a space (similar to the bash #$HISTCONTROL=ignorespace#).
  Info: in far2l history work like bash #$HISTCONTROL#
   with options #ignoredups# (lines which match the previous line are not saved)
   and #erasedups# (all previous lines matching the current line are removed from the history).

@InfoPanelSettings
$ #Настройка информационной панели#
  #Формат вывода имени компьютера#

  Задает формат вывода имени компьютера. Может быть одним из:

    #Physical NetBIOS#
      ^<wrap>NetBIOS имя локального компьютера.

    #Physical DNS hostname#
      ^<wrap>DNS имя локального компьютера.

    #Physical DNS domain#
      ^<wrap>DNS имя домена, которому принадлежит компьютер.

    #Physical DNS fully-qualified#
      ^<wrap>Полное доменное имя, которое однозначно идентифицирует этот компьютер. Имя представляет собой комбинацию DNS имени хоста и DNS имени домена (формат HostName.DomainName).

    #NetBIOS#
      ^<wrap>NetBIOS имя локального компьютера. Если компьютер является узлом кластера, то выводится NetBIOS имя кластера виртуального сервера.

    #DNS hostname#
      ^<wrap>DNS имя локального компьютера. Если компьютер является узлом кластера, то выводится DNS имя кластера виртуального сервера.

    #DNS domain#
      ^<wrap>DNS имя домена, которому принадлежит компьютер. Если компьютер является узлом кластера, то выводится DNS имя домена кластера виртуального сервера.

    #DNS fully-qualified#
      ^<wrap>Полное доменное имя, которое однозначно идентифицирует этот компьютер. Имя представляет собой комбинацию DNS имени хоста и DNS имени домена (формат HostName.DomainName). Если компьютер является узлом кластера, то выводится полное DNS имя кластера виртуального сервера.

    Формат вывода зависит от наличия доменной структуры, групповых политик, настроек DNS.

  #Формат вывода имени пользователя#

  Задает формат вывода имени пользователя. Может быть одним из:

    #По умолчанию#
      Логин пользователя, например, JohnDoe
    #Полностью определенное имя домена#
      ^<wrap>FQDN (Fully Qualified Domain Name) - включает наименования всех уровней иерархии, начиная от имени конечного узла и заканчивая корневой точкой (логином), например,
      CN=JohnDoe, OU=Software, OU=Engineering, O=Widget, C=US
    #Sam Compatible#
      Engineering\JohnDoe
    #Display#
      Probably "John Doe" but could be something else.  I.e. The display name is not necessarily the defining RDN.
    #Уникальный идентификатор#
      ^<wrap>Уникальный идентификатор имени пользователя (GUID), например,
      {4fa050f0-f561-11cf-bdd9-00aa003a77b6}
    #Канонический вид#
      ^<wrap>ADCN (Active Directory Canonical Name) - данный формат является путем в иерархической структуре к объекту (логину), например,
      engineering.widget.com/software/JohnDoe
    #Основное имя пользователя#
      ^<wrap>UPN (User Principal Name) - известен так же как адрес электронной почты, например,
      someone@example.com
    #Service Principal#
      www/srv.engineering.com/engineering.com
    #Dns Domain#
      engineering.widget.com\JohnDoe

    Формат вывода зависит от наличия доменной структуры.

@CommandPrompt
$ #Command line prompt format#
   FAR2L allows to change the command line prompt format.
To change it you have to enter the needed sequence of variables and
special code words in the #Set command line prompt format# input field
of the ~Command line settings~@CmdlineSettings@ dialog, this will allow showing
additional information in the command prompt.

   It is allowed to use environment variables and the following special
code words:

     $a - the & character
     $b - the | character
     $c - the ( character
     $d - current date (depends on system settings)
     $f - the ) character
     $g - the > character
     $h - delete the previous character
     $l - the < character
     $## - ## character if user is root, otherwise $
     $p - current drive and path, possible abbreviated
     $r - current drive and path, not abbreviated
     $u - login user name
     $n - computer name
     $q - the = character
     $s - space
     $t - current time in HH:MM:SS format
     $$ - the $ character
     $+ - глубина стека каталогов

     $@@xx - ^<wrap>"Администратор", если FAR2L запущен от имени администратора.
Вместо 'xx' необходимо указать два символа, которые будут обрамлять слово "Администратор".

   Examples.

   1. ^<wrap>A prompt of the following format #[%HOSTNAME%]$S$P$G#
will contain the computer name, current drive and path

   2. ^<wrap>A prompt of the following format #[$T$H$H$H]$S$P$G# will
display the current time in HH:MM format before the current
drive and path

   3. ^<wrap>Code "$+" displays the number of pluses (+) needed according to
current ~PUSHD~@OSCommands@ directory stack depth, one character per each
saved path.

@Viewer
$ #Viewer: control keys#
   Viewer commands

    #Left#               Character left
    #Right#              Character right
    #Up#                 Line up
    #Down#               Line down
    #Ctrl-Left#          20 characters left
                       In Hex-mode - 1 position left
    #Ctrl-Right#         20 characters right
                       In Hex-mode - 1 position right
    #PgUp#               Page up
    #PgDn#               Page down
    #Alt+PgUp#           Page up with increasing speed
    #Alt+PgDn#           Page down with increasing speed
    #Ctrl-Shift-Left#    Start of lines on the screen
    #Ctrl-Shift-Right#   End of lines on the screen
    #Home, Ctrl-Home#    Start of file
    #End, Ctrl-End#      End of file
    #Shift-Left/Right#   Grow existing text selection left/right

    #F1#                 Help
    #F2#                 Toggle line wrap/unwrap
    #Shift-F2#           Toggle wrap type (letters/words)
    #F4#                 Toggle text/hex mode
     (hex mode does not support UTF-8 and other multibyte code pages
      and switches the view to a single-byte code page)
    #F5#                 Toggle raw/processed mode
    #F6#                 Switch to ~editor~@Editor@
    #Alt-F5#             Print the file
                       ("Print manager" plugin is used).
    #F7#                 ~Search~@ViewerSearch@
    #Shift-F7, Space#    Continue search
    #Alt-F7#             Continue search in "reverse" mode
    #Ctrl-F7#            ~Grep filter~@GrepFilter@
    #F8#                 Toggle UTF8/~ANSI/OEM~@CodePagesSet@ code page
    #Shift-F8#           Select code page
    #Alt-F8#             ~Change current position~@ViewerGotoPos@
    #Alt-F9#             Toggles the size of the FAR2L console window
    #F9,Alt-Shift-F9#    Call ~Viewer settings~@EditorSettings@ dialog
    #Numpad5,F3,F10,Esc# Quit
    #Ctrl-F10#           Position to the current file.
    #F11#                Call "~Plugin commands~@Plugins@" menu
    #Alt-F11#            Display ~view history~@HistoryViews@
    #+#                  Go to next file
    #-#                  Go to previous file
    #Ctrl-O#             Show user screen
    #Ctrl-Alt-Shift#     Temporarily show user screen
                       (as long as these keys are held down)
    #Ctrl-B#             Show/Hide functional key bar at the bottom
                       line.
    #Ctrl-Shift-B#       Show/Hide status line
    #Ctrl-S#             Show/Hide the scrollbar.
    #Alt-BS, Ctrl-Z#     Undo position change
    #RightCtrl-0..9#     Set a bookmark 0..9 at the current position
    #Ctrl-Shift-0..9#    Set a bookmark 0..9 at the current position
    #LeftCtrl-0..9#      Go to bookmark 0..9

    #Ctrl-Ins, Ctrl-C#   Copy the text highlighted as a result of
                       the search to the clipboard.
    #Ctrl-U#             Remove the highlighting of the search results.

    See also the list of ~macro keys~@KeyMacroViewerList@, available in the viewer.

    Notes:

    1. Also to call search dialog you may just start typing the
       text to be located.

    2. !! Windows legacy (not relevant on Linux/*BSD/Mac) !!

       When the viewer opens a file, it permits the file to be
       deleted by other processes. If such a deletion happens,
       the file is actually deleted from the directory only after
       the viewer is closed, but any operations on the deleted
       file fail - this is a Windows feature.

       !! Windows legacy end !!

    3. The current version of FAR2L has a limitation on the maximum
       number of columns in the internal viewer - the number
       cannot exceed 2048. If a file contains a line that does not
       fit in this number of columns, it will be split into several
       lines, even if the word wrap mode is turned off.

    4. FAR2L ~searches~@ViewerSearch@ the first occurrence of the string (#F7#) from
       the beginning of the area currently displayed.

    5. For automatic scrolling of a dynamically updating file,
       position the "cursor" to the end of the file (End key).

    6. Pressing Alt+PgUp/PgDn smoothly increases scrolling speed, futher releasing
       Alt while keeping PgUp/PgDn will continue scrolling with selected speed boost.
       Speed boost dismissed by releasing all keys for long time or pressing any other key.

@GrepFilter
    Here user may temporarily filter currently viewed file content using #UNIX grep# tool pattern matching.
    You may specify pattern to match (or several patterns separated by #\|# - as in usual grep) and/or
pattern that will be excluded from output.
    Optionally it's possible to see specified #lines amount before or after matched region#, as well as
use #case sensitive# matching or match #whole words# instead of plain substring.

@ViewerGotoPos
$ #Viewer: go to specified position#
    This dialog allows to change the position in the internal viewer.

    You can enter decimal offset, percent, or hexadecimal offset. The meaning
of the value you enter is defined either by the radio buttons in the dialog or
by format specifiers added before or after the value.

    You can also enter relative values, just add + or - before the number.

    Hexadecimal offsets must be specified in one of the following formats:
       0xNNNN, NNNNh, $NNNN

    Decimal offsets (not percentages) must be specified in the format NNNNd.

  Examples
   #50%#                     Go to middle of file (50%)
   #-10%#                    Go to 10% percent back from current offset
                           If the old position was 50%, the new
                           position will be 40%
   #0x100#                   Go to offset 0x100 (256)
   #+0x300#                  Go 0x300 (768) bytes forward

  If you enter the value with one of the format specifiers (%, '0x', 'h',
'$', 'd'), the radio buttons selected in the dialog will be ignored.


@ViewerSearch
$ #Viewer: search#
    For searching in the ~viewer~@Viewer@, the following modes and options are
available:

    #Search for text#

      Search for any text entered in the #Search for# edit line.
      The following options are available in that mode:

        #Case sensitive#      - ^<wrap>the case of the characters entered will be taken into account
while searching (so, for example, #Text# will not be found when searching for #text#).

        #Whole words#         - ^<wrap>the given text will be found only if it occurs in the text as a whole word.

        #Regular expressions# - ^<wrap>enable the use of ~regular expressions~@RegExp@ in the search string.

    #Search for hex#

      ^<wrap>Search for a string corresponding to hexadecimal codes entered in the #Search for# string.

    #Reverse search#

      ^<wrap>Reverse the search direction - search from the end of file towards the beginning.


@Editor
$ #Editor#
    To edit the file currently under the cursor you should press #F4#. This
can be used to open the internal editor or any of the user defined external
editors which are defined in the ~Editor settings~@EditorSettings@ dialog.

    #Creating files using the editor#

    If a nonexistent file name is entered after pressing the #Shift-F4# hotkey
then a ~new file~@FileOpenCreate@ will be created.

    Remarks:

    1. ^<wrap>If a name of a nonexistent folder is entered when creating a new file
then a "~Path to the file to edit does not exist~@WarnEditorPath@" warning
will be shown.

    2. ^<wrap>When trying to reload a file already opened in the editor the
"~reloading a file~@EditorReload@" warning message will be shown.

    3. ^<wrap>The UTF-8 encoding is used by default when creating new files, this
behavior can be changed in the ~Editor settings~@EditorSettings@ dialog.

  #Control keys#

  Cursor movement

   #Left#                    Character left
   #Ctrl-S#                  ^<wrap>Move the cursor one character to the left, but don't move to the previous line if the line beginning is reached.
   #Right#                   Character right
   #Up#                      Line up
   #Down#                    Line down
   #Ctrl-Left#               Word left
   #Ctrl-Right#              Word right
   #Ctrl-Up#                 Scroll screen up
   #Ctrl-Down#               Scroll screen down
   #PgUp#                    Page up
   #PgDn#                    Page down
   #Home#                    Start of line
   #End#                     End of line
   #Ctrl-Home, Ctrl-PgUp#    Start of file
   #Ctrl-End, Ctrl-PgDn#     End of file
   #Ctrl-N#                  Start of screen
   #Ctrl-E#                  End of screen

  Delete operations

   #Del#                     ^<wrap>Delete char (also may delete block, depending upon ~Editor settings~@EditorSettings@).
   #BS#                      Delete char left
   #Ctrl-Y#                  Delete line
   #Ctrl-K#                  Delete to end of line
   #Ctrl-BS#                 Delete word left
   #Ctrl-T, Ctrl-Del#        Delete word right

  Block operations

   #Shift-Cursor keys#       Select block
   #Ctrl-Shift-Cursor keys#  Select block by words
   #Alt-Cursor keys#         Select vertical block
   #Alt-Shift-Cursor keys#   Select vertical block (use NumLock cursor keys)
   #Ctrl-A#                  Select all text
   #Ctrl-U#                  Deselect block
   #Shift-Ins, Ctrl-V#       Paste block from clipboard
   #Shift-Del, Ctrl-X#       Cut block
   #Ctrl-Ins, Ctrl-C#        Copy block to clipboard
   #Ctrl-<Gray +>#           Append block to clipboard
   #Ctrl-D#                  Delete block
   #Ctrl-P#                  ^<wrap>Copy block to current cursor position (in persistent blocks mode only)
   #Ctrl-M#                  ^<wrap>Move block to current cursor position (in persistent blocks mode only)
   #Alt-U#                   Shift block left
   #Alt-I#                   Shift block right
   #Shift-Tab#               Shift block left by Tab or by indent size (processed by SimpleIndent plugin)
   #Tab#                     Shift block right by Tab or by indent size (processed by SimpleIndent plugin)

  Other operations

   #F1#                      Help
   #F2#                      Save file
   #Shift-F2#                ~Save file as...~@FileSaveAs@
   #Shift-F4#                Edit ~new file~@FileOpenCreate@
   #F5#                      Toggle whitespace characters displaying
   #Shift-F5#                Change Tab character width
   #Ctrl-F5#                 Toggle Tab-to-spaces expansion
   #Alt-F5#                  ^<wrap>Print file or selected block ("Print manager" plugin is used).
   #F6#                      Switch to ~viewer~@Viewer@
   #F7#                      ~Search~@EditorSearch@
   #Ctrl-F7#                 ~Replace~@EditorSearch@
   #Shift-F7#                Continue search/replace
   #Alt-F7#                  Continue search/replace in "reverse" mode
   #F8#                      Toggle UTF8/~ANSI/OEM~@CodePagesSet@ code page
   #Shift-F8#                Select code page
   #Alt-F8#                  ~Go to~@EditorGotoPos@ specified line and column
   #Alt-F9#                  Toggles the size of the FAR2L console window
   #F9, Alt-Shift-F9#        Call ~Editor settings~@EditorSettings@ dialog
   #F10, F4, Esc#            Quit
   #Shift-F10#               Save and quit
   #Ctrl-F10#                Position to the current file
   #F11#                     Call "~Plugin commands~@Plugins@" menu
   #Alt-F11#                 Display ~edit history~@HistoryViews@
   #Alt-BS, Ctrl-Z#          Undo
   #Ctrl-Shift-Z#            Redo
   #Ctrl-L#                  Disable edited text modification
   #Ctrl-O#                  Show user screen
   #Ctrl-Alt-Shift#          ^<wrap>Temporarily show user screen (as long as these keys are held down)
   #Ctrl-Q#                  ^<wrap>Treat the next key combination as a character code
   #RightCtrl-0..9#          ^<wrap>Set a bookmark 0..9 at the current position
   #Ctrl-Shift-0..9#         ^<wrap>Set a bookmark 0..9 at the current position
   #LeftCtrl-0..9#           Go to bookmark 0..9
   #Shift-Enter#             ^<wrap>Insert the name of the current file on the active panel at the cursor position.
   #Ctrl-Shift-Enter#        ^<wrap>Insert the name of the current file on the passive panel at the cursor position.
   #Ctrl-F#                  ^<wrap>Insert the full name of the file being edited at the cursor position.
   #Ctrl-B#                  ^<wrap>Show/Hide functional key bar at the bottom line.
   #Ctrl-Shift-B#            Show/Hide status line

   See also the list of ~macro keys~@KeyMacroEditList@, available in the editor.

    Notes:

    1. #Alt-U#/#Alt-I# indent the current line if no block is selected.

    2. ^<wrap>Holding down #Alt# and typing a character code on the numeric
keypad inserts the character that has the specified code (0-65535).

    3. ^<wrap>If no block is selected, #Ctrl-Ins#/#Ctrl-C# marks the current
line as a block and copies it to the clipboard.


@EditorSearch
$ #Editor: search/replace#
    The following options are available for search and replace in ~editor~@Editor@:

      #Case sensitive#      - ^<wrap>the case of the characters entered will be taken into account while searching (so, for example,
#Text# will not be found when searching for #text#).

      #Whole words#         - the given text will be found only if it occurs in the text as a whole word.

      #Reverse search#      - ^<wrap>change the direction of search (from the end of file towards the beginning)

      #Regular expressions# - ^<wrap>treat input as Perl regular expression (~search~@RegExp@ and ~replace~@RegExpRepl@)

    The following option is available in search dialog only:

      #Select found#        - ^<wrap>found text is selected


@FileOpenCreate
$ #Editor: Open/Create file#
    With #Shift-F4#, one can open the existing file or create a new file.

    According to ~editor settings~@EditorSettings@, newly created file
is assigned to OEM or ANSI codepage. You can change the codepage with #Shift-F8#.

    For existing file, changing the codepage has sense if it hasn't been
correctly detected at open.


@FileSaveAs
$ #Editor: save file as...#
    To save edited file with another name press #Shift-F2# and specify
new name, codepage and carriage return symbols format.

    If file has been edited in one of the following codepages: UTF-8,
UTF-16 (Little endian) or UTF-16 (Big endian), then if the option #Add signature (BOM)# is on,
the appropriate marker is inserted into the beginning of the file, which
helps applications to identify the codepage of this file.

    You can also specify the format of the line break characters:

    #Do not change#
    Do not change the line break characters.

    #Dos/Windows format (CR LF)#
    Line breaks will be represented as a two-character sequence -
    Carriage Return and Line Feed (CR LF), as used in DOS/Windows.

    #Unix format (LF)#
    Line breaks will be represented as a single character - Line
    Feed (LF), as used in UNIX.

    #Mac format (CR)#
    Line breaks will be represented as a single character - Carriage
    Return (CR), as used in Mac OS.


@EditorGotoPos
$ #Editor: go to specified line and column#
    This dialog allows to change the position in the internal editor.

    You can enter a #row# or a #column#, or both.

    The first number will be interpreted as a row number, the second as a
column number. Numbers must be delimited by one of the following characters:
"," "." ";" ":" or space.

    If you enter the value in the form ",Col", the editor will jump to the
specified column in the current line.

    If you enter the row with "%" at the end, the editor will jump to the
specified percent of the file. For example, if you enter #50%#, the editor will
jump to the middle of the text.


@EditorReload
$ #Editor: reloading a file#
    FAR2L tracks all attempts to repeatedly open for editing a file that
is already being edited. The rules for reloading files are as follows:

    1. If the file was not changed and the option "Reload edited file" in the
~confirmations~@ConfirmDlg@ dialog is not enabled, FAR2L switches to the open
editor instance without further prompts.

    2. If the file was changed or the option "Reload edited file" is enabled,
there are three possible options:

    #Current#      - Continue editing the same file

    #New instance# - The file will be opened for editing in a new
                   editor instance. In this case, be attentive: the
                   contents of the file on the disk will correspond
                   to the contents of the editor instance where the
                   file was last saved.

    #Reload#       - The current changes are not saved and the
                   contents of the file on the disk is reloaded into
                   the editor.


@WarnEditorPath
$ #Warning: Path to the file to edit does not exist#
    When opening a new file for ~editing~@Editor@, you have entered the name of
a folder that does not exist. Before saving the file, FAR2L will create the
folder, provided that the path is correct and that you have enough rights
to create the folder.

@WarnEditorPluginName
$ #Warning: The name of the file to edit cannot be empty#
    To create a new file on a plugin's panel you must specify a
file name.

@WarnEditorSavedEx
$ #Warning: The file was changed by an external program#
    The write date and time of the file on the disk are not the same as
those saved by FAR2L when the file was last accessed. This means that another
program, another user (or even yourself in a different editor instance) changed
the contents of the file on the disk.

    If you press "Save", the file will be overwritten and all changes made by
the external program will be lost.

@CodePagesMenu
$ #Code pages menu#
    This menu allows codepage selection in the editor and viewer.

    The menu is divided into several parts:

    #Automatic detection# - Far tries to autodetect the codepage of the text;

    #System# - main 8-bit system codepages - ~ANSI and OEM~@CodePagesSet@;

    #Unicode# - Unicode codepages;

    #Favorites# - user controlled list of codepages;

    #Other# - the rest of codepages installed in the system.

    The menu has two modes: full mode with visible #Other# section and brief
mode with hidden #Other# section. The modes can be switched by pressing #Ctrl-H#.

    #Ins# keypress moves codepage from #Other# to #Favorites#, #Del# moves the
codepage back. Клавиша #F4# позволяет изменять отображаемые
имена для #любимых# и #прочих# кодовых страниц (кодовые страницы для которых было
изменено имя помечаются символом #*# перед именем).

    Диалог ~Изменение имени кодовой страницы~@EditCodePageNameDlg@

    See also: common ~menu~@MenuCmd@ keyboard commands.

@CodePagesSet
$ #ANSI and OEM codepage setting#
  Switchable by #F8# and #Shift-F8# OEM and ANSI code pages are defined based on the file
  #~~/.config/far2l/cp# (first line is #OEM#, second is #ANSI#)
  or, if its absence, by environment variable #LC_CTYPE#

@EditCodePageNameDlg
$ #Изменение имени кодовой страницы#
    Диалог #Изменение имени кодовой страницы# позволяет изменить отображаемое имя для
#любимых# и #прочих# кодовых страниц.

    Примечания:

    - ^<wrap>Если ввести пустое имя кодовой страницы, то после подтверждения ввода
отображаемое имя кодовой страницы примет значение по умолчанию, то есть имя полученное
от системы.
    - ^<wrap>Так же отображаемое имя кодовой страницы принимает значение по умолчанию
после нажатия кнопки #Сбросить#.

@DriveDlg
$ #Location menu#
    This menu allows to change the current location of a panel, unmount mountpoint 
or open a new ~plugin~@Plugins@ panel.

    Select the item and press Enter to change the location to specified filesystem path
or plugin. If the panel type is not a ~file panel~@FilePanel@, it will be changed to the
file panel if you chosen filesystem location, or selected Plugin panel will be opened.

    #F4# key can be used to assign a hotkey to item.

    #Del# key can be used:

     - to ~unmount~@DisconnectDrive@ filesystem at given path.

     - to delete a bookmark.

    The #Shift-Del# hotkey can be used to force-unmount filesystem that requires root privileges.

    #Ctrl-1# - #Ctrl-9# switch the display of different information:

    #F9# shows a ~dialog for configuring Location menu~@ChangeLocationConfig@.

    #Location# menu settings are saved in the FAR2L configuration.

    If the option "~Use Ctrl-PgUp for location menu~@InterfSettings@" is enabled,
pressing #Ctrl-PgUp# works the same as pressing #Esc# - closes the menu.

    Pressing #Shift-Enter# invokes system GUI file manager showing the directory
of the selected line (works only for disk drives and not for plugins).

    #Ctrl-R# allows to refresh the location menu.

    #Alt-Shift-F9# allows you to ~configure plugins~@PluginsConfig@ (it works only if
display of plugin items is enabled).

    #Shift-F9# in the plugins list opens the configuration dialog of the
currently selected plugin.

    #Shift-F1# in the plugins list displays the context-sensitive help of the
currently selected plugin, if the plugin has a help file.

    You can specify manual/scripted source of additional items in Location menu that
will be appended to mountpoints entries. For that you need to create text file under
path #~~/.config/far2l/favorites# and that file must contain lines, each line can have
one or two or three parts separated by <TAB> character. First part represent path,
second and third parts are optional and represent information rendered in additional
columns. It's possible to insert separator with optional title by specifying line
with first part having only '-' character and another part (if present) defining
title text.
Note that favorites file can contain shell environment variables denoted with $
character like $HOME, and shell commands substitution, i.e. $(/path/to/some/script.sh)
will invoke that script.sh and its output will be embedded into content of this file
during processing. This allows to implement custom dynamic locations list composing.

    If you don't see mounted flash drive in the Location menu (#Alt-F1/F2#)
then check #Exceptions list# in ~Location Menu Options~@ChangeLocationConfig@ (#F9#).
E.g., the #/run/*# pattern is included there by default.
If you have udisks2 configured to mount removable drives under #/run/media/$USER/#
you need to delete #/run/*# substring from exceptions list.
After that add more accurate patterns such as #/run/user/*#
in order to hide garbage mountpoints from the Location menu.

    See also:
      The list of ~macro keys~@KeyMacroDisksList@, available in the disk menu.
      Common ~menu~@MenuCmd@ keyboard commands.


@DisconnectDrive
$ #Disconnect network drive#
    You can unmount mountpoint by pressing #Del# in the
~location menu~@DriveDlg@.

    The option #[x] Reconnect at logon# is enabled only for permanently
connected network drives.

    The confirmation can be disabled in the ~confirmations~@ConfirmDlg@ dialog.


@Highlight
$ #Files highlighting and sort groups#
    For more convenient and obvious display of files and directories in the
panels, FAR2L has the possibility of color highlighting for file objects.
You can group file objects by different criteria (~file masks~@FileMasks@, file
attributes) and assign colors to those groups.

    File highlighting can be enabled or disabled in the ~panel settings~@PanelSettings@
dialog (menu item Options | Panel settings).

    You can ~edit~@HighlightEdit@ the parameters of any highlight group through
the "~Options~@OptMenu@" menu (item "~Files highlighting and sort groups~@HighlightList@").


@HighlightList
$ #Files highlighting and sort groups: control keys#
    The ~file highlighting and sort groups~@Highlight@ menu allows you to
perform various operations with the list of the groups. The following key
combinations are available:

  #Space#        - (De)Activate current group

  #Ins#          - Add a new highlighting group

  #F5#           - Duplicate the current group

  #Del#          - Delete the current group

  #Enter# or #F4#  - ~Edit~@HighlightEdit@ the current highlighting group

  #Ctrl-R#       - Restore the default file highlighting groups

  #Ctrl-Up#      - Move a group up.

  #Ctrl-Down#    - Move a group down.

    The highlighting groups are checked from top to bottom. If it is detected
that a file belongs to a group, no further groups are checked.

    Display of markers is controlled globally via a checkbox
in the ~Panel settings~@PanelSettings@ dialog
or may be switched by #Ctrl-Alt-M# in panels.

    See also: common ~menu~@MenuCmd@ keyboard commands.


@HighlightEdit
$ #Files highlighting and sort groups: editing#
    The #Files highlighting# dialog in the ~Options menu~@OptMenu@ allows to
define file highlighting groups. Each group definition ~includes~@Filter@:

     - one or more ~file masks~@FileMasks@;

     - attributes to include or exclude:
       #[x]# - inclusion attribute - file must have this attribute.
       #[ ]# - exclusion attribute - file must not have this attribute.
       #[?]# - ignore this attribute;

     - normal name, selected name, name under cursor and
       selected name under cursor colors to display file names.
       If you wish to use the default color, set color to "Black
       on black";

     - an optional character to mark files from the group.
       It may be used both with or instead of color highlighting.

    If the option "A file mask or several file masks" is turned off, file masks
will not be analyzed, and only file attributes will be taken into account.

    A file belongs to a highlighting group if:

     - file mask analysis is enabled and the name of the file matches
       at least one file mask (if file mask analysis is disabled,
       the file name doesn't matter);

     - it has all of the included attributes;

     - it has none of the excluded attributes.

    Display of markers is controlled globally via a checkbox
in the ~Panel settings~@PanelSettings@ dialog
or may be switched by #Ctrl-Alt-M# in panels.

    The Compressed, Encrypted, Not indexed, Sparse, Temporary attributes and
Symbolic links are valid for NTFS drives only.


@ViewerSettings
$ #Settings dialog: viewer#
    This dialog allows to change the default external or
~internal viewer~@Viewer@ settings.

    External viewer

  #Use for F3#              Run external viewer using #F3#.

  #Use for Alt-F3#          Run external viewer using #Alt-F3#.

  #Viewer command#          Command to execute external viewer.
                          Use ~special symbols~@MetaSymbols@ to specify the
                          name of the file to view. If you do
                          not wish to switch panels off before
                          executing the external viewer, start
                          command from '@@' character.

    Internal viewer

  #Save file state#         Save and restore positions in the recently
                          viewed files. This option also forces
                          restoring of code page, if the page
                          was manually selected by user, and the file
                          viewing mode (normal/hex). Unchecking this
                          option causes resetting of saved state
                          for viewed file.


  #Save bookmarks#          Save and restore bookmarks (current
                          positions) in recently viewed files
                          (created with #RightCtrl-0..9# or
                          #Ctrl-Shift-0..9#)

  #Auto detect#             ~Auto detect~@CodePage@ the code page of
  #code page#               the file being viewed.

  #Tab size#                Number of spaces in a tab character.

  #Show scrollbar#          Show scrollbar in internal viewer. This
                          option can also be switched by pressing
                          #Ctrl-S# in the internal viewer.

  #Show arrows#             Show scrolling arrows in viewer if the text
                          doesn't fit in the window horizontally.

  #Persistent selection#    Do not remove block selection after
                          moving the cursor.

  #Use ANSI code page#      Use ANSI code page for viewing files,
  #by default#              instead of OEM.

    If the external viewer is assigned to #F3# key, it will be executed only if
the ~associated~@FileAssoc@ viewer for the current file type is not defined.

    Modifications of settings in this dialog do not affect previously opened
internal viewer windows.

    The settings dialog can also be invoked from the ~internal viewer~@Viewer@
by pressing #Alt-Shift-F9#. The changes will come into force immediately but
will affect only the current session.


@EditorSettings
$ #Settings dialog: editor#
    This dialog allows to change the default external and
~internal editor~@Editor@ settings.

    External editor

  #Use for F4#              Run external editor using #F4# instead of
                          #Alt-F4#.

  #Editor command#          Command to execute the external editor.
                          Use ~special symbols~@MetaSymbols@ to specify the name
                          of the file to edit. If you do not wish
                          to switch panels off before executing
                          the external editor, start the command with
                          the '@@' character.


    Internal editor

  #Expand tabs#             (unchanged if file located in (sub)directory
                          with .editorconfig file contains "indent_style")

  - #Do not expand tabs#    Do not convert tabs to spaces while
                          editing the document.

  - #Expand newly entered#  While editing the document, convert each
    #tabs to spaces#        newly entered #Tab# into the appropriate
                          number of spaces. Other tabs won't be
                          converted.

  - #Expand all tabs to#    Upon opening the document, all tabs in
    #spaces#                the document will be automatically
                          converted to spaces.

  #Persistent blocks#       Do not remove block selection after
                          moving the cursor.

  #Del removes blocks#      If a block is selected, pressing #Del# will
                          not remove the character under cursor, but
                          this block.

  #Save file state#         Save and restore positions and indentation state in the recently
                          edited files. This option also forces
                          restoring of code page, if the page
                          was manually selected by user. Unchecking
                          this option causes resetting of saved state
                          for edited file.

  #Save bookmarks#          Save and restore bookmarks (current
                          positions) in recently edited files
                          (created with #RightCtrl-0..9# or
                          #Ctrl-Shift-0..9#)

  #Auto indent#             Enables auto indent mode when entering
                          text.

  #Cursor beyond#           Allow moving cursor beyond the end of line.
  #end of line#

  #Tab size#                Number of spaces in a tab character
                          (unchanged if file located in (sub)directory
                          with .editorconfig file contains "indent_size").

  #Show scrollbar#          Show scrollbar.

  #Pick up the word#        При вызове диалога поиска/замены в строку
                          поиска будет подставляться слово, на
                          котором стоит курсор.

  #Use .editorconfig#       Processing .editorconfig parameters
  #settings files#          (see https://editorconfig.org for details)

  #Lock editing of#         When a file with the Read-only attribute
  #read-only files#         is opened for editing, the editor also
                          disables the modification of the edited
                          text, just as if #Ctrl-L# was pressed.

  #Warn when opening#       When a file with the Read-only attribute
  #read-only files#         is opened for editing, a warning message
                          will be shown.

  #Auto detect#             ~Auto detect~@CodePage@ the code page of
  #code page#               the file being edited.

  #Choose default#          Code page for new files,
  #code page#               usually UTF-8.

    If the external editor is assigned to #F4# key, it will be executed only if
~associated~@FileAssoc@ editor for the current file type is not defined.

    Modifications of settings in this dialog do not affect previously opened
internal editor windows.

    The settings dialog can also be invoked from the ~internal editor~@Editor@
by pressing #Alt-Shift-F9#. The changes will come into force immediately but
will affect only the current session.


@CodePage
$ #Auto detect code pages#
    FAR2L will try to choose the correct code page for viewing/editing a file.
Note that correct detection is not guaranteed, especially for small or
non-typical text files.


@FileAttrDlg
$ #File attributes dialog#
    With this command it is possible to change file attributes and file time.
Either single file or group of files may be processed. If you do not want to
process files in subfolders, clear the "Process subfolders" option.

  #File attributes#

    Checkboxes used in the dialog can have the following 3 states:

     #[x]# - attribute is set for all selected items
           (set the attribute for all items)

     #[ ]# - attribute is not set for all selected items
           (clear the attribute for all items)

     #[?]# - attribute state is not the same for selected items
           (don't change the attribute)

    When all selected files have the same attribute value, the corresponding
checkbox will be in 2-state mode - set/clear only. When there are selected
folders, all checkboxes will always be 3-state.

    Only those attributes will be changed for which the state of the
corresponding checkboxes was changed from the initial state.

    The #Compressed#, #Encrypted#, #Not indexed#, #Sparse#, #Temporary#,
#Offline#, #Reparse point# and #Virtual# attributes are available only on NTFS drives. The
#Virtual# attribute is not used in Windows 2000/XP/2003. The #Compressed#
and #Encrypted# attributes are mutually exclusive, that is, you can set only
one of them. You cannot clear the #Sparse# attribute in Windows 2000/XP/2003.

    For ~symbolic links~@HardSymLink@ the dialog will display the path where it refers to.
If this information is not available, then the "#(data not available)#" message will be shown.

  #File date and time#

    Four different file times are supported:

    - last write time;

    - creation time;

    - last access time;

    - change time.

    On FAT drives the hours, minutes, seconds and milliseconds of the last access time are
always equal to zero.

    If you do not want to change the file time, leave the respective field
empty. You can push the #Blank# button to clear all the date and time fields
and then change an individual component of the date or time, for example, only
month or only minutes. All the other date and time components will remain
unchanged.

    The #Current# button fills the file time fields with the current time.

    The #Original# button fills the file time fields with their original
values. Available only when the dialog is invoked for a single file object.


    The #System properties# button invoke the system properties dialog for
selected objects.



@Bookmarks
$ #Bookmarks#
    Bookmarks are designed to provide fast access to frequently used
folders. Press #Ctrl-Shift-0..9#, to create a shortcut to the current folder.
To change to the folder recorded in the shortcut, press #RightCtrl-0..9#. If
#RightCtrl-0..9# pressed in edit line, it inserts the shortcut path into the
line. Bookmarks are also available from Location menu.

    The #Show bookmarks# item in the ~Commands menu~@CmdMenu@ may be
used to view, set, edit and delete bookmarks on different shortcuts.

    You can move selected bookmark to upper/lower position by pressing
#Shift+Up# and #Shift+Down# keys.

    When you are editing a bookmark (#F4#), you cannot create a bookmark to a
plugin panel.

    See also: common ~menu~@MenuCmd@ keyboard commands.

@FiltersMenu
$ #Filters menu#
    Using the #Filters menu# you can define a set of file types with user
defined rules according to which files will be processed in the area of
operation this menu was called from.

    The filters menu consists of two parts. In the upper part custom #User#
#filters# are shown, the lower part contains file masks of all the files that
exist in the current panel (including file masks that are selected in the
current area of operation the menu was called from even if there are no files
that match those mask in the current panel).

    For the #User filters# the following commands are available:

   #Ins#        Create a new filter, an empty ~filter~@Filter@ settings
              dialog will open for you to set.

   #F4#         Edit an existing ~filter~@Filter@.

   #F5#         Duplicate an existing ~filter~@Filter@.

   #Del#        Remove a filter.

   #Ctrl-Up#    Move a filter one position up.

   #Ctrl-Down#  Move a filter one position down.


    To control the #User filters# and also the auto-generated filters (file
masks) the following commands are available:

   #Space#,              Items selected using #Space# or '#+#' are
   #Plus#                marked by '+'. If such items are present
                       then only files that match them will be
                       processed.

   #Minus#               Items selected using '#-#' are marked by '-',
                       and files that match then will not be
                       processed.

   #I# and #X#             Similar to #Plus# and #Minus# respectively,
                       but have higher priority when matching.

   #Backspace#           Clear selection from the current item.

   #Shift-Backspace#     Clear selection from all items.


    Filters selection is stored in the FAR2L configuration.

    When a filter is used in a panel, it is indicated by '*' after the sort
mode letter in the upper left corner of the panel.

    Filters menu is used in the following areas:
     - ~File panel~@FilePanel@;
     - ~Copying, moving, renaming and creating links~@CopyFiles@;
     - ~Find file~@FindFile@.

    See also: common ~menu~@MenuCmd@ keyboard commands.

@FileDiz
$ #File descriptions#
    File descriptions may be used to associate text information with a file.
Descriptions of the files in the current folder are stored in this folder in a
description list file. The format of the description file is the file name
followed by spaces and the description.

    Descriptions may be viewed in the appropriate file panel
~view modes~@PanelViewModes@. By default these modes are #Descriptions#
and #Long descriptions#.

    The command #Describe# (#Ctrl-Z#) from the ~Files menu~@FilesMenu@ is used
to describe selected files.

    Description list names may be changed using #File descriptions# dialog from
the ~Options menu~@OptMenu@. In this dialog you can also set local descriptions
update mode. Updating may be disabled, enabled only if panel current view mode
displays descriptions or always enabled. By default FAR2L sets "Hidden" attribute
to created description lists, but you may disable it by switching off the
option "Set "Hidden" attribute to new description lists" in this dialog. Also
here you may specify the position to align new file descriptions in a
description list.

    If a description file has the "read-only" attribute set, FAR2L does not
attempt to update descriptions, and after moving or deleting file objects, an
error message is shown. If the option "#Update read only description file#" is
enabled, FAR2L will attempt to update the descriptions correctly.

    If it is enabled in the configuration, FAR2L updates file descriptions when
copying, moving and deleting files. But if a command processes files from
subfolders, descriptions in the subfolders are not updated.


@PanelViewModes
$ #Customizing file panel view modes#
    The ~file panel~@FilePanel@ can represent information using 10 predefined
modes: brief, medium, full, wide, detailed, descriptions, long descriptions,
file owners, file links and alternative full. Usually it is enough, but if you
wish, you may either customize its parameters or even replace them with
completely new modes.

    The command #File panel modes# from the ~Options menu~@OptMenu@ allows to
change the view mode settings. First, it offers to select the desired mode from
the list. In this list "Brief mode" item corresponds to brief panel mode
(#LeftCtrl-1#), "Medium" corresponds to medium panel mode (#LeftCtrl-2#) and so
on. The last item, "Alternative full", corresponds to view mode called with
#LeftCtrl-0#. After selecting the mode, you may change the following settings:

  - #Column types# - column types are encoded as one or several
characters, delimited with commas. Allowed column types are:

    N[M,O,R]   - file name
                 where: M - show selection marks;
                        O - show names without paths
                            (intended mainly for plugins);
                        R - right aligned names;
                 These modifiers may be used in combination,
                 for example NMR

    S[C,T,F,E] - file size
    P[C,T,F,E] - packed file size
    G[C,T,F,E] - size of file streams
                 where: C - format file size;
                        T - use 1000 instead of 1024 as a divider;
                        F - show file sizes similar to Windows
                            Explorer (i.e. 999 bytes will be
                            displayed as 999 and 1000 bytes will
                            be displayed as 0.97 K);
                        E - economic mode, no space between file
                            size and suffix will be shown
                            (i.e. 0.97K);

    D          - file last write date
    T          - file last write time

    DM[B,M]    - file last write date and time
    DC[B,M]    - file creation date and time
    DA[B,M]    - file last access date and time
    DE[B,M]    - file change date and time
                 where: B - brief (Unix style) file time format;
                        M - use text month names;

    A          - file attributes
    Z          - file descriptions

    O[L]       - file owner
                 where: L - show domain name;

    LN         - number of hard links

    F          - number of streams

    If the column types description contains more than one file name column,
the file panel will be displayed in multicolumn form.

    Windows file attributes have the following indications:
       #R#         - Read only
       #S#         - System
       #H#         - Hidden
       #A#         - Archive
       #L#         - Junction or symbolic link
       #C# or #E#    - Compressed or Encrypted
       #$#         - Sparse
       #T#         - Temporary
       #I#         - Not content indexed
       #O#         - Offline
       #V#         - Virtual

    Unix file types:
       #B#         - Broken
       #d#         - Directory
       #c#         - Character device
       #b#         - Block device
       #p#         - FIFO (named Pipe)
       #s#         - Socket
       #l#         - Symbolic Link
       #-#         - Regular file
    Unix file permissions (in each triad for owner, group, other users):
       #r# or #-#    - readable or not
       #w# or #-#    - writable or not
       #x# or #-#    - executable or not
       #s# or #S#    - setuid/setgid also executable (#s#) or not executable (#S#)
       #t# or #T#    - sticky also executable (#t#) or not executable (#T#)

    The attributes are displayed in the following order - RSHALCTIOV. The
"Sparse" attribute applies only to files and is shown instead of 'L'. The
"Encrypted" attribute is shown instead of 'C' as a file/folder can not
have both attributes ("Compressed" and "Encrypted") set at the same time.
By default the size of the attributes column is 6 characters. To display
the additional 'T', 'I', 'O' and 'V' attributes it is necessary to manually
set the size of the column to 10 characters.

  - #Column widths# - used to change width of panel columns.
If the width is equal to 0, the default value will be used. If the width of
the Name, Description or Owner column is 0, it will be calculated
automatically, depending upon the panel width. For correct operation with
different screen widths, it is highly recommended to have at least one column
with automatically calculated width. Width can be also set as a percentage of
remaining free space after the fixed width columns by adding the "%" character
after the numerical value. If the total size of such columns exceeds 100%,
their sizes are scaled.

    Incrementing the default width of the file time column or file date and
time column by 1 will force a 12-hour time format. Further increase will lead
to the display of seconds and milliseconds.

    To display years in 4-digits format increase the date column width by 2.

    При указании колонок, отвечающих за показ ссылок, потоков и владельцев (G, LN, F и O) время
отображения содержимого каталога увеличивается.

  - #Status line column types# and #Status line column widths# -
similar to "Column types" and "Column widths", but for panel status line.

  - #Fullscreen view# - force fullscreen view instead of half-screen.

  - #Align file extensions# - show file extensions aligned.

  - #Align folder extensions# - show folder extensions aligned.

  - #Show folders in uppercase# - display all folder names in upper
case, ignoring the original case.

  - #Show files in lowercase# - display all file names in lower case,
ignoring the original case.

  - #Show uppercase file names in lowercase# - display all uppercase
file names in lower case. By default this option is on, but if you wish
to always see the real files case, switch it, "Show folders in uppercase"
and "Show files in lowercase" options off. All these settings only change
the method of displaying files, when processing files FAR2L always uses the
real case.

  See also: common ~menu~@MenuCmd@ keyboard commands.

@SortGroups
$ #Sort groups#
    File sort groups may be used in #by name# and #by extension#
~file panel~@FilePanel@ sort modes. Sort groups are applied by
pressing #Shift-F11# and allow to define additional file sorting rules,
complementary to those already used.

    Each sort group contains one or more comma delimited
~file masks~@FileMasks@. If one sort group position in the group list
is higher than another and an ascending sort is performed, all files
belonging to this group files will be higher than those belonging to
following groups.

    The command #Edit sort groups# from the ~Commands menu~@CmdMenu@ is used to
delete, create and edit sort groups, using #Del#, #Ins# and #F4#. The groups
above the menu separator are applicable to the file panel start, and included
files will be placed higher than those not included to any group. The groups
below the menu separator are applicable to the file panel end, and included
files will be placed lower than those not included.


@FileMasks
$ #File masks#
    File masks are frequently used in FAR2L commands to select single files and
folders or groups of them. Masks may contain common valid file name symbols,
wildcards ('*' and '?') and special expressions:

    #*#           zero or more characters;

    #?#           any single character;

    #[c,x-z]#     any character enclosed by the brackets.
                Both separate characters and character intervals
                are allowed.

    For example, files ftp.exe, fc.exe and f.ext may be selected using mask
f*.ex?, mask *co* will select both color.ini and edit.com, mask [c-f,t]*.txt
can select config.txt, demo.txt, faq.txt and tips.txt.

    In many FAR2L commands you may enter several file masks separated with commas
or semicolons. For example, to select all the documents, you can enter
#*.doc,*.txt,*.wri# in the "Select" command.

    It is allowed to put any of the masks in quotes but not the whole list. For
example, you have to do this when a mask contains any of the delimiter
characters (a comma or a semicolon), so that the mask doesn't get confused with
a list.

    File mask surrounded with slashes #/# is treated as ~Perl regular expression~@RegExp@.

    Example:
    #/(eng|rus)/i#  any files with filenames containing string “eng” or “rus”,
                  the character case is not taken into account.

    In some commands (~find files~@FindFile@, ~filter~@Filter@,
~filters menu~@FiltersMenu@, file ~selection~@SelectFiles@,
file ~associations~@FileAssoc@ and
~file highlighting and sort groups~@Highlight@) you may use exclude masks. An
#exclude mask# is one or multiple file masks that must not be matched by the
files matching the mask. The exclude mask is delimited from the main mask by
the character '#|#'.

^Usage examples of exclude masks:
 1. *.cpp
    All files with the extension #cpp#.
 2. *|*.bak,*.tmp
    All files except for the files with extensions #bak# and #tmp#.
 3. *|
    This mask has an error - the character | is entered, but the
    mask itself is not specified.
 4. *|*.bak|*.tmp
    Also an error - the character | may not be contained in the mask
    more than once.
 5. |*.bak
    The same as *|*.bak
 6. *|/^pict\d{1,3}\.gif$/i
    All files except for pict0.gif — pict999.gif, disregard the character case.

    The comma (or semicolon) is used for separating file masks from each other,
and the '|' character separates include masks from exclude masks.


@SelectFiles
$ #Selecting files#
    To process several ~file panel~@FilePanel@ files and folders, they may be
selected using different methods. #Ins# selects the file under the cursor and
moves the cursor down, #Shift-Cursor keys# move the cursor in different
directions.

    #Gray +# and #Gray -# perform group select and deselect, using one or more
~file masks~@FileMasks@ delimited by commas. #Gray *# inverts the current
selection. #Restore selection# command (#Ctrl-M#) restores the previously
selected group.

    #Ctrl-<Gray +># and #Ctrl-<Gray -># selects and deselects all files with
the same extension as the file under cursor.

    #Alt-<Gray +># and #Alt-<Gray -># selects and deselects all files with the
same name as the file under cursor.

    #Ctrl-<Gray *># inverts the current selection including folders. If #Select#
#folders# option in ~Panel settings~@PanelSettings@ dialog is on, it has the
same function as #Gray *#.

    #Shift-<Gray +># and #Shift-<Gray -># selects and deselects all files.

    If no files are selected, only the file under the cursor will be processed.

    See options #Select folders# and #Case sensitive when compare or select#
in ~Panel settings~@PanelSettings@.


@CopyFiles
$ #Copying, moving, renaming and creating links#
    Following commands may be used to copy, move and rename files and folders:

  Copy ~selected~@SelectFiles@ files                                           #F5#

  Copy the file under cursor regardless of selection      #Shift-F5#

  Rename or move selected files                                 #F6#

  Rename or move the file under the cursor                #Shift-F6#
  regardless of selection

    For folders: if the specified path (absolute or relative) points to an
existing folder, the source folder is moved inside that folder. Otherwise the
folder is renamed/moved to the new path.
    E.g. when moving #/folder1/# to #/folder2/#:
    - if #/folder2/# exists, contents of #/folder1/# is
moved into #/folder2/folder1/#;
    - otherwise contents of #/folder1/# is moved into the
newly created #/folder2/#.

  Create ~file links~@HardSymLink@                                         #Alt-F6#

    If #Process multiple destinations# is enabled, you may specify
multiple copy or move targets in the input line. In this case, targets should
be separated with a character "#;#" or "#,#". If the name of a target contains
the character ";" or ",", it must be enclosed in quotes.

    If #Copy files access mode# is enabled, then copied files will get same
UNIX access mode bits as original files had unless some bits masked out by current umask.

    If #Copy extended attributes# is enabled, then copied files will get same
extended attributes as original files.

    If #Disable write cache# is enabled, then copy routine will use O_DIRECT flag
unless its not supported by OS or filesystem.

    If #Produce sparse files# is enabled, then copy routine will detect zero-filled
regions at least 4KB length and will create sparse files with holes instead of them.
This saves disk space and improves copy speed if files contain huge zero-filled regions.
Potential downside includes higher file fragmentation if such regions will be overwritten
with normal data in the future.

    If #Use copy-on-write# is enabled, then copy routine will use special kernel API
that copies files in a way, so copied files refer original files data and will be really
copied only when that data will be modified in any of files. Note that this functionality
requires Linux kernel v4.5+ or MacOS 10.12+ and any FS that supports COW files otherwise
files will be silently copied using conventional way. If being in use this option greatly
improves copy speed and reduces disk space usage. Potential downside include higher file
fragmentation if it or original file will be overwritten in the future.

    #With symlink# combobox allows to chose from any of three possible ways of handling
symlinks during copying:
    - Either all symlinks will be copied as is.
    - Either far2l will check each symlink target to find out if it refers 'outer' file
or some file also being copied. In first case link will be copied as file, in second
it will be copied as symlink with possible adjusted destination, so it will refer copied
target file.
    - Either all symlinks will be copied as files.

    If you wish to create the destination folder before copying, terminate the
name with backslash. Also in the Copy dialog you may press #F10# to select a
folder from the active file panel tree or #Alt-F10# to select from the passive
file panel tree. #Shift-F10# allows to open the tree for the path entered in
the input line (if several paths are entered, only the first one is taken into
account). If the option "Process multiple destinations" is enabled, the dialog
selected in the tree is appended to the edit line.

    The possibility of copying, moving and renaming files for plugins depends
upon the plugin module functionality.

    If a destination file already exists, it can be overwritten, skipped or
appended with the file being copied.

    If during copying or moving the destination disk becomes full, it is
possible to either cancel the operation or replace the disk and select the
"Split" item. In the last case the file being copied will be split between
disks. This feature is available only when "Use system copy routine" option in
the ~System settings~@SystemSettings@ dialog is switched off.

    The "Already existing files" option controls FAR2L behavior if a target file
of the same name already exists.
    Possible values:
    #Ask# - a ~confirmation dialog~@CopyAskOverwrite@ will be shown;
    #Overwrite# - all target files will be replaced;
    #Skip# - target files will not be replaced;
    #Append# - target file will be appended with the file being copied;
    #Only newer file(s)# - only files with newer write date and time
will be copied; This option affects only the current copy session and not saved
for later copy operations.
    #Also ask on R/O files# - controls whether an additional confirmation
dialog should be displayed for read-only files.
    If the corresponding item in ~Confirmations~@ConfirmDlg@ is unchecked,
then "Already existing files" are disabled
and the #Overwrite# action is silently applied.

    When moving files, to determine whether the operation should be performed
as a copy with subsequent deletion or as a direct move (within one physical
drive), FAR2L takes into account ~symbolic links~@HardSymLink@.

    Check the #Use filter# checkbox to copy the files that meet the user
defined conditions. Press the #Filter# button to open the ~filters menu~@FiltersMenu@.
Consider, that if you copy the folder with files and all of them does not meet
the filter conditions, then the empty folder will not be copied to the
destination.


@CopyAskOverwrite
$ #Copying: confirmation dialog#
    If a file of the same name exists in the target folder the user will be
prompted to select on of the following actions:

    #Overwrite# - target file will be replaced;

    #Skip# - target file will not be replaced;

    #Rename# - existing file will not be changed, a new name will be given to
the file being copied;

    #Append# - target file will be appended with the file being copied;

    If #Remember choice# is checked, the selected action will be applied to
all existing files and the confirmation dialog will not be displayed again for
the current copying session.

    If the displayed information is not sufficient you can also view the files
in the internal viewer without exiting the confirmation dialog.


@CopyRule
$ #Copying: rules#
    When ~copying/moving~@CopyFiles@ folders and/or
~symbolic links~@HardSymLink@ the following rules apply:

@HardSymLink
$ #Hard and Symbolic link#
    You can create #hard links# or #symbolic links# for files and only
#symbolic links# for folders using the #Alt-F6# command.

    #Hard links#

    A #hard link# is an additional directory entry for the given file. When a
hard link is created, the file is not copied itself, but receives one more name
or location, while its previous name and location remain intact. Since the
moment of its creation, a hard link is indistinguishable from the original
entry. The only difference is that short file names are not created for hard
links, and so they cannot be seen from DOS programs.

    When the file size or date changes, all of the corresponding directory
entries are updated automatically. When a file is deleted, it is not deleted
physically until all the hard links pointing at it will be deleted. The
deletion order doesn't matter. When a hard link is deleted into the recycle
bin, the number of links of a file does not change.

    FAR2L can create hard links and can show the number of the file's hard links
in a separate column (by default, it's the last column in the 9th panel mode)
and sort the files by hard link number.

    Hard links can only be created on the same device as the source file.

    #Symbolic links#

    Symbolic links point to files and non-local folders, relative paths also supported.

    #Default suggestion# in field #Link type# may be changed in ~System settings~@SystemSettings@ to
    - Hardlink for files, Symlink for directories
    - Symlink always

@ErrCopyItSelf
$ #Error: copy/move onto itself.#
    You may not copy or move a file or folder onto itself.

    This error can also happen if there are two directories, one of which is
a ~symbolic link~@HardSymLink@ to another.


@WarnCopyEncrypt
$ #Warning: Losing file encryption#
    The source file is encrypted. Copying or moving it outside of the current
disk is possible if in the destination the file will be decrypted.

    The "Ignore" (or "Ignore all") buttons ignore the given warning and copy
the file unencrypted to the destination.

    The internal copying mechanism will be used for copying encrypted files
outside of the current disk with no regard to the "Use system copy routine"
option.


@WarnCopyStream
$ #Warning: copying or moving file with multiple streams#

    The source file contains more than one data stream or the destination file
system does not support files with multiple streams.

    Streams are a feature of the NTFS file system allowing to associate
additional information with a file (for example, author's name, title, keywords
and so on, or any other data). This information is stored together with the
file and is invisible to programs that do not support streams. For example,
streams are used by Windows Explorer to store additional file properties
(summary). FAT/FAT32 file systems do not support streams.

    To copy a file completely (together with all its streams), turn on the
option "#Use system copy routine#" in the ~system settings~@SystemSettings@
dialog.

    If you are copying a file with multiple streams to a volume with a file
system other than NTFS, you will also lose data - only the main stream will be
copied.


@ErrLoadPlugin
$ #Error: plugin not loaded#
   This error message can appear in the following cases:

   1. A dynamic link library not present on your system is required
      for correct operation of the plugin module.

   2. For some reason, the module returned an error code
      telling the system to abort plugin loading.

   3. The DLL file of the plugin is corrupt.


@ScrSwitch
$ #Screens switching#
    FAR2L allows to open several instances of the internal viewer and editor at
the same time. Use #Ctrl-Tab#, #Ctrl-Shift-Tab# or #F12# to switch between
panels and screens with these instances. #Ctrl-Tab# switches to the next
screen, #Ctrl-Shift-Tab# to the previous, #F12# shows a list of all available
screens.

    Additionally there can be multiple terminal commands running in background.
You may view or activate any of them also from #F12# menu: use #F3# to view
current command output or Enter to switch to it in terminal.

    The number of background terminal commands, viewers and editors is displayed
in the left panel upper left corner. This may be disabled by using
~Panel settings~@PanelSettings@ dialog.

    See also: common ~menu~@MenuCmd@ keyboard commands.

@ApplyCmd
$ #Apply command#
    With #Apply command# item in ~Files menu~@FilesMenu@ it is possible to
apply a command to each selected file. The same ~special symbols~@MetaSymbols@
as in ~File associations~@FileAssoc@ should be used to denote the file name.

    For example, 'type !.!' will output to the screen all selected files, one
at a time, and the command 'rar32 m !.!.rar !.!' will move all selected files
into RAR archives with the same names. The command 'explorer /select,!.!' will
start system GUI file browser and set the cursor to the current file
or directory.

    See also ~Special commands~@SpecCmd@
    See also ~Operating system commands~@OSCommands@

@OSCommands
$ #Operating system commands#
    FAR2L by itself processes the following operating system commands:

    #CLS#

    Clears the screen.

    #disk:#

    !! Windows legacy (not relevant on Linux/*BSD/Mac) !!

    To change the current disk on the active panel to the specified disk.

    !! Windows legacy end !!

    #CD path# or #CHDIR path#

    To change the current path on the active panel to the specified path.
If the active panel shows a ~plugin~@Plugins@ emulated file system, the command
"CD" in the command line may be used to change the folder in the plugin file
system. Unlike "CD", "CHDIR" command always treats the specified parameter
as a real folder name, regardless of the file panel type.

    #CHCP [nnn]#

    Displays or sets the active code page number. "nnn" - specifies a code
page number. Type CHCP without a parameter to display the active code
page number.

    #SET variable=[string]#

    Set environment variable "variable" to the value "string". If "string" is
not specified, the environment variable "variable" will be removed. On startup,
FAR2L sets several ~environment variables~@FAREnv@ by itself.

    #IF [NOT] EXIST filename command#

    Execute a command "command" if "filename" exists. Prefix "NOT" - execute
the command only if the condition is false.

    #IF [NOT] DEFINED variable command#

    The "DEFINED" conditional works just like "EXISTS" except it takes an
environment variable name and returns true if the environment variable is
defined.


    "IF" commands can be nested, for instance, command "command"

    #if exist file1 if not exist file2 if defined variable command#

    will be executed if the file "file1" exists, the file "file2" does not
exist and the environment variable "variable" is defined.

    #pushd path#

    Команда PUSHD сохраняет текущий каталог во внутреннем стеке и делает
текущим каталог path.

    #popd#

    Переходит в каталог, сохраненный командой PUSHD.

    #clrd#

    Очищает стек каталогов, сохраненных командой PUSHD.

    Notes:

    1. ^<wrap>Any other commands will be sent to the operating
system command processor.

    2. The commands listed above work in:
       - ~Command line~@CmdLineCmd@
       - ~Apply command~@ApplyCmd@
       - ~User menu~@UserMenu@
       - ~File associations~@FileAssoc@


@FAREnv
$ #Environment variables#
    On startup, FAR2L sets the following environment variables available
to child processes:

    #FARHOME#            path to the folder from which FAR2L was started.

    #FARLANG#            the name of the current interface language.

    #FARUSER#            ^<wrap>the name of the current user given by the -u ~command line~@CmdLine@ option.

    #FARDIRSTACK#        ^<wrap>the contents of directories stack top (the stack is managed with #pushd# and #popd# commands)

    #FARADMINMODE#       ^<wrap>equals "1" if FAR2L was run by an administrator


@RegExp
$ #Regular expressions#
    The regular expressions syntax is almost equal to Perl regexp`s.

    General form: #regexp# or /#regexp#/#options#.

    #Options#:
    #i# - ignore character case;
    #s# - ^<wrap>consider the whole text as one line, '.' matches any character;
    #m# - ^<wrap>consider the whole text as multiple lines. ^ and $ match the
    beginning and the end of any "inner" string;
    #x# - ^<wrap>ignore space characters (unscreened ones, i.e. without backslash before).
This is useful to outline the complex expressions.

    #regexp# - the sequence of characters and metacharacters. The characters are
letters and digits, any other symbol becomes character when screened, i.e.
prepended the backslash #\#.

    Pay attention that all slashes and backslashes in regular expression must
be prepended with the symbol #\# to differ from other special symbols or with
the end of expression. An example: the string "big\white/scary" looks in the
form of regular expression like "big\\white\/scary".

    #Metacharacters#

    #\#  - ^<wrap>the next symbol is treated as itself, not a metacharacter
    #^#  - ^<wrap>the beginning of string
    #$#  - ^<wrap>the end of string
    #|#  - ^<wrap>the alternative. Either expression before or after #|# has to match.

          ^<wrap>An example: "\d+\w+|Hello\d+" means "(\d+\w+)|(Hello\d+)", not "\d+(\w+|H)ello\d+".

    #()# - ^<wrap>grouping - it is used for references or when replacing matched text.
    #[]# - ^<wrap>character class - the metacharacter which matches any symbol
or range of symbols enumerated in #[]#. Ranges are defined as [a-z].
Metacharacters are not taken into account in character classes. If the first
symbol in class is #^# then this is a negative class. If the character #^# has
to be added to class, then it either must not to be at first place or it must
be prepended with #\#.

    Except grouping, the parentheses are used for the following operations:
    #(?:pattern)#  - ^<wrap>usual grouping, but it does not get a number.
    #(?=pattern)#  - ^<wrap>the forward lookup. The matching continues from
the same place, but only if the pattern in these parentheses has matched. For
example, #\w+(?=\s)# matches the word followed by space symbol, and the space
is not included into the search result.
    #(?!pattern)#  - ^<wrap>the negation of forward lookup. The matching
continues from the same place if the pattern does not match. For example,
#foo(?!bar)# matches any "foo" without following "bar". Remember that this
expression has zero size, which means that #a(?!b)d# matches #ad# because #a#
is followed by the symbol, which is not #b# (but #d#), and #d# follows the
zero-size expression.
    #(?<=pattern)# - ^<wrap>the backward lookup. Unfortunately, the pattern must have fixed length.
    #(?<!pattern)# - ^<wrap>the negation of backward lookup. The same restriction.

    #Quantifiers#

    Any character, group or class can be followed by a quantifier:

    #?#      - ^<wrap>Match 0 or 1 time, greedily.
    #??#     - ^<wrap>Match 0 or 1 time, not greedily.
    #*#      - ^<wrap>Match 0 or more times, greedily.
    #*?#     - ^<wrap>Match 0 or more times, not greedily.
    #+#      - ^<wrap>Match 1 or more times, greedily.
    #+?#     - ^<wrap>Match 1 or more times, not greedily
    #{n}#    - ^<wrap>Match exactly n times.
    #{n,}#   - ^<wrap>Match at least n times, greedily.
    #{n,}?#  - ^<wrap>Match at least n times, not greedily.
    #{n,m}#  - ^<wrap>Match at least n but not more than m times, greedily.
    #{n,m}?# - ^<wrap>Match at least n but not more than m times, not greedily.
    #{,m}#   - ^<wrap>equals to {0,m}
    #{,m}?#  - ^<wrap>equals to {0,m}?


    #"Greedy" and "not greedy" quantifiers#

    Greedy quantifier captures as much symbols as possible, and only if
    further match fails, it "returns" the captured string (the rollback
happens, which is rather expensive).
    When expression "A.*Z" is matched to string
"AZXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", #.*# captures the whole string, and then
rolls back symbol by symbol until it finds Z. On the opposite, if the expression
is "A.*?Z" then Z is found at once. Not greedy quantifier is also known as
#minimizing#, it captures minimal possible quantity of symbols, and only if
further match fails it captures more.

    #Special symbols#

   Non-letter and non-digit symbol can be prepended by '#\#' in most cases,
but in case of letters and digits this must be done with care because this is
the way thew special symbols are written:

    #.#    - ^<wrap>any symbol except carriage return. If there is “s” among
the options then this can be any symbol.
    #\t#   - tab (0x09)
    #\n#   - new line (lf, 0x0a)
    #\r#   - carriage return (cr, 0x0d)
    #\f#   - form feed (0x0c)
    #\a#   - bell (0x07)
    #\e#   - escape (0x1b)
    #\xNNNN# - hex character, where N - [0-9A-Fa-f].
    #\Q#   - ^<wrap>the beginning of metacharacters quoting - the whole quoted
text is treated as text itself, not the regular expression
    #\E#   - the end of metacharacters quoting
    #\w#   - letter, digit or '_'.
    #\W#   - not \w
    #\s#   - space symbol (tab/space/lf/cr).
    #\S#   - not \s
    #\d#   - digit
    #\D#   - not digit
    #\i#   - letter
    #\I#   - not letter
    #\l#   - lower case symbol
    #\L#   - not lower case symbol
    #\u#   - upper case symbol
    #\U#   - not upper case symbol
    #\b#   - ^<wrap>the word margin - means that to the left or right from the
current position there is a word symbol, and to the right or left,
accordingly, there is non-word symbol
    #\B#   - not \b
    #\A#   - the beginning of the text, disregard the option “m”
    #\Z#   - the end of the text, disregard the option “m”
    #\O#   - ^<wrap>the no-return point. If the matching has passed by this symbol,
it won't roll back and and will return "no match". It can be used in complex expressions
after mandatory fragment with quantifier. This special symbol can be used when
big amounts of data are processed.
         Example:
         /.*?name\O=(['"])(.*?)\1\O.*?value\O=(['"])(.*?)\3/
         ^<wrap>Strings containing "name=", but not containing "value=", are processed (in fact, skipped) faster.

    #\NN#  - ^<wrap>reference to earlier matched parentheses . NN is an integer from 0 to 15.
Each parentheses except (?:pattern), (?=pattern), (?!pattern), (?<=pattern) and (?<!pattern)
have a number (in the order of appearance).

         Example:
         "(['"])hello\1" matches to "hello" or 'hello'.


    #Examples:#

    #/foobar/#
       matches to "foobar", but not to "FOOBAR"
    #/ FOO bar /ix#
       matches to "foobar" and "FOOBAR"
    #/(foo)?bar/#
       matches to "foobar" and "bar"
    #/^foobar$/#
       matches to "foobar" only, but not to "foofoofoobarfoobar"
    #/[\d\.]+/#
       matches to any number with decimal point
    #/(foo|bar)+/#
       matches to "foofoofoobarfoobar" and "bar"
    #/\Q.)))$\E/#
       equals to "\.\)\)\)\$"

@RegExpRepl
$ #Regular expressions in replace#
    In "Replace with" line one can use special replace string regular
expressions:

    #$0#..#$9#, #$A#..#$Z#

    The found group numbers, they are replaced with appropriate groups.
The numbers are assigned to the groups in order of opening parentheses
sequence in regular expression. #$0# means the whole found sequence.
#$*# is replaced with '*' character.


@ElevationDlg
$ #Запрос привилегий администратора#


@KeyMacro
$ #Macro command #
    Keyboard macro commands or macro commands - are recorded sequences of key
presses that can be used to perform repetitive task unlimited number of times
by pressing a single hotkey.

    Each macro command has the following parameters:

    - an hotkey, that will execute the recorded sequence when
      pressed;
    - additional ~settings~@KeyMacroSetting@, that influence the method and
      the area of execution of the recorded sequence.

    Macro commands may contain special ~commands~@KeyMacroLang@, that will be
interpreted in a special way upon execution, those allowing to create complex
constructions.

    Macro commands are mostly used for:

    1. Performing repetitive task unlimited number of times by
       pressing a single hotkey.
    2. Execution of special functions, which are represented by
       special commands in the text of the macro command.
    3. Redefine standard hotkeys, which are used by FAR2L for
       execution of internal commands.

    The main usage of macro commands is assignment of hotkeys for calling
external plugins and for overloading FAR2L actions.

    See also:

    ~Macro command areas of execution~@KeyMacroArea@
    ~Hotkeys~@KeyMacroAssign@
    ~Recording and playing-back macro commands~@KeyMacroRecPlay@
    ~Deleting a macro command~@KeyMacroDelete@
    ~Macro command settings~@KeyMacroSetting@
    ~Commands, used inside the text of a macro command~@KeyMacroLang@


@KeyMacroArea
$ #Macro command: areas of execution#
    FAR2L allows the creation of independent ~macro commands~@KeyMacro@ (commands with
identical hotkeys) for different areas of execution.

    Attention: The area of execution, to which the macro command will
               belong, is determined by the location in which the
               recording of the macro command has been #started#.

    Currently those are the available independent areas:

    - file panels;
    - internal viewer;
    - internal editor;
    - dialogs;
    - quick file search;
    - location menu;
    - main menu;
    - other menus;
    - help window;
    - info panel;
    - quick view panel;
    - tree panel;
    - user menu;
    - screen grabber, vertical menus.

    It is impossible to assign a macro command to an already used hotkey. When
such an attempt is made, a warning message will appear telling that the macro
command that is assigned to this hotkey will be deleted.

    This way you can have identical hotkeys for different macro commands only
in different areas of execution.


@KeyMacroAssign
$ #Macro command: hotkeys#
    A ~macro command~@KeyMacro@ can be assigned to:

    1. any key;
    2. any key combination with #Ctrl#, #Alt# and #Shift# modifiers;
    3. any key combination with two modifiers.
       FAR2L allows to use the following double modifiers:
       #Ctrl-Shift-<key>#, #Ctrl-Alt-<key># and #Alt-Shift-<key>#

    A macro command #can't# be assigned to the following key combinations:
#Alt-Ins#, #Ctrl-<.>#, #Ctrl-Shift-<.>#, #Ctrl-Alt#, #Ctrl-Shift#, #Shift-Alt#,
#Shift-<symbol>#.

    It is impossible to enter some key combinations (in particular #Enter#,
#Esc#, #F1#, #Ctrl-F5#, #MsWheelUp# and #MsWheelDown# with #Ctrl#, #Shift#,
#Alt#) in the hotkey assignment dialog because of their special meanings. To
assign a macro command to such key combination, select it from the dropdown
list.

@ChangeLocationConfig
$ #Change location configuration#
    This dialog can be opened by pressing F9 key in Location menu opened by Alt+F1/F2.
    Here you can choose specific kind of items to be included in change Location
menu: #mountpoints#, #bookmarks# and #plugins#.
    Also you can customize mountpoints items by specifying wildcards exceptions
and changing templates of what should be included into additional columns.
    Following abbreviations can be used there to represent values:
    #$T# - total disk space
    #$U# - used disk space
    #$F# - free disk space
    #$A# - disk space available for non-privileged user
    #$u# - percents space used of total
    #$f# - percents space free of total
    #$a# - percents space available of total
    #$N# - filesystem name
    #$D# - device from which filesystem is mounted 
    #$S# - filesystem status, single character that can be
       ! - for readonly FS
       ? - for erroring/unresponsive FS
       <space> - for normally mounted and accessible FS
    Following abbreviations can be used there for extra alignment:
    #$<# - pad word on the left with spaces so its length will be same as longest word at same place
    #$># - pad word on the right with spaces so its length will be same as longest word at same place

@KeyMacroRecPlay
$ #Macro command: recording and playing-back#
    A ~macro command~@KeyMacro@ can be played-back in one of the two following
modes:

    1. General mode: keys pressed during the recording or the
       playing-back #will be# sent to plugins.

    2. Special mode: keys pressed during the recording or the
       playing-back #will not be# sent to plugins that intercept
       editor events.

    For example, if some plugin processes the key combination - #Ctrl+A#, then
in the special mode this plugin will not receive focus and will not do what it
usually does as a reaction to this combination.

    Creation of a macro command is achieved by the following actions:

    1. To start recording a macro command

       Press #Ctrl-<.># (#Ctrl# and a period pressed together) to record
       a macro in the general mode or #Ctrl-Shift-<.># (#Ctrl#, #Shift# and
       a period pressed together) to record a macro in the special
       mode.

       As the recording begins the symbol '\4FR\-' will appear at the
       upper left corner of the screen.

    2. Contents of the macro command.

       All keys pressed during the recording will be saved with the
       following exceptions:

       - only keys processed by FAR2L will be saved. Meaning that if
         during the macro recording process an external program is
         run inside the current console then only the keys pressed
         before the execution and after completion of that program
         will be saved.

    3. To finish recording the macro command.

       To finish a macro recording there are special key
       combinations. Because a macro command can be additionally
       configured there are two such combinations: #Ctrl-<.># (#Ctrl#
       and a period pressed together) and #Ctrl-Shift-<.># (#Ctrl#,
       Shift and a period pressed together). Pressing the first
       combination will end the recording of the macro command
       and will use the default settings for its playback. Pressing
       the second combination will end the recording of the macro
       command and a dialog showing macro command ~options~@KeyMacroSetting@
       will appear.

    4. Assign a hotkey to the macro command

    When the macro recording is finished and all the options are set the
    ~hotkey assignment~@KeyMacroSetting@ dialog will appear, where the hotkey that
    will be used to execute the recorded sequence can be set.

    Воспроизведение макроса сопровождается отображением в левом верхнем углу экрана символа '\2FP\-'.


@KeyMacroDelete
$ #Macro command: deleting a macro command#
    To delete a ~macro command~@KeyMacro@ an empty (containing no commands)
macro should be recorded and assigned the hotkey of the macro command that
needs to be deleted.

    This can be achieved by the following steps:

    1. Start recording a macro command (#Ctrl-<.>#)
    2. Stop recording a macro command (#Ctrl-<.>#)
    3. Enter or select in the hotkey assignment
       dialog the hotkey of the macro command that
       needs to be deleted.

    Attention: after deleting a macro command, the key combination
               (hotkey) that was used for its execution will begin
               to function as it was meant to, originally. That is
               if that key combination was somehow processed by FAR2L
               or some plugin then after deleting the macro command
               the key combination would be processed by them as in
               the past.


@KeyMacroSetting
$ #Macro command: settings#
    To specify additional ~macro command~@KeyMacro@ settings, start or finish
macro recording with #Ctrl-Shift-<.># instead of #Ctrl-<.># and select the
desired options in the dialog:

   #Sequence:#

    Allows to edit the recorded key sequence.

   #Allow screen output while executing macro#

    If this option is not set during the macro command execution FAR2L
does not redraw the screen. All the updates will be displayed when the macro
command playback is finished.

   #Execute after FAR2L start#

    Allows to execute the macro command immediately after the FAR2L is
started.

    The following execution conditions can be applied for the active and
passive panels:

     #Plugin panel#
         [x] - execute only if the current panel is a plugin panel
         [ ] - execute only if the current panel is a file panel
         [?] - ignore the panel type

     #Execute for folders#
         [x] - execute only if a folder is under the panel cursor
         [ ] - execute only if a file is under the panel cursor
         [?] - execute for both folders and files

     #Selection exists#
         [x] - execute only if there are marked files/directories
               on the panel
         [ ] - execute only if there are no marked files/directories
               on the panel
         [?] - ignore the file selection state

   Other execution conditions:

     #Empty command line#
         [x] - execute only if the command line is empty
         [ ] - execute only if the command line is not empty
         [?] - ignore the command line state

     #Selection block present#
         [x] - execute only if there is a selection block present
              in the editor, viewer, command line or dialog
              input line
         [ ] - execute only if there is no selection present
         [?] - ignore selection state


   Notes:

    1. Before executing a macro command, all of the above conditions are
checked.

    2. Some key combinations (including #Enter#, #Esc#, #F1# and #Ctrl-F5#,
#MsWheelUp#, #MsWheelDown# and other mouse keys combined with #Ctrl#, #Shift#, #Alt#) cannot be entered
directly because they have special functions in the dialog. To assign a macro
to one of those key combinations, select it from the drop-down list.


@KeyMacroLang
$ #Macro command: macro language#
    A primitive macro language is implemented in FAR2L. It allows to
insert logical commands into a simple keystrokes sequence, making macros (along
with ~plugins~@Plugins@) a powerful facility assisting in the everyday use of
FAR2L.

    Several of the available commands are listed below:
    #$Exit#         - stop macro playback
    #$Text#         - arbitrary text insertion
    #$XLat#         - transliteration function
    #$If-$Else#     - condition operator
    #$While#        - conditioned loop operator
    #$Rep#          - loop operator
    #%var#          - using variables
     and others...

    Addition of macro language commands to a ~macro~@KeyMacro@ can only be done
by manually editing the config file or by using special tools/plugins.

    Description of the macro language can be found in the accompanying
documentation.

    Online documentation:
    ~https://api.farmanager.com/ru/macro/~@https://api.farmanager.com/ru/macro/@

@KeyMacroList
$ #Макросы: Список установленных макросов#
    Ниже приведен список разделов, в которых можно узнать какие ~макросы~@KeyMacro@ 
действуют в текущей сессии Far Manager.

  ~Список переменных~@KeyMacroVarList@
  ~Список констант~@KeyMacroConstList@

  ~Общие макросы#~@KeyMacroCommonList@

  ~Панели~@KeyMacroShellList@
  ~Панель быстрого просмотра~@KeyMacroQViewList@
  ~Панель папок~@KeyMacroTreeList@
  ~Информационная панель~@KeyMacroInfoList@

  ~Быстрый поиск в панелях~@KeyMacroSearchList@
  ~Поиск папки~@KeyMacroFindFolderList@

  ~Диалоги~@KeyMacroDialogList@

  ~Главное меню~@KeyMacroMainMenuList@
  ~Меню выбора дисков~@KeyMacroDisksList@
  ~Меню пользователя~@KeyMacroUserMenuList@
  ~Прочие меню~@KeyMacroMenuList@
  
  ~Программа просмотра~@KeyMacroViewerList@
  ~Редактор~@KeyMacroEditList@
  
  ~Файл помощи#~@KeyMacroHelpList@
  
  ~Остальные области~@KeyMacroOtherList@

@KeyMacroVarList
$ #Макросы: Список переменных#
    Ниже приведен список переменных, которые можно использовать в макросах.

<!Macro:Vars!>

@KeyMacroConstList
$ #Макросы: Список констант#
    Ниже приведен список констант, которые можно использовать в макросах.

<!Macro:Consts!>

@KeyMacroCommonList
$ #Макросы: Общие#
    Ниже приведены комбинации макроклавиш, действующих везде.
    Описание для каждой макроклавиши берется из файла конфигурации (поле Description).

<!Macro:Common!>

@KeyMacroQViewList
$ #Макросы: Панель быстрого просмотра#
    Ниже приведены комбинации макроклавиш, действующих в панели быстрого просмотра.
    Описание для каждой макроклавиши берется из файла конфигурации (поле Description).

<!Macro:Common!>
<!Macro:Qview!>

@KeyMacroMainMenuList
$ #Макросы: Главное меню#
    Ниже приведены комбинации макроклавиш, действующих в главном меню.
    Описание для каждой макроклавиши берется из файла конфигурации (поле Description).

<!Macro:Common!>
<!Macro:MainMenu!>

@KeyMacroTreeList
$ #Макросы: Панель папок#
    Ниже приведены комбинации макроклавиш, действующих в панели папок.
    Описание для каждой макроклавиши берется из файла конфигурации (поле Description).

<!Macro:Common!>
<!Macro:Tree!>

@KeyMacroDialogList
$ #Макросы: Диалоги#
    Ниже приведены комбинации макроклавиш, действующих в диалогах.
    Описание для каждой макроклавиши берется из файла конфигурации (поле Description).

<!Macro:Common!>
<!Macro:Dialog!>

@KeyMacroInfoList
$ #Макросы: Информационная панель#
    Ниже приведены комбинации макроклавиш, действующих в информационной панели.
    Описание для каждой макроклавиши берется из файла конфигурации (поле Description).

<!Macro:Common!>
<!Macro:Info!>

@KeyMacroDisksList
$ #Макросы: Меню выбора дисков#
    Ниже приведены комбинации макроклавиш, действующих в меню выбора дисков.
    Описание для каждой макроклавиши берется из файла конфигурации (поле Description).

<!Macro:Common!>
<!Macro:Disks!>

@KeyMacroUserMenuList
$ #Макросы: Меню пользователя#
    Ниже приведены комбинации макроклавиш, действующих в пользовательском меню.
    Описание для каждой макроклавиши берется из файла конфигурации (поле Description).

<!Macro:Common!>
<!Macro:UserMenu!>

@KeyMacroShellList
$ #Макросы: Панели#
    Ниже приведены комбинации макроклавиш, действующих в файловых панелях.
    Описание для каждой макроклавиши берется из файла конфигурации (поле Description).

<!Macro:Common!>
<!Macro:Shell!>

@KeyMacroSearchList
$ #Макросы: Быстрый поиск в панелях#
    Ниже приведены комбинации макроклавиш, действующих в быстром поиске файловых панелей.
    Описание для каждой макроклавиши берется из файла конфигурации (поле Description).

<!Macro:Common!>
<!Macro:Search!>

@KeyMacroFindFolderList
$ #Макросы: Поиск папки#
    Ниже приведены комбинации макроклавиш, действующих в поиске папки.
    Описание для каждой макроклавиши берется из файла конфигурации (поле Description).

<!Macro:Common!>
<!Macro:FindFolder!>

@KeyMacroEditList
$ #Макросы: Редактор#
    Macro-commands available in the editor are listed below. Descriptions are read from the config file.

<!Macro:Common!>
<!Macro:Editor!>

@KeyMacroViewerList
$ #Макросы: Программа просмотра#
    Macro-commands available in the viewer are listed below. Descriptions are read from the config file.

<!Macro:Common!>
<!Macro:Viewer!>

@KeyMacroMenuList
$ #Макросы: Прочие меню#
    Ниже приведены комбинации макроклавиш, действующих в прочих меню.
    Описание для каждой макроклавиши берется из файла конфигурации (поле Description).

<!Macro:Common!>
<!Macro:Menu!>

@KeyMacroHelpList
$ #Макросы: Файл помощи#
    Ниже приведены комбинации макроклавиш, действующих в файле помощи.
    Описание для каждой макроклавиши берется из файла конфигурации (поле Description).

<!Macro:Common!>
<!Macro:Help!>

@KeyMacroOtherList
$ #Макросы: Остальные области#
    Ниже приведены комбинации макроклавиш, действующих в других областях: копировании текста с экрана, вертикальных меню.
    Описание для каждой макроклавиши берется из файла конфигурации (поле Description).

<!Macro:Common!>
<!Macro:Other!>

@Index
$ #Index help file#
<%INDEX%>
