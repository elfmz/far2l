// verifies https://github.com/elfmz/far2l/commit/4c719ba0dc9bb5770af90f2be0a11bb7f1181524
mydir=WorkDir()
profile=mydir + "/profile"
paneldir=mydir + "/test-paneldir"
MkdirsAll([profile, paneldir], 0700)
StartApp(["--tty", "--nodetect", "--mortal", "-u", profile, "-cd", paneldir, "-cd", paneldir]);
ExpectString("Help - FAR2L", 0, 0, 0, 0, 10000);
TypeEscape(10)
ExpectString("OSC52", 0, 0, -1, -1, 10000);
TypeEscape(10)
status = AppStatus();

// System settings
TypeFKey(9)
TypeText("o")
TypeEnter()

ExpectString("═ System settings ═", 0, 0, 0, 0, 10000)
/*	══════════════════ System settings ═══════════════════╗
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

LClickWhereFound(ExpectString("[x] Enable sudo", 0, 0, 0, 0, 1000))
LClickWhereFound(ExpectString("[ ] Delete to Trash", 0, 0, 0, 0, 1000))
LClickWhereFound(ExpectString("[x] Scan symbolic links", 0, 0, 0, 0, 1000))
LClickWhereFound(ExpectString("[ ] Use only files size in estimation", 0, 0, 0, 0, 1000))
LClickWhereFound(ExpectString("[ ] Inactivity time", 0, 0, 0, 0, 1000))
LClickWhereFound(ExpectString("[ ] Auto save setup", 0, 0, 0, 0, 1000))
Sync(10000)
LClickWhereFound(ExpectString("15 minutes"))
Sync(10000)
TypeText("42")
Sync(10000)
LClickWhereFound(ExpectString("{ OK }"))
Sync(10000)
TypeFKey(10)
ExpectString("Do you want to quit FAR?", 0, 0, 0, 0, 10000)
TypeEnter()
ExpectAppExit(0, 10000)

// start again and check settings changes really saved
StartApp(["--tty", "--nodetect", "--mortal", "-u", profile, "-cd", paneldir, "-cd", paneldir]);
status = AppStatus();

// System settings
TypeFKey(9)
TypeText("o")
TypeEnter()

ExpectString("═ System settings ═", 0, 0, 0, 0, 10000)
ExpectString("[ ] Enable sudo", 0, 0, 0, 0, 1000)
ExpectString("[x] Delete to Trash", 0, 0, 0, 0, 1000)
ExpectString("[ ] Scan symbolic links", 0, 0, 0, 0, 1000)
ExpectString("[x] Use only files size in estimation", 0, 0, 0, 0, 1000)
ExpectString("[x] Inactivity time", 0, 0, 0, 0, 1000)
ExpectString("[x] Auto save setup", 0, 0, 0, 0, 1000)

ExpectString("42 minutes")

0;
