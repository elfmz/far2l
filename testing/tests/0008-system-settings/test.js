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

// System settings
TypeFKey(9)
TypeText("o")
TypeEnter()

ExpectString("═ System settings ═")
/** ══════════════════ System settings ═══════════════════╗
	[x] Enable sudo privileges elevation                  ║
	    Password expiration (sec): 900                    ║
	    [x] Always confirm modify operations              ║
	[ ] Delete to Trash                                   ║
	    [x] Delete symbolic links                         ║
	[x] Scan symbolic links                               ║
	[ ] Use only files size in estimation                 ║
	[ ] Inactivity time                                   ║
	    15 minutes                                        ║
	When making a link the default suggestion is          ║
	    Symlink always                                   ↓║
	──────────────────────────────────────────────────────╢
	[x] Save commands history                             ║
	    Max history items:  512                           ║
	[x] Save folders history                              ║
	    Max history items:  512                           ║
	[x] Save view and edit history                        ║
	    Max history items:  512                           ║
	Remove duplicates in history:  by name and path     ↓ ║
	[x] Autohighlight in history                          ║
	──────────────────────────────────────────────────────╢
	[ ] Auto save setup                                   ║
	    [ ] Auto save panels state                        ║
	──────────────────────────────────────────────────────╢
	                  { OK } [ Cancel ]                   ║ */

LClickWhereFound(ExpectString("[x] Enable sudo"))
LClickWhereFound(ExpectString("[ ] Delete to Trash"))
LClickWhereFound(ExpectString("[x] Scan symbolic links"))
LClickWhereFound(ExpectString("[ ] Use only files size in estimation"))
LClickWhereFound(ExpectString("[ ] Inactivity time"))
LClickWhereFound(ExpectString("[ ] Auto save setup"))
LClickWhereFound(ExpectString("15 minutes"))
TypeText("42")
LClickWhereFound(ExpectString("{ OK }"))
TypeFKey(10)
ExpectString("Do you want to quit FAR?")
TypeEnter()
ExpectAppExit(0)

// start again and check settings changes really saved
StartApp(["--tty", "--nodetect", "--mortal", "-u", profile, "-cd", paneldir, "-cd", paneldir]);
status = AppStatus();

// System settings
TypeFKey(9)
TypeText("o")
TypeEnter()

ExpectString("═ System settings ═")
ExpectString("[ ] Enable sudo")
ExpectString("[x] Delete to Trash")
ExpectString("[ ] Scan symbolic links")
ExpectString("[x] Use only files size in estimation")
ExpectString("[x] Inactivity time")
ExpectString("[x] Auto save setup")

ExpectString("42 minutes")


TypeEscape()

TypeFKey(10)
ExpectString("Do you want to quit FAR?")
TypeEnter()
ExpectAppExit(0)

0;
