LoadJS("../common.js");
var dirs = SetupTestDirs();

StartTestApp(dirs.profile, dirs.left, dirs.right);
DismissHelpAndOSC52();

// Enable kitty keyboard protocol (CSI = 1 u), then run cat and wait for EOF.
// If Ctrl+D is sent as raw control byte (0x04), cat exits and the marker is printed.
// If Ctrl+D is sent as kitty sequence (\e[4;5u), cat never exits and the test times out.
TypeText("printf '\\033[=1u' > /dev/tty; cat; echo \"kittyeof\" | tr 'a-z' 'A-Z'")
TypeEnter()
Sync(5000)
ToggleLCtrl(true)
TypeVK(0x44)
ToggleLCtrl(false)
ExpectString("KITTYEOF", 0, 0, -1, -1, 10000)
Sync(5000)

// Wait for far2l to return to panel mode before typing exit command.
ExpectString("left", 0, 0, -1, -1, 10000)
Sync(5000)
TypeText("exit far")
TypeEnter()
ExpectAppExit(0, 10000)
