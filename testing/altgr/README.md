# AltGr event regressions

Run `./testing/altgr/run.sh` with a C++17 compiler and wxWidgets development
files (`wx-config`) installed. `CXX` and `WX_CONFIG` can override these tools.

The test exercises the production `KeyTracker` and `wx2INPUT_RECORD` code with
real wxKeyEvent objects, using X11 keycodes for ISO_Level3_Shift and Alt_R.
It covers the default and enabled option, keycode zero, modifier-only key bar
state, lowercase Polish characters, ordinary Alt on us/ru, key release, and
forced input reset. On macOS it also tests Right Option and runs the non-Mac
translation paths with recorded X11 keycodes.

Display-independent services (keyboard LEDs, Touch Bar, input queue, scan-code
mapping, and the option getter) are stubbed. These checks do not exercise GTK
layout translation or WinPortPanel's OnKeyDown/OnChar dispatch. They do not
replace an X11/Xvfb integration run. In that run, check Polish AltGr+a -> ą and
AltGr+Shift+a -> Ą with the option both off and on, key bar titles while holding
AltGr alone, ordinary Right Alt shortcuts on us/ru with the option off, and
F-key/navigation shortcuts with the option on.
