mydir=WorkDir()
profile=mydir + "/profile"
left=mydir + "/left"
right=mydir + "/right"

StartApp(["--tty", "--nodetect", "--mortal", "-u", profile, "-cd", left, "-cd", right]);
ExpectString("Help - FAR2L", 0, 0, -1, -1, 10000);
status = AppStatus();

TypeEscape()
TypeDown()
TypeFKey(3)
ExpectString("left/viewme.txt", 0, 0, -1, -1, 10000)

Sync(10000)

TypePageDown()
Sync(10000)
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, mydir + '/test1.txt')

TypePageDown()
Sync(10000)
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, mydir + '/test2.txt')

TypeDown()
Sync(10000)
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, mydir + '/test3.txt')

TypeHome()
Sync(10000)
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, mydir + '/test4.txt')

TypeFKey(7)
ExpectString("═══ Search ═══", 0, 0, -1, -1, 10000)
TypeText("::setselectpos")
TypeEnter()

Sync(10000)
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, mydir + '/test5.txt')

TypeUp()
Sync(10000)
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, mydir + '/test6.txt')

TypeUp()
Sync(10000)
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, mydir + '/test7.txt')

TypeEscape()

TypeFKey(10)
ExpectString("Do you want to quit FAR?", 0, 0, -1, -1, 10000)
TypeEnter()
ExpectAppExit(0, 10000)
0;
