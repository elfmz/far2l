mydir=WorkDir()
profile=mydir + "/profile"
left=mydir + "/left-fgdfgfd"
right=mydir + "/right"
MkdirsAll([profile, left, right], 0700)

///////////////////
// First start - skip Help window, press F10 expecting exit confirmation dialog
StartApp(["--tty", "--nodetect", "--mortal", "-u", profile, "-cd", left, "-cd", right]);
ExpectString("left-fgdfgfd");
ExpectString("Help - FAR2L");
TypeEscape(10)
ExpectString("OSC52");
TypeEscape(10)
status = AppStatus();
TypeFKey(10)
ExpectString("Do you want to quit FAR?")
TypeEnter()

ExpectAppExit(0)

///////////////////
// Second start - there should no Help appeared automatically
StartApp(["--tty", "--nodetect", "--mortal", "-u", profile, "-cd", left, "-cd", right]);
ExpectString("left-fgdfgfd");
TypeFKey(10)
ExpectString("Do you want to quit FAR?")
TypeEnter()
ExpectAppExit(0)

// Now lets disable exit confirmation and save settings
StartApp(["--tty", "--nodetect", "--mortal", "-u", profile, "-cd", left, "-cd", right]);
ExpectString("left-fgdfgfd");

// disable exit confirmation
TypeFKey(9)
ExpectString("Left    Files    Commands    Options    Right");
TypeText("on")
ExpectString("══ Confirmations ══");
ToggleLAlt(true)
TypeText("x")
ToggleLAlt(false)
TypeEnter()
ExpectNoString("══ Confirmations ══");

// Shift+F9 to save settings
ToggleShift(true)
TypeFKey(9)
ToggleShift(false)
ExpectString("══ Save setup ══");
TypeEnter()
ExpectNoString("══ Save setup ══");

// exit without confirmation now
TypeFKey(10)
ExpectAppExit(0)

///////////////////
// Third run just start and exit expecting no confirmation as such settings were saved
StartApp(["--tty", "--nodetect", "--mortal", "-u", profile, "-cd", left, "-cd", right]);
ExpectString("left-fgdfgfd");
// exit without confirmation again
TypeFKey(10)
ExpectAppExit(0)

0;
