StartApp(["--tty", "--nodetect", "--mortal", "-cd", "/usr/include"]);
ExpectString("/usr/include", 0, 0, -1, -1, 10000);
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
WriteTTY("\x1b[21~");
ExpectString("Do you want to quit FAR?", 0, 0, -1, -1, 10000)
WriteTTY("\r\n");
ExpectAppExit(0, 10000)
0;
