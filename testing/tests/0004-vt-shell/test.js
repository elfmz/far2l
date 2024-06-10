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
TypeText("echo 'VT' 'Shell' 'smoke' 'test'; false")
TypeEnter()
ExpectString("VT Shell smoke test", 0, 0, -1, -1, 10000)
ExpectString("~~~~~~~~~~~~~~~~~~~", 0, 0, -1, -1, 10000)
TypeEscape()
TypeText("exit far")
TypeEnter()
ExpectAppExit(0, 10000)
0;
