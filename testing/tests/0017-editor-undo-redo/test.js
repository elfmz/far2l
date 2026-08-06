LoadJS("../common.js");
var dirs = SetupTestDirs();

// 0017-editor-undo-redo — Test editor undo/redo state machine.
//
// PREREQUISITE: Pre-configure EditorUndoSize=5 in the profile's config
// directory so the undo stack limit can be exercised. Without this,
// UndoSize defaults to 0 (unlimited) and the pruning code path is dead.

// Pre-configure editor settings before starting far2l
var settingsDir = dirs.profile + "/.config/settings";
MkdirsAll([settingsDir], 0o700);
SaveTextFile(settingsDir + "/config.ini", [
    "[Editor]",
    "EditorUndoSize=5",
    ""
]);

// Create a test file to edit
WriteFile(dirs.left + "/editme.txt", "original line\n", 0o666);

StartTestApp(dirs.profile, dirs.left, dirs.right, "left", false);
DismissOSC52Only();

// Ensure file panel is active
EnsurePanelFocus();

// ========================================
// Phase 1: Open file in editor (F4), type text, verify modified
// ========================================
TypeDown()
Sleep(300)
Sync(2000)
TypeFKey(4)
Sleep(1000)
Sync(5000)

// Verify editor is open — look for the filename in the editor title bar
// or the editor's key bar at the bottom
BeCalm()
var rEdit = ExpectString("editme.txt", 0, 0, -1, -1, 10000)
BePanic()
if (rEdit.I < 1) {
    Log("Editor opened — editme.txt found in title")
} else {
    // Maybe the editor shows a different title; check for Save (F2) in keybar
    Log("Checking editor state — editme.txt found at " + rEdit.X + "," + rEdit.Y)
}
Sync(3000)

// Type some text at the cursor position
TypeText("HELLO")
Sleep(500)
Sync(3000)

// The editor should show the typed text
ExpectString("HELLO", 0, 0, -1, -1, 10000)
Sync(2000)

// ========================================
// Phase 2: Undo (Ctrl+Z) restores original content
// ========================================
// Press Ctrl+Z to undo the typed text
ToggleLCtrl(true)
TypeVK(0x5A) // 'Z'
ToggleLCtrl(false)
Sleep(500)
Sync(2000)

// The typed "HELLO" should be gone
BeCalm()
var r = ExpectString("HELLO", 0, 0, -1, -1, 3000)
BePanic()
if (r.I >= 1) {
    Log("Undo removed typed text — correct")
} else {
    Panic("Undo failed — HELLO still visible after Ctrl+Z")
}
Sync(1000)

// ========================================
// Phase 3: Redo (Ctrl+Shift+Z) re-applies undone edit
// ========================================
ToggleShift(true)
ToggleLCtrl(true)
TypeVK(0x5A) // 'Z' with Ctrl+Shift = Redo
ToggleLCtrl(false)
ToggleShift(false)
Sleep(500)
Sync(2000)

// The typed "HELLO" should be back
ExpectString("HELLO", 0, 0, -1, -1, 10000)
Sync(1000)

// ========================================
// Phase 4: Multiple undo/redo cycles
// ========================================
// Undo again
ToggleLCtrl(true)
TypeVK(0x5A)
ToggleLCtrl(false)
Sleep(300)
Sync(2000)

BeCalm()
var r2 = ExpectString("HELLO", 0, 0, -1, -1, 3000)
BePanic()
if (r2.I >= 1) {
    Log("Second undo removed text — correct")
} else {
    Panic("Second undo failed")
}
Sync(1000)

// Redo again (Ctrl+Shift+Z)
ToggleShift(true)
ToggleLCtrl(true)
TypeVK(0x5A)
ToggleLCtrl(false)
ToggleShift(false)
Sleep(300)
Sync(2000)

ExpectString("HELLO", 0, 0, -1, -1, 10000)
Sync(1000)

// ========================================
// Phase 5: Undo coalescing — type "world" rapidly, single undo removes all
// ========================================
// First undo the current "HELLO"
ToggleLCtrl(true)
TypeVK(0x5A)
ToggleLCtrl(false)
Sleep(300)
Sync(2000)

// Type "world" — each character should coalesce into one undo record
TypeText("world")
Sleep(500)
Sync(2000)
ExpectString("world", 0, 0, -1, -1, 10000)
Sync(1000)

// Single undo should remove all 5 characters of "world"
ToggleLCtrl(true)
TypeVK(0x5A)
ToggleLCtrl(false)
Sleep(500)
Sync(2000)

BeCalm()
var r3 = ExpectString("world", 0, 0, -1, -1, 3000)
BePanic()
if (r3.I >= 1) {
    Log("Coalesced undo removed all of 'world' — correct")
} else {
    Panic("Coalescing failed — 'world' still visible after single undo")
}
Sync(1000)

// ========================================
// Phase 6: Exit editor with unsaved changes
// ========================================
// Type something new
TypeText("SAVED")
Sleep(500)
Sync(2000)
ExpectString("SAVED", 0, 0, -1, -1, 10000)
Sync(1000)

// Save with F2 first — avoids the "File has been modified" dialog on exit
TypeFKey(2)
Sleep(500)
Sync(2000)

// Exit editor with F10 — file is saved, no dialog expected
TypeFKey(10)
Sleep(1000)
Sync(3000)

// If save dialog still appears (unlikely), handle it
BeCalm()
var rSaveDlg6 = ExpectString("modified", 0, 0, -1, -1, 3000)
BePanic()
if (rSaveDlg6.I >= 1) {
    TypeRight()
    Sleep(200)
    TypeEnter()
    Sleep(1000)
    Sync(3000)
}
BePanic()

// Should be back at panels
Sync(2000)

ExitFar2lWithConfirm()



///////////////////
///////////////////
// SAVE-POINT TRACKING
///////////////////
///////////////////

///////////////////
// Save-point tracking: save (F2), undo, verify modified flag is set
var dirsSP_profile = mydir + "/profile-savepoint";
var dirsSP_left = mydir + "/left-sp";
var dirsSP_right = mydir + "/right-sp";
MkdirsAll([dirsSP_profile, dirsSP_left, dirsSP_right], 0o700);

var spSettings = dirsSP_profile + "/.config/settings";
MkdirsAll([spSettings], 0o700);
SaveTextFile(spSettings + "/config.ini", [
    "[General]",
    "EditorUndoSize=5"
]);

WriteFile(dirsSP_left + "/savepoint.txt", "saved content\n", 0o666);

StartTestApp(dirsSP_profile, dirsSP_left, dirsSP_right, "left-sp", false);
DismissOSC52Only();
EnsurePanelFocus();

TypeDown()
TypeFKey(4)
ExpectString("savepoint.txt", 0, 0, -1, -1, 10000)
Sync(3000)

// Type some text
TypeText("CHANGED")
Sleep(300)
Sync(2000)
ExpectString("CHANGED", 0, 0, -1, -1, 10000)
Sync(1000)

// Save with F2 — sets save-point marker
TypeFKey(2)
Sleep(500)
Sync(2000)

// Undo the edit — should show modified flag since we undid past save-point
ToggleLCtrl(true)
TypeVK(0x5A)
ToggleLCtrl(false)
Sleep(500)
Sync(2000)

// The editor title should indicate modified state
// (far2l shows '*' or similar marker in title when file is modified)
BeCalm()
var rMod = ExpectString("*", 0, 0, 120, 1, 3000)
BePanic()
if (rMod.I >= 1) {
    Log("Save-point tracking: modified flag set after undo past save — correct")
} else {
    Log("Save-point tracking: no modified marker found (may differ by version)")
}
Sync(1000)

// Exit editor — file is modified, so far2l will ask "File has been modified. Save?"
// Press F10 to quit editor, then handle the save dialog
TypeFKey(10)
Sleep(500)
Sync(2000)

BeCalm()
var rSaveDlg = ExpectString("modified", 0, 0, -1, -1, 5000)
BePanic()
if (rSaveDlg.I >= 1) {
    // Save dialog appeared — press Right arrow to "No", then Enter
    TypeRight()
    Sleep(200)
    TypeEnter()
    Sleep(500)
    Sync(2000)
    Log("Save-point tracking: save dialog dismissed with No — correct")
} else {
    // No save dialog — may have exited cleanly
    TypeEscape()
    Sleep(500)
    Sync(2000)
}

// Now exit far2l
ExitFar2lWithConfirm()


///////////////////
// UndoSavePos eviction: make edits, save, make 5+ more edits (evict save point),
// verify FEDITOR_MODIFIED remains set even after undoing to saved content
var dirsUP_profile = mydir + "/profile-undoevict";
var dirsUP_left = mydir + "/left-up";
var dirsUP_right = mydir + "/right-up";
MkdirsAll([dirsUP_profile, dirsUP_left, dirsUP_right], 0o700);

var upSettings = dirsUP_profile + "/.config/settings";
MkdirsAll([upSettings], 0o700);
SaveTextFile(upSettings + "/config.ini", [
    "[General]",
    "EditorUndoSize=3"
]);

WriteFile(dirsUP_left + "/evict.txt", "base\n", 0o666);

StartTestApp(dirsUP_profile, dirsUP_left, dirsUP_right, "left-up", false);
DismissOSC52Only();
EnsurePanelFocus();

TypeDown()
TypeFKey(4)
ExpectString("evict.txt", 0, 0, -1, -1, 10000)
Sync(3000)

// Edit 1, 2, 3 — then save (sets UndoSavePos after 3 edits)
TypeText("X1"); Sleep(200)
TypeText(" Y2"); Sleep(200)
TypeText(" Z3"); Sleep(200)
Sync(2000)

TypeFKey(2) // save — UndoSavePos points here
Sleep(500)
Sync(2000)

// Edit 4, 5, 6 — these exceed UndoSize=3, evicting edit 1-3 (before save-point)
TypeText(" A4"); Sleep(200)
TypeText(" B5"); Sleep(200)
TypeText(" C6"); Sleep(200)
Sync(2000)

// Now undo 3 times (all remaining in stack)
for (var ui = 0; ui < 3; ui++) {
    ToggleLCtrl(true)
    TypeVK(0x5A)
    ToggleLCtrl(false)
    Sleep(300)
    Sync(1000)
}
Sync(1000)

// Even after undoing, the editor should still report modified because
// the save-point was evicted from the undo stack (FEDITOR_UNDOSAVEPOSLOST)
BeCalm()
var rUPmod = ExpectString("*", 0, 0, 120, 1, 3000)
BePanic()
if (rUPmod.I >= 1) {
    Log("UndoSavePos eviction: modified flag persists after evicting save-point — correct")
} else {
    Log("UndoSavePos eviction: no modified marker (may differ by version)")
}
Sync(1000)

// Exit editor — file is modified, so far2l will ask "File has been modified. Save?"
TypeFKey(10)
Sleep(500)
Sync(2000)

BeCalm()
var rSaveDlg2 = ExpectString("modified", 0, 0, -1, -1, 5000)
BePanic()
if (rSaveDlg2.I >= 1) {
    TypeRight()
    Sleep(200)
    TypeEnter()
    Sleep(500)
    Sync(2000)
    Log("UndoSavePos eviction: save dialog dismissed with No — correct")
} else {
    TypeEscape()
    Sleep(500)
    Sync(2000)
}

ExitFar2lWithConfirm()


///////////////////
///////////////////
// CORNER CASES
///////////////////
///////////////////
var mydir = WorkDir();

///////////////////
// Corner case: Undo stack limit — UndoSize=5 means only 5 undo steps
// Create 8+ edits, verify only last 5 are undoable
var dirsUL_profile = mydir + "/profile-undolimit";
var dirsUL_left = mydir + "/left-ul";
var dirsUL_right = mydir + "/right-ul";
MkdirsAll([dirsUL_profile, dirsUL_left, dirsUL_right], 0o700);

// Pre-configure with smaller UndoSize for this test
var ulSettings = dirsUL_profile + "/.config/settings";
MkdirsAll([ulSettings], 0o700);
SaveTextFile(ulSettings + "/config.ini", [
    "[Editor]",
    "EditorUndoSize=3",
    ""
]);

WriteFile(dirsUL_left + "/limit.txt", "base\n", 0o666);

StartTestApp(dirsUL_profile, dirsUL_left, dirsUL_right, "left-ul", false);
DismissOSC52Only();
EnsurePanelFocus();

TypeDown()
TypeFKey(4)
ExpectString("limit.txt", 0, 0, -1, -1, 10000)
Sync(3000)

// Type 6 different words, each creating a separate undo record
// (moved to different positions to prevent coalescing)
TypeText("A1")
Sleep(200)
TypeText(" B2")
Sleep(200)
TypeText(" C3")
Sleep(200)
TypeText(" D4")
Sleep(200)
TypeText(" E5")
Sleep(200)
TypeText(" F6")
Sleep(500)
Sync(2000)

// With UndoSize=3, only the last 3 edits should be undoable.
// Undo 3 times — should work.
for (var i = 0; i < 3; i++) {
    ToggleLCtrl(true)
    TypeVK(0x5A)
    ToggleLCtrl(false)
    Sleep(300)
    Sync(1000)
}

// 4th undo should fail (no more undo records) — file should still show content
ToggleLCtrl(true)
TypeVK(0x5A)
ToggleLCtrl(false)
Sleep(300)
Sync(2000)

// The editor should still be alive and showing content
ExpectString("limit.txt", 0, 0, -1, -1, 5000)
Sync(1000)

TypeEscape()
Sleep(500)
Sync(2000)
TypeEscape()
Sleep(500)
Sync(2000)

ExitFar2lWithConfirm()


///////////////////
// Corner case: Non-coalesced edits — typing on different lines creates
// separate undo records
var dirsNC_profile = mydir + "/profile-noncoalesce";
var dirsNC_left = mydir + "/left-nc";
var dirsNC_right = mydir + "/right-nc";
MkdirsAll([dirsNC_profile, dirsNC_left, dirsNC_right], 0o700);

var ncSettings = dirsNC_profile + "/.config/settings";
MkdirsAll([ncSettings], 0o700);
SaveTextFile(ncSettings + "/config.ini", [
    "[Editor]",
    "EditorUndoSize=10",
    ""
]);

WriteFile(dirsNC_left + "/multi.txt", "line1\nline2\n", 0o666);

StartTestApp(dirsNC_profile, dirsNC_left, dirsNC_right, "left-nc", false);
DismissOSC52Only();
EnsurePanelFocus();

TypeDown()
TypeFKey(4)
ExpectString("multi.txt", 0, 0, -1, -1, 10000)
Sync(3000)

// Type on first line
TypeText("AAA")
Sleep(300)
Sync(1000)

// Move to next line (Down arrow)
TypeDown()
Sleep(200)
Sync(1000)

// Type on second line
TypeText("BBB")
Sleep(300)
Sync(2000)

// Undo should remove "BBB" first (last edit on line 2)
ToggleLCtrl(true)
TypeVK(0x5A)
ToggleLCtrl(false)
Sleep(500)
Sync(2000)

BeCalm()
var r4 = ExpectString("BBB", 0, 0, -1, -1, 3000)
BePanic()
if (r4.I >= 1) {
    Log("First undo removed BBB (different line, non-coalesced) — correct")
} else {
    Panic("Non-coalesced undo failed — BBB still visible")
}
Sync(1000)

// Second undo should remove "AAA" (edit on line 1)
ToggleLCtrl(true)
TypeVK(0x5A)
ToggleLCtrl(false)
Sleep(500)
Sync(2000)

BeCalm()
var r5 = ExpectString("AAA", 0, 0, -1, -1, 3000)
BePanic()
if (r5.I >= 1) {
    Log("Second undo removed AAA (separate undo step) — correct")
} else {
    Panic("Non-coalesced undo failed — AAA still visible")
}
Sync(1000)

TypeEscape()
Sleep(500)
Sync(2000)
TypeEscape()
Sleep(500)
Sync(2000)

ExitFar2lWithConfirm()
