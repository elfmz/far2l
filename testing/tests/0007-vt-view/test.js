// verifies https://github.com/elfmz/far2l/commit/4c719ba0dc9bb5770af90f2be0a11bb7f1181524
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

// Ctrl+O and wait until panels will be hidden
ToggleLCtrl(true)
TypeText("O")
ToggleLCtrl(false)
ExpectNoString("test-paneldir", 0, 0, 0, 1);

TypeText("echo 'VT' 'Shell' 'ready'")
TypeEnter()
ExpectString("VT Shell ready")
ExpectString("↑", -1, -2, 1, 1) // wait when command line input edit will be activated again

TypeText("cat '" + mydir + "/catme.txt'")
TypeEnter()
ExpectString("CATMECATMECATME")
ExpectString("↑", -1, -2, 1, 1) // wait when command line input edit will be activated again

// open viewer
TypeFKey(3)
ExpectString(".ans", 0, 0, -1, 1)
ExpectString("XXXXXXXXXXXXXXXXXXX 🔀 XXX: foobar hello world XXXXXXXXXXXXXXXXXXX")

// open search dialog
TypeFKey(7)
ExpectString("════ Search ═══")
TypeText("foobar")
TypeEnter()
// wait search dialog closed
ExpectNoString("════ Search ═══")

// ensure found text is not corrupted
ExpectString("XXXXXXXXXXXXXXXXXXX 🔀 XXX: foobar hello world XXXXXXXXXXXXXXXXXXX")

// close viewer
TypeEscape()
ExpectNoString(".ans", 0, 0, -1, 1)

// exit
TypeText("exit far")
TypeEnter()
ExpectAppExit(0)
0;
