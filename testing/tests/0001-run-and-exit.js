mydir=WorkDir()
profile=mydir + "/profile"
left=mydir + "/left-fgdfgfd"
right=mydir + "/right"
MkdirsAll([profile, left, right], 0700)
StartApp(["--tty", "--nodetect", "--mortal", "-u", profile, "-cd", left, "-cd", right]);
ExpectString("left-fgdfgfd", 0, 0, -1, -1, 10000);
ExpectString("Help - FAR2L", 0, 0, -1, -1, 10000);
TypeEscape(10)
status = AppStatus();
cell = ReadCell(0, 0);
cell = ReadCell(0, status.Height - 2);
lines = SurroundedLines(1, 1, "║═│─", " \t")
lines = SurroundedLines(status.Width - 2, 1, "║═│─", " \t")
TypeFKey(10)
ExpectString("Do you want to quit FAR?", 0, 0, -1, -1, 10000)
TypeEnter()
ExpectAppExit(0, 10000)

// Second start - there should no Help appeared automatically
StartApp(["--tty", "--nodetect", "--mortal", "-u", profile, "-cd", left, "-cd", right]);
ExpectString("left-fgdfgfd", 0, 0, -1, -1, 10000);
TypeFKey(10)
ExpectString("Do you want to quit FAR?", 0, 0, -1, -1, 10000)
TypeEnter()
ExpectAppExit(0, 10000)

// Now lets disable exit confirmation and save settings
StartApp(["--tty", "--nodetect", "--mortal", "-u", profile, "-cd", left, "-cd", right]);
ExpectString("left-fgdfgfd", 0, 0, -1, -1, 10000);

// disable exit confirmation
TypeFKey(9)
ExpectString("Left    Files    Commands    Options    Right", 0, 0, -1, -1, 10000);
TypeText("on")
ExpectString("══ Confirmations ══", 0, 0, -1, -1, 10000);
ToggleLAlt(true)
TypeText("x")
ToggleLAlt(false)
TypeEnter()
ExpectNoString("══ Confirmations ══", 0, 0, -1, -1, 10000);

// Shift+F9 to save settings
ToggleShift(true)
TypeFKey(9)
ToggleShift(false)
ExpectString("══ Save setup ══", 0, 0, -1, -1, 10000);
TypeEnter()
ExpectNoString("══ Save setup ══", 0, 0, -1, -1, 10000);

// exit without confirmation now
TypeFKey(10)
ExpectAppExit(0, 10000)

// Now lets disable exit confirmation and save settings
StartApp(["--tty", "--nodetect", "--mortal", "-u", profile, "-cd", left, "-cd", right]);
ExpectString("left-fgdfgfd", 0, 0, -1, -1, 10000);
// exit without confirmation again
TypeFKey(10)
ExpectAppExit(0, 10000)

0;
