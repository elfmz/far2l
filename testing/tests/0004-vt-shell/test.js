mydir=WorkDir()
bashrc=mydir + "/bashrc.far2l"
profile=mydir + "/profile"
left=mydir + "/left-fgdfgfd"
right=mydir + "/right"
MkdirsAll([profile, left, right], 0700)
for (testrun = 1; testrun <= 2; testrun++) {
	StartApp(["--tty", "--nodetect", "--mortal", "-u", profile, "-cd", left, "-cd", right]);
	ExpectString("left-fgdfgfd");
	if (testrun == 1) {
		ExpectString("Help - FAR2L");
		TypeEscape()
		ExpectString("OSC52");
		TypeEscape()
	} else {
		Log("---- TESTRUN 2 (with input consuming bashrc) ----")
	}
	ExpectString("↑", -1, -2, 1, 1) // wait when command line input edit will be activated
	status = AppStatus();
	TypeText("echo 'VT' 'Shell' 'smoke' \"${BASHRC_FAR2L}test\"; false")
	TypeEnter()
	if (testrun == 1) {
		ExpectString("VT Shell smoke test")
	} else {
		ExpectString("VT Shell smoke Repro3171test")
	}
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

	////////////////

	// Command line settings
	TypeFKey(9)
	TypeText("o")
	TypeDown(9) // 9 times
	TypeEnter()

	ExpectString("═ Command line settings ═")
	/** ╔═══════════ Command line settings ════════════╗
	.............
		║ [ ] Use shell                                ║
		║     «user/bashrc.far2l                       ║
	..........................
		╟──────────────────────────────────────────────╢
		║              { OK } [ Cancel ]               ║
		╚══════════════════════════════════════════════╝*/
	if (testrun == 1) {
		found=ExpectString("[ ] Use shell")
		LClickWhereFound(found)
		LClick(found.Y + 4, found.Y + 1)
		Sync(1000)
		TypeHome()
		TypeDel(100)
		Sync(1000)
		TypeText("bash --init-file " + bashrc + " -i")
		Sync(1000)
		TypeTab()
		LClickWhereFound(ExpectString("{ OK }"))
		ExpectNoString("═ Command line settings ═")
		TypeEscape()
		ExpectString("↑", -1, -2, 1, 1) // wait when command line input edit will be activated again

		// Shift+F9 to save settings
		ToggleShift(true)
		TypeFKey(9)
		ToggleShift(false)
		ExpectString("══ Save setup ══");
		TypeEnter()
		ExpectNoString("══ Save setup ══");
	} else {
		ExpectString("[x] Use shell")
		LClickWhereFound(ExpectString("[ Cancel ]"))
		ExpectNoString("═ Command line settings ═")
	}

	TypeText("exit far")
	TypeEnter()
	ExpectAppExit(0)
}
0;
