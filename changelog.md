# far2l changelog

Only significant user-side changes are listed here
(for all changes see history of [Commits](https://github.com/elfmz/far2l/commits/master/) and [Pull requests](https://github.com/elfmz/far2l/pulls?q=is%3Apr+is%3Aclosed)
or via `git log --no-merges --pretty=format:"%as: %B"`).

## Master (current development)
* Editor: Word wrap (like in Windows Notepad or HTML textareas). Toggled by **F3** or **Alt+W**
* Editor: Line numbers. Toggled by **Ctrl+F3**
* Editor: mouse selection (hold **Alt** to vertical selection)
* Tree panel: Option to exclude subtrees from scanning using a mask (default: hidden folders `.*`).
  Option to set the maximum recursive scanning depth (default: 4).
  **Right Arrow** expands excluded subtrees, and **Left Arrow** collapses subtree in focus, if it's already collapsed - navigates one level up.
  **Left Ctrl+1**...**Left Ctrl+0** expands all branches to the chosen depth (1...10).
* _New:_ Options of the special command `edit:[line,col]` for openening file with position
* _hexitor plugin_: fix broken layout with narrow window
* _ImageViewer plugin_:  New panel plugin (**F11**->**I** to open image/video file.
  Uses ImageMagick for graphics operations and ffmpeg for video preview,
  works in GUI and in TTY|F and TTY|k,
  see [#3028](https://github.com/elfmz/far2l/pull/3028#issuecomment-3508025346))).
* _edsort plugin_:  New plugin in editor (**F11**->Sort rows) to sort selected block of text at choosen column
* _arclite plugin_: More symlinks/hardlinks support: works with RAR/TAR/NTFS; fix option "Link path: Absolute/Relative" for symlinks when create archive (see [#3094](https://github.com/elfmz/far2l/pull/3094))
* _OpenWith plugin_: Added desktop-specific mimeapps.list support. Improved MIME detection (Magika AI & native globs2 pattern matching). Better compliance with XDG/Freedesktop specifications. Bugfixes and performance optimizations.
* Several bugfixes and improvements

## 2.7.0 beta (2025-10-26)
* Far2l internal virtual terminal: Now the original output of applications is preserved. The Far2l VT window applies dynamic formatting with correct line wrapping. Operations such as F3/F4 and copy/paste use the original, unwrapped lines.
* _New:_ new debug dump functionality (see [DUMPER.md](https://github.com/elfmz/far2l/blob/master/DUMPER.md))
* _New:_ far2l-cd.sh wrapper to enable external directory change to far2l's when it exit ([#2758](https://github.com/elfmz/far2l/issues/2758))
* _New:_ '$z' command prompt variable that returns the "{current git branch} " string; an empty string otherwise
* _New:_ Separate icons for WX versions of far2l and far2ledit
* _New:_ far:config as "Configuration editor" and far:about as "About FAR" in Commands menu
* _New:_ support (and warning) for pasting and executing multiline text in command line
* Editor: Display of various non-printable characters on **F5** (ShwSpc)
* Workaround for wxWigets Numeric Keypad regression in wxWidgets 3.2.7 only ([#2721](https://github.com/elfmz/far2l/issues/2721))
* Actions recorded in commands history are configured in the AutoComplete & History dialog
* _NetRocks plugin_: Add support of libssh SSH_OPTIONS_PROXYCOMMAND option
* _NetRocks plugin_: Fix AWS S3 1000 files limit via pagination
* _NetRocks plugin_: Enable smb protocol in macOS builds
* _Temporary panel plugin_: Show file groups
* _colorer plugin_: Update colorer schemes to v1.2.0.90
* _colorer plugin_: Update colorer library to v1.5.0-22.08.2025
* _colorer plugin_: Added features for easier modification of the set and behavior of the user's hrc/hrd files, without editing the supplied base set.
* _colorer plugin_: Improved performance around logging
* _colorer plugin_: Fix read default-back/default-fore params
* _python plugin_: fixes and new subplugin **uedreplace**
* _multiarc plugin_: Update bundled 7z sources to 2501
* _multiarc plugin_: Update bundled unrar sources to 7.13
* _arclite plugin_: New plugin for archives processing
  (now as experimental version which partially more effective then multiarc;
  arclite disabled by default, to enable manually turn on
  F9->Options->Plugins configuration->ArcLite->[x] Enable Arclite plugin)
* _hexitor plugin_: Hex editor (ported from far3) + preliminary support for viewing and editing UTF-8 characters
* _OpenWith plugin_: New plugin provides a context-aware menu to open the currently selected file with an appropriate application
* Several bugfixes

## 2.6.5 beta (2025-03-30)
* _New:_ Different desktop files for launch WX (GUI) `--notty` and TTY `--tty`
* _New:_ Horizontal panels layout (toggle horizontal-vertical by **Ctrl+,**)
* _New:_ Groups of file masks
* _New:_ "Files highlighting and sort groups" dialog short/full view by **Ctrl+M**
* _New:_ Customize Size column in panels (all by dialog or **Ctrl+Alt+D**, symlinks by **Ctrl+Alt+L**)
* More customize dirs/files markers in panels (by dialog or **Ctrl+Alt+M** and **Ctrl+Alt+N**)
* _New:_ Link item in File menu (in addition to the usual **Alt+F6**)
* File attributes dialog (by **Ctrl+A**): ability to show "uname"/"gname" or "uid: uname"/"gid: gname"
* _New:_ Chattr / chflags with all flags for single file (in File menu or by **Ctrl+Alt+A**)
* _New:_ Python-subplugin: copy/paste files via clipboard - wx ONLY version - with support for gnome clipboard types, works with gnome files/nautilus
* _New:_ At 1st run detect Russian locale
* _New:_ At 1st run show OSC52 info (if need)
* _New:_ Far colors moved to farcolors.ini
* Help actualization and improvements
* Info panel: New EditorConfig block
* _colorer plugin_: Update colorer schemes to v1.2.0.62
* _colorer plugin_: Update colorer library to v1.4.1-24.01.2025
* _Inside plugin_: Add PE format, add png, ogg, m4a and Mach-O support
* _multiarc plugin_: Removed PCRE library dependencies
* _NetRocks plugin_: Explicit SSH algorithms options
* _NetRocks plugin_: Add AWS S3 protocol support (_optional compilation if AWSSDK installed_)
* _python plugin_: New subplugin **hex editor**
* _python plugin_: Removed debugpy dependencies
* _Temporary panel plugin_: Significant fixes, tweaks and improvements
* optional ability to use icu available on build system or target
* Several bugfixes

## 2.6.4 beta (2024-11-18)
* _New:_ icons and a desktop file for far2ledit
* _New:_ Default Files highlighting with dirs/files markers a la `mc` and `ls -F`
* _New:_ Panels customization align filenames by marks (by dialog and **Ctrl+Alt+M**)
* Info panel: add hide/show blocks + additions in file systems info and tuning git status
* _New:_ **Ctrl-Alt-\\** quick jump to the mount point of the current folder's file system
* Enable esc expiration if we've got no TTY|X or got TTY|X without Xi (`--ee` no need more)
* _New:_ Add special commands `edit:<` and `view:<` for grab redirect output
* _New:_ Extend `--nodetect` option to cover win32/iTerm2/kovidgoyal's kitty modes
* _New:_ Add bash-completion
* _colorer plugin_: Update colorer lib to v1.4.1
* _colorer plugin_: Change logger library
* _multiarc plugin_: Update bundled 7z sources to 2408
* _multiarc plugin_: Update bundled unrar sources to 7.0.9
* _python plugin_: New subplugins
* _python plugin_: Build changes and requirements
* Several bugfixes

## 2.6.3 beta (2024-07-26)
* Panels resize by **Ctrl+(Shift+)Down** allows to hide command line, subsequently allowing fast file find without pressing **Alt**
* Several bugfixes

## 2.6.2 beta (2024-07-16)
* _New:_ Case (in)sensitive option for file masks in Find file
* _New:_ Case (in)sensitive option for Compare folders and (De)Select in Panels
* _New:_ Attributes dialog (by **Ctrl+A**) - dynamically show marks (\*) near changed fields
* _New:_ Option to disable automatic highlights in history lists
* _New:_ Commands History - delete duplicates if Name and Path equal (customization via System settings)
* _New:_ Datetime format customization via Interface settings
* _New:_ RGB in far2l Palette
* Several bugfixes

## 2.6.1 beta (2024-04-14)
## 2.6.0 beta (2024-02-19)
## 2.5.3 beta (2023-11-05)
## 2.5.2 beta (2023-08-15)
## 2.5.1 beta (2023-05-28)
## 2.5.0 beta (2023-01-15)
## 2.4.1 beta (2022-09-25)
## 2.4.0 beta (2022-01-12)
