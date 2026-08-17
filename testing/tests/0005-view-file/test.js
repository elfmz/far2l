mydir=WorkDir()
profile=mydir + "/profile"
left=mydir + "/left"
right=mydir + "/right"

StartApp(["--tty", "--nodetect", "--mortal", "-u", profile, "-cd", left, "-cd", right]);
ExpectString("Help - FAR2L", 0, 0, -1, -1, 10000);
TypeEscape(10)
ExpectString("OSC52", 0, 0, -1, -1, 10000);
status = AppStatus();

TypeEscape()
TypeDown()
TypeFKey(3)
ExpectString("left/viewme.txt", 0, 0, 0, 0, 10000)

TypePageDown()
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, mydir + '/test1.txt')

TypePageDown()
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, mydir + '/test2.txt')

TypeDown()
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, mydir + '/test3.txt')

TypeHome()
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, mydir + '/test4.txt')

TypeFKey(7)
ExpectString("═══ Search ═══", 0, 0, 0, 0, 10000)
TypeText("::setselectpos")
TypeEnter()

BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, mydir + '/test5.txt')

TypeUp()
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, mydir + '/test6.txt')

TypeUp()
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, mydir + '/test7.txt')

TypeEscape()

TypeFKey(10)
ExpectString("Do you want to quit FAR?", 0, 0, 0, 0, 10000)
TypeEnter()
ExpectAppExit(0, 10000)
0;
