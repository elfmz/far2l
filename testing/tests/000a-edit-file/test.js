mydir=WorkDir()
profile=mydir + "/profile"
left=mydir + "/left"
right=mydir + "/right"

StartApp(["--tty", "--nodetect", "--mortal", "-u", profile, "-cd", left, "-cd", right]);
ExpectString("Help - FAR2L");
TypeEscape(10)
ExpectString("OSC52");
status = AppStatus();

TypeEscape()
TypeDown()
TypeFKey(4)
ExpectString("left/left.txt")

ExpectString("Hello, world! 🌍✨")
TypeRight(5)
TypeBack()
ExpectNoString("Hello, world! 🌍✨")
ExpectString("Hell, world! 🌍✨")
TypeBack(4)
TypeText("Good morning")
ExpectString("Good morning, world! 🌍✨")
TypeIns()
TypeText(" people")
ExpectString("Good morning people! 🌍✨")
TypeIns()
TypeRight(3)
TypeText(" ")
ExpectString("Good morning people! 🌍 ✨")
TypeBack(2)
ExpectString("Good morning people! ✨")

TypeEscape()
ExpectString("File has been modified. Save?")
TypeEnter()
ExpectNoString("left/left.txt")

TypeFKey(4)
ExpectString("left/left.txt")
ExpectString("Good morning people! ✨")
ExpectNoString("Hello, world! 🌍✨")
TypeFKey(10)
ExpectNoString("left/left.txt")

TypeFKey(10)
ExpectString("Do you want to quit FAR?")
TypeEnter()
ExpectAppExit(0)
0;
