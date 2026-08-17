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
TypeText("exit far")
TypeEnter()
ExpectAppExit(0)
0;
