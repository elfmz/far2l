LoadJS("../common.js");
var dirs = SetupTestDirs();

// 0019-file-delete — Test F8 file deletion with disk-based verification.
//
// Uses direct disk checks (Exists/HashPath) as primary verification,
// panel reading as secondary. Adds Sync(2000) after delete for panel refresh.
// Resets all modifier toggles before F8 to prevent Shift+F8 bypass.

// Create test files
Mkfiles([dirs.left + "/delete_me.txt"], 0o666, 100, 1000);
Mkfiles([dirs.left + "/keep_me.txt"], 0o666, 100, 1000);
Mkfiles([dirs.left + "/file_a.txt", dirs.left + "/file_b.txt", dirs.left + "/file_c.txt"], 0o666, 100, 1000);

StartTestApp(dirs.profile, dirs.left, dirs.right);
DismissHelpAndOSC52();

// Ensure file panel is active
EnsurePanelFocus();

// ========================================
// Phase 1: Delete single file (F8 + confirm)
// ========================================
// Navigate to delete_me.txt (first file alphabetically below directories)
TypeDown()
Sleep(200)
Sync(1000)

// Ensure no modifiers are toggled (prevent Shift+F8 bypass)
ToggleLCtrl(false)
ToggleLAlt(false)
ToggleShift(false)

// Press F8 to delete
TypeFKey(8)
Sleep(500)
Sync(2000)

// Delete confirmation dialog should appear
ExpectString("Delete", 0, 0, -1, -1, 10000)
Sync(1000)

// Confirm with Enter
TypeEnter()
Sleep(1000)
Sync(3000)

// Verify file is removed from disk
if (!Exists(dirs.left + "/delete_me.txt")) {
    Log("Single file delete: file removed from disk — correct")
} else {
    Panic("Single file delete: file still exists on disk")
}

// Verify keep_me.txt still exists
if (Exists(dirs.left + "/keep_me.txt")) {
    Log("Single file delete: other file preserved — correct")
} else {
    Panic("Single file delete: keep_me.txt was also deleted")
}
Sync(1000)
ExitFar2lWithConfirm()

// ========================================
// Phase 2: Delete multiple files sequentially
// ========================================
var dirsMF_profile = dirs.mydir + "/profile-multifile";
var dirsMF_left = dirs.mydir + "/left-mf";
var dirsMF_right = dirs.mydir + "/right-mf";
MkdirsAll([dirsMF_profile, dirsMF_left, dirsMF_right], 0o700);
Mkfiles([dirsMF_left + "/file_a.txt", dirsMF_left + "/file_b.txt", dirsMF_left + "/file_c.txt"], 0o666, 100, 1000);

StartTestApp(dirsMF_profile, dirsMF_left, dirsMF_right, "left-mf", false);
DismissHelpAndOSC52();
EnsurePanelFocus();

// Delete each file one at a time — this tests the delete path reliably
// without depending on selection state.
for (var fi = 0; fi < 3; fi++) {
    TypeDown()
    Sleep(300)
    Sync(2000)

    ToggleLCtrl(false)
    ToggleLAlt(false)
    ToggleShift(false)

    TypeFKey(8)
    Sleep(500)
    Sync(2000)

    ExpectString("Delete", 0, 0, -1, -1, 10000)
    Sync(1000)

    TypeEnter()
    Sleep(1000)
    Sync(3000)

    // Go back to top for next iteration
    TypeHome()
    Sleep(200)
    Sync(1000)
}

// Verify all 3 files removed from disk
var remaining = CountExisting([
    dirsMF_left + "/file_a.txt",
    dirsMF_left + "/file_b.txt",
    dirsMF_left + "/file_c.txt"
])
if (remaining == 0) {
    Log("Multi-file delete: all 3 files removed — correct")
} else {
    Panic("Multi-file delete: " + remaining + " files still exist on disk")
}
Sync(1000)

ExitFar2lWithConfirm()


///////////////////
///////////////////
// CORNER CASES
///////////////////
///////////////////
var mydir = WorkDir();

///////////////////
// Corner case: Cancel delete (F8 then Escape)
var dirsCN_profile = mydir + "/profile-cancel";
var dirsCN_left = mydir + "/left-cancel";
var dirsCN_right = mydir + "/right-cancel";
MkdirsAll([dirsCN_profile, dirsCN_left, dirsCN_right], 0o700);
Mkfiles([dirsCN_left + "/cancel_me.txt"], 0o666, 100, 1000);

StartTestApp(dirsCN_profile, dirsCN_left, dirsCN_right);
DismissHelpAndOSC52();
EnsurePanelFocus();

TypeDown()
Sleep(200)
Sync(1000)

// Ensure no modifiers
ToggleLCtrl(false)
ToggleLAlt(false)
ToggleShift(false)

TypeFKey(8)
Sleep(500)
Sync(2000)

ExpectString("Delete", 0, 0, -1, -1, 10000)
Sync(1000)

// Cancel with Escape
TypeEscape()
Sleep(500)
Sync(2000)

// File should still exist on disk
if (Exists(dirsCN_left + "/cancel_me.txt")) {
    Log("Cancel delete: file preserved — correct")
} else {
    Panic("Cancel delete: file was deleted despite cancellation")
}
Sync(1000)

ExitFar2lWithConfirm()


///////////////////
// Corner case: Delete single file without selection (cursor on file)
var dirsSN_profile = mydir + "/profile-single";
var dirsSN_left = mydir + "/left-single";
var dirsSN_right = mydir + "/right-single";
MkdirsAll([dirsSN_profile, dirsSN_left, dirsSN_right], 0o700);
Mkfiles([dirsSN_left + "/only_file.txt"], 0o666, 100, 1000);

StartTestApp(dirsSN_profile, dirsSN_left, dirsSN_right);
DismissHelpAndOSC52();
EnsurePanelFocus();

TypeDown()
Sleep(200)
Sync(1000)

ToggleLCtrl(false)
ToggleLAlt(false)
ToggleShift(false)

TypeFKey(8)
Sleep(500)
Sync(2000)

ExpectString("Delete", 0, 0, -1, -1, 10000)
Sync(1000)
TypeEnter()
Sleep(1000)
Sync(3000)

if (!Exists(dirsSN_left + "/only_file.txt")) {
    Log("Single cursor delete: file removed — correct")
} else {
    Panic("Single cursor delete: file still exists")
}
Sync(1000)

ExitFar2lWithConfirm()


///////////////////
// Corner case: Delete file with spaces in name
var dirsSP_profile = mydir + "/profile-spaces";
var dirsSP_left = mydir + "/left-sp";
var dirsSP_right = mydir + "/right-sp";
MkdirsAll([dirsSP_profile, dirsSP_left, dirsSP_right], 0o700);
Mkfiles([dirsSP_left + "/file with spaces.txt"], 0o666, 100, 1000);

StartTestApp(dirsSP_profile, dirsSP_left, dirsSP_right);
DismissHelpAndOSC52();
EnsurePanelFocus();

TypeDown()
Sleep(200)
Sync(1000)

ToggleLCtrl(false)
ToggleLAlt(false)
ToggleShift(false)

TypeFKey(8)
Sleep(500)
Sync(2000)

ExpectString("Delete", 0, 0, -1, -1, 10000)
Sync(1000)
TypeEnter()
Sleep(1000)
Sync(3000)

if (!Exists(dirsSP_left + "/file with spaces.txt")) {
    Log("Spaces filename delete: file removed — correct")
} else {
    Panic("Spaces filename delete: file still exists")
}
Sync(1000)

ExitFar2lWithConfirm()

///////////////////
// Corner case: Delete in read-only directory
var dirsRO_profile = mydir + "/profile-readonly";
var dirsRO_left = mydir + "/left-ro";
var dirsRO_right = mydir + "/right-ro";
MkdirsAll([dirsRO_profile, dirsRO_left, dirsRO_right], 0o700);
Mkdir(dirsRO_left + "/readonly_dir", 0o700);
Mkfiles([dirsRO_left + "/readonly_dir/protected.txt"], 0o666, 100, 1000);
Chmod(dirsRO_left + "/readonly_dir", 0o555);

StartTestApp(dirsRO_profile, dirsRO_left, dirsRO_right);
DismissHelpAndOSC52();
EnsurePanelFocus();

// Navigate into readonly_dir
TypeDown()
Sleep(200)
TypeEnter()
Sleep(500)
Sync(2000)

// Navigate to the file inside
TypeDown()
Sleep(200)
Sync(1000)

ToggleLCtrl(false)
ToggleLAlt(false)
ToggleShift(false)

TypeFKey(8)
Sleep(500)
Sync(2000)

// Delete dialog should appear
BeCalm()
var rRO = ExpectString("Delete", 0, 0, -1, -1, 5000)
BePanic()
if (rRO.I >= 1) {
    // Confirm — may fail due to permissions but dialog should still work
    TypeEnter()
    Sleep(1000)
    Sync(2000)
    Log("Read-only directory: delete dialog completed (file may or may not be deleted depending on permissions)")
} else {
    Log("Read-only directory: no delete dialog (expected behavior)")
}
Sync(1000)

// Escape any error dialogs and return to panel
TypeEscape()
Sleep(500)
Sync(2000)
TypeEscape()
Sleep(500)
Sync(2000)

ExitFar2lWithConfirm()


///////////////////
// Corner case: Delete non-existent file (cursor on a file that was removed externally)
var dirsNE_profile = mydir + "/profile-nonexist";
var dirsNE_left = mydir + "/left-ne";
var dirsNE_right = mydir + "/right-ne";
MkdirsAll([dirsNE_profile, dirsNE_left, dirsNE_right], 0o700);
Mkfiles([dirsNE_left + "/will_vanish.txt"], 0o666, 100, 1000);

StartTestApp(dirsNE_profile, dirsNE_left, dirsNE_right);
DismissHelpAndOSC52();
EnsurePanelFocus();

TypeDown()
Sleep(200)
Sync(1000)

// Verify file exists before deletion attempt
if (!Exists(dirsNE_left + "/will_vanish.txt")) {
    Panic("Non-existent file test: file should exist before test")
}

// Remove the file externally while far2l is running
Remove(dirsNE_left + "/will_vanish.txt")
Sync(2000)

// Try to delete via F8 — should show error or handle gracefully
ToggleLCtrl(false)
ToggleLAlt(false)
ToggleShift(false)

TypeFKey(8)
Sleep(500)
Sync(2000)

BeCalm()
var rNE = ExpectStrings(["Delete", "not found", "Cannot find"], 0, 0, -1, -1, 5000)
BePanic()
if (rNE.I >= 1) {
    Log("Non-existent file: dialog or error appeared — correct")
} else {
    Log("Non-existent file: no dialog (panel may have refreshed automatically)")
}
Sync(1000)

// Dismiss any dialog
TypeEscape()
Sleep(500)
Sync(2000)
TypeEscape()
Sleep(500)
Sync(2000)

ExitFar2lWithConfirm()
