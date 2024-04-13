StartApp(["--tty", "--nodetect", "--mortal", "-cd", "/usr/include"]);
ExpectString("/usr/include", 0, 0, -1, -1, 10000);
status = AppStatus();
LogInfo("Status: Width=" + status.Width + " Height=" + status.Width + " Title: " + status.Title);
WriteTTY("\x1b[21~");
ExpectString("Do you want to quit FAR?", 0, 0, -1, -1, 10000)
WriteTTY("\r\n");
ExpectAppExit(0, 10000)
0;
