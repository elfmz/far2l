mydir=WorkDir()
profile=mydir + "/profile"
left=mydir + "/left"
right=mydir + "/right"

StartAppWithSize(["--tty", "--nodetect", "--mortal", "-u", profile, "-cd", left, "-cd", right], 95, 24);
ExpectString("Help - FAR2L", 0, 0, 0, 0, 10000);
TypeEscape(10)
ExpectString("OSC52", 0, 0, 0, 0, 10000);
status = AppStatus();

TypeEscape()
TypeDown()
TypeFKey(3)
ExpectString("left/viewme.txt", 0, 0, 0, 0, 10000)

ToggleShift(true)
TypeFKey(2)
ToggleShift(false)

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

TypeHome()
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, mydir + '/test4.txt')

TypeFKey(7)
ExpectString("═══ Search ═══", 0, 0, 0, 0, 10000)
TypeText("VMenu::SetUserData")
TypeEnter()

BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, mydir + '/test8.txt')

TypeUp()
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, mydir + '/test9.txt')

TypeUp()
TypeUp()
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, mydir + '/test10.txt')

TypeDown()
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, mydir + '/test11.txt')

TypeDown()
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, mydir + '/test12.txt')

TypeDown()
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, mydir + '/test13.txt')

TypePageDown()
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, mydir + '/test14.txt')

TypePageUp()
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, mydir + '/test15.txt')

TypeEscape()

TypeFKey(10)
ExpectString("Do you want to quit FAR?", 0, 0, 0, 0, 10000)
TypeEnter()
ExpectAppExit(0, 10000)
0;
