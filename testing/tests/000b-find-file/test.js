mydir=WorkDir()
profile=mydir + "/profile"
left=mydir + "/left"
right=mydir + "/right"

StartApp(["--tty", "--nodetect", "--mortal", "-u", profile, "-cd", left, "-cd", right]);
ExpectString("Help - FAR2L");
TypeEscape()
ExpectString("OSC52");
status = AppStatus();

TypeEscape()

ToggleLAlt(true)
TypeFKey(7)
ToggleLAlt(false)

ExpectString("═══ Find file")
LClickWhereFound(ExpectString("[ ] Search in archives"))

TypeEnter()
ExpectString("Search done.")
ExpectString("Found files: 101, folders: 3")

TypeEscape()
ExpectNoString("═══ Find file")

ToggleLAlt(true)
TypeFKey(7)
ToggleLAlt(false)

ExpectString("═══ Find file")
ExpectString("[x] Search in archives")

where = ExpectString("Containing text:")
LClick(where.X, where.Y + 1)
TypeText("test")
TypeEnter()
ExpectString("Search done. Found files: 11, folders: 0")

TypeFKey(4)
ExpectString("/CMakeLists.txt", 0, 0, 0, 1)
ExpectNoString("hello world 123")
TypeText("hello world 123")
TypeFKey(2)
ExpectString("══ Add to ")
TypeEnter()
ExpectNoString("══ Add to ")
ExpectNoString("*", 0, 0, 0, 1)
ExpectString("/CMakeLists.txt", 0, 0, 0, 1)
TypeEscape()
ExpectNoString("/CMakeLists.txt", 0, 0, 0, 1)
TypeEscape()
ExpectNoString("═══ Find file")

ToggleLAlt(true)
TypeFKey(7)
ToggleLAlt(false)

ExpectString("═══ Find file")
TypeEnter()
ExpectString("Search done. Found files: 11, folders: 0")
TypeFKey(3)
ExpectString("/CMakeLists.txt", 0, 0, 0, 1)
ExpectString("hello world 123")
TypeEscape()
ExpectNoString("/CMakeLists.txt", 0, 0, 0, 1)
TypeEscape()
ExpectNoString("═══ Find file")

TypeFKey(10)
ExpectString("Do you want to quit FAR?")
TypeEnter()
ExpectAppExit(0)
0;
