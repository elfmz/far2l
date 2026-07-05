LoadJS("../common.js");
var dirs = SetupTestDirs("left-тест1", "right");

///////////////////
// First start - skip Help window and OSC52 dialog, press F10 expecting exit confirmation dialog
StartTestApp(dirs.profile, dirs.left, dirs.right, "left-тест1");
DismissHelpAndOSC52();
status = AppStatus();
ExitFar2lWithConfirm();

///////////////////
// Second start - there should no Help appeared automatically
StartTestApp(dirs.profile, dirs.left, dirs.right, "left-тест1", false);
ExitFar2lWithConfirm();

// Now lets disable exit confirmation and save settings
StartTestApp(dirs.profile, dirs.left, dirs.right, "left-тест1", false);

// disable exit confirmation
TypeFKey(9)
ExpectString("Left    Files    Commands    Options    Right", 0, 0, -1, -1, 10000);
TypeText("on")
ExpectString("══ Confirmations ══", 0, 0, -1, -1, 10000);
ToggleLAlt(true)
TypeText("x")
ToggleLAlt(false)
TypeEnter()
ExpectNoString("══ Confirmations ══", 0, 0, -1, -1, 10000);

// Shift+F9 to save settings
ToggleShift(true)
TypeFKey(9)
ToggleShift(false)
ExpectString("══ Save setup ══", 0, 0, -1, -1, 10000);
TypeEnter()
ExpectNoString("══ Save setup ══", 0, 0, -1, -1, 10000);

// exit without confirmation now
TypeFKey(10)
ExpectAppExit(0, 10000)

///////////////////
// Third run just start and exit expecting no confirmation as such settings were saved
StartTestApp(dirs.profile, dirs.left, dirs.right, "left-тест1", false);
// exit without confirmation again
TypeFKey(10)
ExpectAppExit(0, 10000)


///////////////////
///////////////////
// CORNER CASES
///////////////////
///////////////////
var mydir = WorkDir();

///////////////////
// Corner case: Unicode directory names
// Verifies far2l handles non-ASCII paths in panel display
var dirsUC_profile = mydir + "/profile-unicode";
var dirsUC_left = mydir + "/кириллица";
var dirsUC_right = mydir + "/日本語";
MkdirsAll([dirsUC_profile, dirsUC_left, dirsUC_right], 0o700);
StartTestApp(dirsUC_profile, dirsUC_left, dirsUC_right, "кириллица");
DismissHelpAndOSC52();
ExitFar2lWithConfirm();

///////////////////
// Corner case: Space and dot in directory name
// Verifies path handling with whitespace and special characters
var dirsSD_profile = mydir + "/profile-spacedot";
var dirsSD_left = mydir + "/dir with spaces.v2";
var dirsSD_right = mydir + "/right-sd";
MkdirsAll([dirsSD_profile, dirsSD_left, dirsSD_right], 0o700);
StartTestApp(dirsSD_profile, dirsSD_left, dirsSD_right, "dir with spaces.v2");
DismissHelpAndOSC52();
ExitFar2lWithConfirm();

///////////////////
// Corner case: Nonexistent -cd directory
// Verifies far2l doesn't crash when -cd points to a missing path
var dirsNE_profile = mydir + "/profile-nonexist";
var dirsNE_left = mydir + "/nonexist/deep/path";
var dirsNE_right = mydir + "/right-ne";
MkdirsAll([dirsNE_profile, dirsNE_right], 0o700);
// left does NOT exist — far2l should handle gracefully
StartApp(["--tty", "--nodetect", "--mortal", "-u", dirsNE_profile, "-cd", dirsNE_left, "-cd", dirsNE_right]);
Sleep(2000);
// Dismiss any dialogs that may appear
TypeEscape();
Sleep(500);
TypeEscape();
Sleep(500);
// Exit
TypeFKey(10);
Sleep(500);
TypeEnter();
ExpectAppExit(0, 10000);

///////////////////
// Corner case: Same directory for both panels
// Verifies far2l handles identical -cd paths without crash
var dirsSame_profile = mydir + "/profile-same";
var dirsSame_shared = mydir + "/same-dir";
MkdirsAll([dirsSame_profile, dirsSame_shared], 0o700);
StartTestApp(dirsSame_profile, dirsSame_shared, dirsSame_shared, "same-dir");
DismissHelpAndOSC52();
ExitFar2lWithConfirm();

///////////////////
// Corner case: Escape on quit confirmation dialog
// Verifies Escape cancels the dialog rather than exiting
var dirsEsc_profile = mydir + "/profile-escape";
var dirsEsc_left = mydir + "/left-esc";
var dirsEsc_right = mydir + "/right-esc";
MkdirsAll([dirsEsc_profile, dirsEsc_left, dirsEsc_right], 0o700);
StartTestApp(dirsEsc_profile, dirsEsc_left, dirsEsc_right, "left-esc");
DismissHelpAndOSC52();
// Open quit dialog
TypeFKey(10);
ExpectString("Do you want to quit FAR?", 0, 0, -1, -1, 10000);
// Escape should cancel, not exit
TypeEscape();
Sleep(500);
// Verify quit dialog is gone — we're back at the panel
ExpectNoString("Do you want to quit FAR?", 0, 0, -1, -1, 2000);
// Exit properly
ExitFar2lWithConfirm();

///////////////////
// Corner case: F10 during Help window
// Verifies F10 closes Help (not triggers exit) on first start
var dirsF10H_profile = mydir + "/profile-f10help";
var dirsF10H_left = mydir + "/left-f10h";
var dirsF10H_right = mydir + "/right-f10h";
MkdirsAll([dirsF10H_profile, dirsF10H_left, dirsF10H_right], 0o700);
// First start — Help appears
StartApp(["--tty", "--nodetect", "--mortal", "-u", dirsF10H_profile, "-cd", dirsF10H_left, "-cd", dirsF10H_right]);
ExpectString("Help - FAR2L", 0, 0, -1, -1, 10000);
// Press F10 while Help is visible — should close Help, not trigger exit
TypeFKey(10);
Sleep(500);
// Help should be gone
ExpectNoString("Help - FAR2L", 0, 0, -1, -1, 2000);
// No exit dialog should appear
ExpectNoString("Do you want to quit FAR?", 0, 0, -1, -1, 2000);
// Dismiss OSC52 if present
BeCalm();
var r_f10h = ExpectString("OSC52", 0, 0, -1, -1, 2000);
BePanic();
if (r_f10h.I < 1) {
    TypeEnter();
    Sleep(500);
}
ExitFar2lWithConfirm();

///////////////////
// Corner case: Rapid F10 presses
// Verifies no crash or undefined state from multiple F10 in quick succession
var dirsRF_profile = mydir + "/profile-rapidf10";
var dirsRF_left = mydir + "/left-rf10";
var dirsRF_right = mydir + "/right-rf10";
MkdirsAll([dirsRF_profile, dirsRF_left, dirsRF_right], 0o700);
StartTestApp(dirsRF_profile, dirsRF_left, dirsRF_right, "left-rf10");
DismissHelpAndOSC52();
// Send F10 three times rapidly — first opens quit dialog, rest are consumed
TypeFKey(10);
Sleep(100);
TypeFKey(10);
Sleep(100);
TypeFKey(10);
Sleep(500);
// Verify no crash — quit dialog should be showing
ExpectString("Do you want to quit FAR?", 0, 0, -1, -1, 5000);
TypeEnter();
ExpectAppExit(0, 10000);

///////////////////
// Corner case: Read-only profile directory
// Verifies far2l starts and exits cleanly when profile is not writable
var dirsRO_profile = mydir + "/profile-readonly";
var dirsRO_left = mydir + "/left-ro";
var dirsRO_right = mydir + "/right-ro";
MkdirsAll([dirsRO_profile, dirsRO_left, dirsRO_right], 0o700);
// Initialize profile with default settings
StartTestApp(dirsRO_profile, dirsRO_left, dirsRO_right, "left-ro");
DismissHelpAndOSC52();
ExitFar2lWithConfirm();
// Make profile directory read-only
Chmod(dirsRO_profile, 0o555);
// Start with read-only profile — should not crash
StartTestApp(dirsRO_profile, dirsRO_left, dirsRO_right, "left-ro", false);
Sleep(500);
// Restore permissions for cleanup
Chmod(dirsRO_profile, 0o700);
// Exit
TypeFKey(10);
ExpectString("Do you want to quit FAR?", 0, 0, -1, -1, 10000);
TypeEnter();
ExpectAppExit(0, 10000);
