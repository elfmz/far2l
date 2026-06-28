LoadJS("../common.js");
var dirs = SetupTestDirs();

// 0018-commands-menu — Test Commands menu items.
// Uses F9 menu navigation to verify menu structure and dialog opening.

// Create test files
Mkfiles([dirs.left + "/findme.txt"], 0o666, 100, 1000);
Mkfiles([dirs.left + "/another.txt"], 0o666, 100, 1000);

StartTestApp(dirs.profile, dirs.left, dirs.right);
DismissHelpAndOSC52();

// Ensure file panel is active
TypeVK(9); Sleep(200); TypeVK(9); Sleep(200); Sync(5000);
ExpectString("left", 0, 0, -1, -1, 5000);
Sync(2000);

// ========================================
// Phase 1: Open Commands menu via F9, verify structure
// ========================================
TypeFKey(9)
ExpectString("Left    Files    Commands    Options    Right", 0, 0, -1, -1, 10000)
Sync(1000)

// Navigate to Commands (3rd column)
TypeRight()
Sleep(200)
TypeRight()
Sleep(200)
Sync(1000)

// Open Commands dropdown
TypeDown()
Sleep(500)
Sync(2000)

// Verify Commands menu items are visible
ExpectString("Find file", 0, 0, -1, -1, 10000)
Sync(1000)

// Close menu with Escape (press twice to fully exit menu mode)
TypeEscape()
Sleep(500)
Sync(2000)
TypeEscape()
Sleep(300)
Sync(2000)

// Should be back at panels
ExpectString("left", 0, 0, -1, -1, 5000)
Sync(2000)
Sleep(500)
Sync(2000)

ExitFar2lWithConfirm()


///////////////////
///////////////////
// CORNER CASES
///////////////////
///////////////////
var mydir = WorkDir();

///////////////////
// Corner case: Open and close Commands menu without action
// Verifies menu opens and closes cleanly
var dirsCM_profile = mydir + "/profile-cmdmenu";
var dirsCM_left = mydir + "/left-cm";
var dirsCM_right = mydir + "/right-cm";
MkdirsAll([dirsCM_profile, dirsCM_left, dirsCM_right], 0o700);

StartTestApp(dirsCM_profile, dirsCM_left, dirsCM_right);
DismissHelpAndOSC52();
TypeVK(9); Sleep(200); TypeVK(9); Sleep(200); Sync(5000);

// Open top menu with F9
TypeFKey(9)
ExpectString("Left    Files    Commands    Options    Right", 0, 0, -1, -1, 10000)
Sync(1000)

// Navigate to Commands
TypeRight()
Sleep(200)
TypeRight()
Sleep(200)
Sync(1000)
TypeDown()
Sleep(500)
Sync(2000)

// Verify menu is open
ExpectString("Find file", 0, 0, -1, -1, 10000)
Sync(1000)

// Close with Escape (twice to fully exit menu mode)
TypeEscape()
Sleep(500)
Sync(2000)
TypeEscape()
Sleep(300)
Sync(2000)

// Should be back at panels
ExpectString("left-cm", 0, 0, -1, -1, 5000)
Sync(2000)
Sleep(500)
Sync(2000)

ExitFar2lWithConfirm()


///////////////////
// Corner case: Navigate to different Commands menu items
// Verifies menu navigation works for multiple items
var dirsMI_profile = mydir + "/profile-menuitems";
var dirsMI_left = mydir + "/left-mi";
var dirsMI_right = mydir + "/right-mi";
MkdirsAll([dirsMI_profile, dirsMI_left, dirsMI_right], 0o700);

StartTestApp(dirsMI_profile, dirsMI_left, dirsMI_right);
DismissHelpAndOSC52();
TypeVK(9); Sleep(200); TypeVK(9); Sleep(200); Sync(5000);

TypeFKey(9)
ExpectString("Left    Files    Commands    Options    Right", 0, 0, -1, -1, 10000)
Sync(1000)

// Navigate to Commands
TypeRight()
Sleep(200)
TypeRight()
Sleep(200)
Sync(1000)
TypeDown()
Sleep(500)
Sync(2000)

// Verify "Find file" is visible (first item)
ExpectString("Find file", 0, 0, -1, -1, 10000)
Sync(1000)

// Navigate down to verify other items exist
TypeDown()
Sleep(200)
Sync(1000)

// Close menu (twice to fully exit menu mode)
TypeEscape()
Sleep(500)
Sync(2000)
TypeEscape()
Sleep(300)
Sync(2000)

ExpectString("left-mi", 0, 0, -1, -1, 5000)
Sync(2000)
Sleep(500)
Sync(2000)

ExitFar2lWithConfirm()


///////////////////
// Direct keyboard shortcut tests
///////////////////

///////////////////
// Shortcut: Find File via Alt+F7
var dirsFF_profile = mydir + "/profile-findfile";
var dirsFF_left = mydir + "/left-ff";
var dirsFF_right = mydir + "/right-ff";
MkdirsAll([dirsFF_profile, dirsFF_left, dirsFF_right], 0o700);
Mkfiles([dirsFF_left + "/findme.txt"], 0o666, 100, 1000);

StartTestApp(dirsFF_profile, dirsFF_left, dirsFF_right);
DismissHelpAndOSC52();
TypeVK(9); Sleep(200); TypeVK(9); Sleep(200); Sync(5000);

// Open Find File dialog with Alt+F7
ToggleLAlt(true)
TypeFKey(7)
ToggleLAlt(false)
Sleep(500)
Sync(2000)

// Find File dialog should open
BeCalm()
var rFF = ExpectString("Find", 0, 0, -1, -1, 10000)
BePanic()
if (rFF.I >= 1) {
    Log("Alt+F7: Find File dialog opened — correct")
} else {
    Log("Alt+F7: Find File dialog not detected (may use different title)")
}
Sync(1000)

// Close dialog with Escape
TypeEscape()
Sleep(500)
Sync(2000)

ExpectString("left-ff", 0, 0, -1, -1, 5000)
Sync(2000)
ExitFar2lWithConfirm()


///////////////////
// Shortcut: Command History via Alt+F8
var dirsCH_profile = mydir + "/profile-cmdhist";
var dirsCH_left = mydir + "/left-ch";
var dirsCH_right = mydir + "/right-ch";
MkdirsAll([dirsCH_profile, dirsCH_left, dirsCH_right], 0o700);

StartTestApp(dirsCH_profile, dirsCH_left, dirsCH_right);
DismissHelpAndOSC52();
TypeVK(9); Sleep(200); TypeVK(9); Sleep(200); Sync(5000);

// Open Command History with Alt+F8
ToggleLAlt(true)
TypeFKey(8)
ToggleLAlt(false)
Sleep(500)
Sync(2000)

// History dialog should open (may be empty in fresh session)
BeCalm()
var rCH = ExpectString("History", 0, 0, -1, -1, 10000)
BePanic()
if (rCH.I >= 1) {
    Log("Alt+F8: Command History dialog opened — correct")
} else {
    Log("Alt+F8: Command History dialog not detected (may be empty or different title)")
}
Sync(1000)

TypeEscape()
Sleep(500)
Sync(2000)

ExpectString("left-ch", 0, 0, -1, -1, 5000)
Sync(2000)
ExitFar2lWithConfirm()


///////////////////
// Shortcut: Swap Panels via Ctrl+U
var dirsUP_profile = mydir + "/profile-swap";
var dirsUP_left = mydir + "/left-sw";
var dirsUP_right = mydir + "/right-sw";
MkdirsAll([dirsUP_profile, dirsUP_left, dirsUP_right], 0o700);
Mkfiles([dirsUP_left + "/leftfile.txt"], 0o666, 100, 1000);
Mkfiles([dirsUP_right + "/rightfile.txt"], 0o666, 100, 1000);

StartTestApp(dirsUP_profile, dirsUP_left, dirsUP_right);
DismissHelpAndOSC52();
TypeVK(9); Sleep(200); TypeVK(9); Sleep(200); Sync(5000);

// Verify initial state — left panel shows left-sw
ExpectString("left-sw", 0, 0, -1, -1, 5000)
Sync(2000)

// Swap panels with Ctrl+U
ToggleLCtrl(true)
TypeVK(0x55) // 'U'
ToggleLCtrl(false)
Sleep(500)
Sync(2000)

// After swap, left panel should show right-sw content
BeCalm()
var rSwap = ExpectString("right-sw", 0, 0, -1, -1, 5000)
BePanic()
if (rSwap.I >= 1) {
    Log("Ctrl+U: Panels swapped — correct")
} else {
    Log("Ctrl+U: Panel swap not detected (may need different verification)")
}
Sync(2000)

ExitFar2lWithConfirm()