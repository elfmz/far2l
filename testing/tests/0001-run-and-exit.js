mydir=WorkDir() + "/run-and-exit"
profile=mydir + "/profile"
left=mydir + "/left"
right=mydir + "/right"
MkdirsAll([profile, left, right], 0700)
StartApp(["--tty", "--nodetect", "--mortal", "-u", profile, "-cd", left, "-cd", right]);
ExpectStringOrDie("/run-and-exit/left", 0, 0, -1, -1, 10000);
ExpectStringOrDie("Help - FAR2L", 0, 0, -1, -1, 10000);
TypeEscape(10)
status = AppStatus();
LogInfo("Status: Width=" + status.Width + " Height=" + status.Width + " Title: " + status.Title);
cell = ReadCell(0, 0);
LogInfo("TopCell: Fore=" + cell.Fore + " Back=" + cell.Back + " Text: " + cell.Text);
cell = ReadCell(0, status.Height - 2);
LogInfo("CmdCell: Fore=" + cell.Fore + " Back=" + cell.Back + " Text: " + cell.Text);
lines = SurroundedLines(1, 1, "║═│─", " \t")
LogInfo("Left panel:" + lines)
lines = SurroundedLines(status.Width - 2, 1, "║═│─", " \t")
LogInfo("Right panel:" + lines)
TypeFKey(10)
//TTYWrite("\x1b[21~");
ExpectStringOrDie("Do you want to quit FAR?", 0, 0, -1, -1, 10000)
//TTYWrite("\r\n");
TypeEnter()
ExpectAppExit(0, 10000)
0;
