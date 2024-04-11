far2l_WriteTTY("\x1b[21~");
far2l_WaitString("Do you want to quit FAR?", 0, 0, -1, -1, 10000)
far2l_Log("Got exit....")
far2l_WriteTTY("\r\n");
far2l_Bye("\r\n");
far2l_ExpectExit(0, 10000)
0;
