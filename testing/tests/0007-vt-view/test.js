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

// Ctrl+O and wait until panels will be hidden
ToggleLCtrl(true)
TypeText("O")
ToggleLCtrl(false)
ExpectNoString("test-paneldir", 0, 0, 0, 1, 10000);

TypeText("echo 'VT' 'Shell' 'ready'")
TypeEnter()
ExpectString("VT Shell ready", 0, 0, 0, 0, 10000)
ExpectString("↑", -1, -2, 1, 1, 10000) // wait when command line input edit will be activated again

TypeText("cat '" + mydir + "/catme.txt'")
TypeEnter()
ExpectString("CATMECATMECATME", 0, 0, 0, 0, 10000)
ExpectString("↑", -1, -2, 1, 1, 10000) // wait when command line input edit will be activated again

// open viewer
TypeFKey(3)
ExpectString(".ans", 0, 0, -1, 1, 10000)
ExpectString("XXXXXXXXXXXXXXXXXXX 🔀 XXX: foobar hello world XXXXXXXXXXXXXXXXXXX", 0, 0, -1, -1, 10000)

// open search dialog
TypeFKey(7)
ExpectString("════ Search ═══", 0, 0, 0, 0, 10000)
TypeText("foobar")
TypeEnter()
// wait search dialog closed
ExpectNoString("════ Search ═══", 0, 0, 0, 0, 10000)

Sync(10000) // ensure viewer painted selection that caused glitch and went to idle

// ensure found text is not corrupted
ExpectString("XXXXXXXXXXXXXXXXXXX 🔀 XXX: foobar hello world XXXXXXXXXXXXXXXXXXX", 0, 0, 0, 0, 10000)

// close viewer
TypeEscape()
ExpectNoString(".ans", 0, 0, -1, 1, 10000)

// exit
TypeText("exit far")
TypeEnter()
ExpectAppExit(0, 10000)
0;
