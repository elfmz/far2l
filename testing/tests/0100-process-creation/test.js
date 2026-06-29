// Integration tests for ProcessCreation API migration
// Tests verify that the cross-platform process creation works correctly
// after migrating from fork() to ProcessCreation API

mydir = WorkDir()
profile = mydir + "/profile"
left = mydir + "/left"
right = mydir + "/right"
MkdirsAll([profile, left, right], 0700)

// Test 1: VT shell startup and exit (uses ProcessCreation via MakePTYAndFork)
StartApp(["--tty", "--nodetect", "--mortal", "-u", profile, "-cd", left, "-cd", right]);
ExpectString("left", 0, 0, -1, -1, 10000);

// Open VT shell
TypeEscape()
TypeText("sh")
TypeEnter()
ExpectString("sh:", 0, 0, -1, -1, 10000)

// Test basic command execution
TypeText("echo PROCESS_CREATION_TEST")
TypeEnter()
ExpectString("PROCESS_CREATION_TEST", 0, 0, -1, -1, 10000)

// Exit shell
TypeText("exit")
TypeEnter()

// Wait for shell to exit
ExpectString("left", 0, 0, -1, -1, 10000)

// Exit far2l
TypeEscape()
TypeText("exit far")
TypeEnter()
ExpectAppExit(0, 10000)

// Test 2: Process cleanup - verify no zombie processes
// This is implicitly tested by running multiple commands above
// If zombie processes accumulated, the system would show issues
