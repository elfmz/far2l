mydir=WorkDir()
profile=mydir + "/profile"
paneldir=mydir + "/test-paneldir"
MkdirsAll([profile, paneldir], 0700)
StartApp(["--tty", "--nodetect", "--mortal", "-u", profile, "-cd", paneldir, "-cd", paneldir]);
ExpectString("Help - FAR2L");
TypeEscape()
ExpectString("OSC52");
TypeEscape()
status = AppStatus();

// Ctrl+O and wait until panels will be hidden, then check that startup banner is there
ExpectString("test-paneldir", 0, 0, 0, 1);
ToggleLCtrl(true)
TypeText("O")
ToggleLCtrl(false)
ExpectNoString("test-paneldir", 0, 0, 0, 1);
ExpectString("While typing command:");
ExpectString("While executing command:");
ExpectString("Text selected with mouse automatically copied to clipboard");


// Command line settings
TypeFKey(9)
TypeText("o")
TypeDown(9) // 9 times
TypeEnter()

ExpectString("═ Command line settings ═")
/** ╔═══════════ Command line settings ════════════╗
	║ [x] Save commands history                    ║
	║     Max history items:  512                  ║
	║ [ ] Persistent blocks                        ║
	║ [x] Del removes blocks                       ║
	║ [x] Ctrl+Enter inserts all selected items    ║
	║ [x] AutoComplete                             ║
	║ [x] Command output splitter                  ║
	║ Terminal log size limit (KB):  1024          ║
	║ Wait keypress before close                   ║
	║ On error                                 ↓   ║
	║ [ ] Set command line prompt format           ║
	║     $p$g                                     ║
	║ [ ] Use shell                                ║
	║     «user/bashrc.far2l                       ║
	║ [x] Show startup banner in built-in terminal ║
	╟──────────────────────────────────────────────╢
	║              { OK } [ Cancel ]               ║
	╚══════════════════════════════════════════════╝*/
LClickWhereFound(ExpectString("[x] Save commands history"))
LClickWhereFound(ExpectString("[ ] Persistent blocks"))
LClickWhereFound(ExpectString("[x] Del removes blocks"))
LClickWhereFound(ExpectString("[x] Ctrl+Enter"))
LClickWhereFound(ExpectString("[x] AutoComplete"))
LClickWhereFound(ExpectString("[x] Command output splitter"))
LClickWhereFound(ExpectString("1024  "))
TypeText("4321")
LClickWhereFound(ExpectString("[x] Show startup banner"))

LClickWhereFound(ExpectString("{ OK }"))

// Shift+F9 to save settings
ToggleShift(true)
TypeFKey(9)
ToggleShift(false)
ExpectString("══ Save setup ══");
TypeEnter()
ExpectNoString("══ Save setup ══");

// exit
TypeFKey(10)
ExpectString("Do you want to quit FAR?")
TypeEnter()
ExpectAppExit(0)

// start again and check settings changes really saved
StartApp(["--tty", "--nodetect", "--mortal", "-u", profile, "-cd", paneldir, "-cd", paneldir]);
status = AppStatus();

// Ctrl+O and wait until panels will be hidden, then check that startup banner is NOT there anymore
ExpectString("test-paneldir", 0, 0, 0, 1);
ToggleLCtrl(true)
TypeText("O")
ToggleLCtrl(false)
ExpectNoString("test-paneldir", 0, 0, 0, 1);
ExpectNoString("While typing command:");
ExpectNoString("While executing command:");
ExpectNoString("Text selected with mouse automatically copied to clipboard");

// Command line settings
TypeFKey(9)
TypeText("o")
TypeDown(9) // 9 times
TypeEnter()

ExpectString("═ Command line settings ═")
ExpectString("[ ] Save commands history")
ExpectString("[x] Persistent blocks")
ExpectString("[ ] Del removes blocks")
ExpectString("[ ] Ctrl+Enter")
ExpectString("[ ] AutoComplete")
ExpectString("[ ] Command output splitter")
ExpectString("4321  ")
ExpectString("[ ] Show startup banner")

LClickWhereFound(ExpectString("[ Cancel ]"))

// exit
TypeFKey(10)
ExpectString("Do you want to quit FAR?")
TypeEnter()
ExpectAppExit(0)

0;
