LoadJS("../common.js");
var dirs = SetupTestDirs();

StartTestApp(dirs.profile, dirs.left, dirs.right);
DismissHelpAndOSC52();

// Start an interactive bash session so the VT shell stays alive across
// multiple raw key injections (Ctrl+D, Ctrl+C).
StartBashShell();

// Enable kitty keyboard protocol (CSI = 1 u)
TTYWriteRaw("printf '\\033[=1u' > /dev/tty\n")
Sync(5000)

// Test 1: Ctrl+D as raw 0x04 exits cat
TTYWriteRaw("cat; echo KITTYEOF\n")
Sync(5000)
ToggleLCtrl(true)
TypeVK(0x44)
ToggleLCtrl(false)
ExpectString("KITTYEOF", 0, 0, -1, -1, 10000)

// Test 2: Ctrl+C as raw 0x03 kills cat with SIGINT
TTYWriteRaw("cat; echo CAT_INTERRUPTED\n")
Sync(5000)
ToggleLCtrl(true)
TypeVK(0x43)
ToggleLCtrl(false)
ExpectString("CAT_INTERRUPTED", 0, 0, -1, -1, 10000)

ExitBashAndFar2l()


///////////////////
///////////////////
// CORNER CASES
///////////////////
///////////////////

///////////////////
// Corner case: Ctrl+L clears screen, cat still running
// Verifies Ctrl+L (0x0C) is delivered as raw form-feed
StartTestApp(dirs.profile, dirs.left, dirs.right, "left", false);
StartBashShell();
TTYWriteRaw("printf '\\033[=1u' > /dev/tty\n")
Sync(5000)
TTYWriteRaw("cat\n")
Sync(5000)
// Ctrl+L clears screen — cat keeps running
ToggleLCtrl(true)
TypeVK(0x4C)
ToggleLCtrl(false)
Sync(5000)
// Type marker then Ctrl+D EOF — cat should echo it
TTYWriteRaw("mark_l\n")
Sync(5000)
ToggleLCtrl(true)
TypeVK(0x44)
ToggleLCtrl(false)
ExpectString("mark_l", 0, 0, -1, -1, 10000)
ExitBashAndFar2l()


///////////////////
// Corner case: Ctrl+C interrupts cat, next cat starts fresh
// Verifies signal delivery doesn't corrupt subsequent cat instances
StartTestApp(dirs.profile, dirs.left, dirs.right, "left", false);
StartBashShell();
TTYWriteRaw("printf '\\033[=1u' > /dev/tty\n")
Sync(5000)
// Start cat, interrupt it
TTYWriteRaw("cat\n")
Sync(5000)
ToggleLCtrl(true)
TypeVK(0x43)
ToggleLCtrl(false)
Sync(5000)
// Start a new cat — should work normally
TTYWriteRaw("cat; echo AFTER_INTERRUPT\n")
Sync(5000)
ToggleLCtrl(true)
TypeVK(0x44)
ToggleLCtrl(false)
ExpectString("AFTER_INTERRUPT", 0, 0, -1, -1, 10000)
ExitBashAndFar2l()


///////////////////
// Corner case: Multiple rapid Ctrl+C — no crash
// Verifies signal delivery under rapid succession
StartTestApp(dirs.profile, dirs.left, dirs.right, "left", false);
StartBashShell();
TTYWriteRaw("printf '\\033[=1u' > /dev/tty\n")
Sync(5000)
TTYWriteRaw("cat\n")
Sync(5000)
ToggleLCtrl(true)
TypeVK(0x43)
ToggleLCtrl(false)
// Short delays between rapid signals — Sync() can't track signal delivery
// to child processes, so bare Sleep is intentional here
Sleep(100)
ToggleLCtrl(true)
TypeVK(0x43)
ToggleLCtrl(false)
Sleep(100)
ToggleLCtrl(true)
TypeVK(0x43)
ToggleLCtrl(false)
Sync(5000)
// Start a new cat — shell should still be alive
TTYWriteRaw("cat; echo RAPID_C_OK\n")
Sync(5000)
ToggleLCtrl(true)
TypeVK(0x44)
ToggleLCtrl(false)
ExpectString("RAPID_C_OK", 0, 0, -1, -1, 10000)
ExitBashAndFar2l()


///////////////////
// Corner case: Ctrl+U clears line, Ctrl+D sends EOF
// Verifies Ctrl+U (0x15) is delivered as raw byte
StartTestApp(dirs.profile, dirs.left, dirs.right, "left", false);
StartBashShell();
TTYWriteRaw("printf '\\033[=1u' > /dev/tty\n")
Sync(5000)
TTYWriteRaw("cat\n")
Sync(5000)
// Type text, Ctrl+U to erase, Ctrl+D for EOF
TTYWriteRaw("garbage")
ToggleLCtrl(true)
TypeVK(0x55)
ToggleLCtrl(false)
Sync(5000)
ToggleLCtrl(true)
TypeVK(0x44)
ToggleLCtrl(false)
Sync(5000)
// Shell should be alive — start new cat
TTYWriteRaw("cat; echo CTRL_U_OK\n")
Sync(5000)
ToggleLCtrl(true)
TypeVK(0x44)
ToggleLCtrl(false)
ExpectString("CTRL_U_OK", 0, 0, -1, -1, 10000)
ExitBashAndFar2l()
