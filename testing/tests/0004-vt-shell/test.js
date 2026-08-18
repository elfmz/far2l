mydir=WorkDir()
profile=mydir + "/profile"
left=mydir + "/left-fgdfgfd"
right=mydir + "/right"
MkdirsAll([profile, left, right], 0700)
StartApp(["--tty", "--nodetect", "--mortal", "-u", profile, "-cd", left, "-cd", right]);
ExpectString("left-fgdfgfd");
ExpectString("Help - FAR2L");
TypeEscape(10)
ExpectString("OSC52");
TypeEscape(10)
ExpectString("↑", -1, -2, 1, 1) // wait when command line input edit will be activated
status = AppStatus();
TypeText("echo 'VT' 'Shell' 'smoke' 'test'; false")
TypeEnter()
ExpectString("VT Shell smoke test")
ExpectString("~~~~~~~~~~~~~~~~~~~")
ExpectString("1Help", 0, -1, 0, 1)
TypeEscape()

ExpectString("↑", -1, -2, 1, 1) // wait when command line input edit will be activated again

// hide panels
ExpectString("══╝╚══")
ToggleLCtrl(true)
TypeText("O")
ToggleLCtrl(false)
ExpectNoString("══╝╚══")

// disable autosync as its not dispatched while console is active, so should sync manually when need
AutoSync(0)

TypeText("ping localhost")
TypeEnter()
ExpectNoString("1Help", 0, -1, 0, 1)
ExpectString("localhost", 0, -2, 0, 0) // expecting ping output filling screen from bottom
ToggleLCtrl(true)
ToggleLAlt(true)
TypeText("Z")
ToggleLAlt(false)
ToggleLCtrl(false)
ExpectString("*** Command put to background")

ExpectString("↑", -1, -2, 1, 1) // wait when command line input edit will be activated again
TypeText("while sleep 1; do echo 'Test' 'Loop' 'Iteration'; done")
TypeEnter()
ExpectString("Test Loop Iteration")
ToggleLCtrl(true)
TypeText("C")
ToggleLCtrl(false)
ExpectString("^C")
ExpectString("↑", -1, -2, 1, 1) // wait when command line input edit will be activated again

TypeFKey(12)
ExpectString("1 ping@")

TypeText("1")
ExpectString("localhost", 0, -2, 0, 0) // expecting ping output filling screen from bottom
ToggleLCtrl(true)
TypeText("C")
ToggleLCtrl(false)


ExpectString("↑", -1, -2, 1, 1) // wait when command line input edit will be activated again
TypeText("exit far")
TypeEnter()
ExpectAppExit(0)
0;
