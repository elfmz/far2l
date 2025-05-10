# far2l changelog

Only significant user-side changes are listed here (for all changes see history of [Commits](https://github.com/elfmz/far2l/commits/master/) and [Pull requests](https://github.com/elfmz/far2l/pulls?q=is%3Apr+is%3Aclosed)).

## Master (current development)
* _NetRocks plugin_: Add support of libssh SSH_OPTIONS_PROXYCOMMAND option
* Editor: Display of various non-printable characters on **F5** (ShwSpc)
* Workaround for wxWigets Numeric Keypad regression in wxWidgets 3.2.7 only
* _Temporary panel plugin_: Show file groups
* Several bugfixes
* _colorer plugin_: Update colorer schemes to v1.2.0.68
* _colorer plugin_: Update colorer library to v1.4.1-10.05.2025

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
## 2.5.3 beta (2023-11-05)
## 2.5.2 beta (2023-08-15)
## 2.5.1 beta (2023-05-28)
## 2.5.0 beta (2023-01-15)
## 2.4.1 beta (2022-09-25)
## 2.4.0 beta (2022-01-12)
