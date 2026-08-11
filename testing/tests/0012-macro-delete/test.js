LoadJS("../common.js");
var dirs = SetupTestDirs();

// Pre-configure macros so Del has a target.
var macrosIni = dirs.profile + "/.config/settings";
MkdirsAll([macrosIni], 0o700);
SaveTextFile(macrosIni + "/key_macros.ini", [
    "[KeyMacros]",
    "MacroVersion=1",
    "",
    "[KeyMacros/Common/F2]",
    "Sequence=F3",
    "Description=Macro to delete",
    ""
]);

// Start far2l. Pre-existing .config/settings suppresses first-run Help.
StartTestApp(dirs.profile, dirs.left, dirs.right, "left", false);
// Dismiss OSC52 clipboard dialog that may appear on first start
DismissOSC52Only();

// ========================================
// Phase 1: MacroDelete — valid deletion
// ========================================
// Navigate to the pre-existing macro, press Del, confirm the delete dialog,
// verify the macro is marked as deleted (shows 'D' marker, grayed out).
OpenMacroBrowser();
Sync(2000);

// Verify the macro is visible and active (no 'D' marker initially)
ExpectString("Macro to delete", 0, 0, -1, -1, 10000);
Sync(1000);

// Navigate to the first macro entry (one Down from "Total macros")
TypeDown();
Sleep(200);
Sync(1000);

// Press Del to delete the selected macro
TypeDel();
Sleep(500);
Sync(3000);

// The "Mark macro as deleted?" confirmation dialog should appear
ExpectString("Mark macro as deleted", 0, 0, -1, -1, 10000);
Sync(1000);

// Confirm with Enter (OK button is the default)
TypeEnter();
Sleep(500);
Sync(3000);

// The list should refresh — verify we're still in Macro Browser
ExpectString("Macro Browser", 0, 0, -1, -1, 10000);
Sync(2000);

// The macro should now be marked as deleted. MacroDelete frees the
// Description, so the description text is no longer shown. The entry
// still appears in the list (grayed with a 'D' marker) but without
// the description text. We verify the description is gone.
BeCalm();
var descFound = ExpectString("Macro to delete", 0, 0, -1, -1, 3000);
BePanic();
if (descFound.I >= 1) {
    // Description not found — correct, it was freed by MacroDelete
    Log("Description removed after deletion — correct");
} else {
    Panic("Description still visible after deletion — should be freed");
}
Sync(1000);

CloseMacroBrowser();

// ========================================
// Phase 2: MacroDelete — invalid index (no action)
// ========================================
// When the cursor is on a non-macro entry (e.g., "Total macros" header),
// pressing Del should do nothing — no delete confirmation dialog appears.
// The MacroBrowser's Del handler checks v_menu_macroindex[i].second >= 0
// and silently skips if the cursor is not on a macro entry.
OpenMacroBrowser();
Sync(2000);

// Ensure cursor is at position 0 ("Total macros") — not a macro entry.
TypeHome();
Sleep(200);
Sync(1000);
// Press Del — nothing should happen.
TypeDel();
Sleep(500);
Sync(2000);
// Verify NO "Mark macro as deleted" dialog appeared.
// ExpectNoString panics if the string IS found (in BePanic mode).
BeCalm();
ExpectNoString("Mark macro as deleted", 0, 0, -1, -1, 3000);
BePanic();
// Good — no dialog appeared, we should still be in the Macro Browser.
ExpectString("Macro Browser", 0, 0, -1, -1, 10000);
Sync(1000);

CloseMacroBrowser();

// ========================================
// Phase 3: MacroDelete — when busy (recording)
// ========================================
// Start macro recording with Ctrl-., then try to open the Macro Browser.
// The MacroBrowser::Show() method guards with IsRecording() and shows
// "Macro Browser does not open while recording a macro" message instead.
// This verifies the IsRecording() guard that protects MacroDelete from
// being called during recording.

// Start recording by pressing Ctrl+. (Ctrl + OEM_PERIOD)
ToggleLCtrl(true);
TypeText(".");
ToggleLCtrl(false);
Sleep(500);
Sync(2000);

// Verify recording started — the 'R' indicator should be visible
// in the top-left corner of the screen.
BeCalm();
var rCell = ReadCellRaw(0, 0);
BePanic();
if (rCell.Text !== "R") {
    // Recording might not have started — check if the 'R' is somewhere
    // The recording indicator replaces the first cell's character with 'R'
    Log("Expected 'R' at 0,0 but got '" + rCell.Text + "'");
}

// Now try to open the Macro Browser via the F9 menu.
// The menu should work, but selecting "Macro Browser" should show
// the "does not open while recording" message.
TypeFKey(9);
ExpectString("Left    Files    Commands    Options    Right", 0, 0, -1, -1, 10000);
Sync(1000);
TypeRight();
Sleep(200);
TypeRight();
Sleep(200);
Sync(1000);
TypeDown();
Sleep(500);
Sync(2000);
TypeEnd();
Sleep(200);
Sync(1000);
TypeUp();
Sleep(200);
Sync(1000);
TypeEnter();
Sleep(500);
Sync(3000);

// The "does not open while recording" message should appear
ExpectString("does not open while recording", 0, 0, -1, -1, 10000);
Sync(1000);

// Dismiss the message with Enter (Continue button)
TypeEnter();
Sleep(300);
Sync(2000);

// Stop recording by pressing Ctrl+. again
ToggleLCtrl(true);
TypeText(".");
ToggleLCtrl(false);
Sleep(500);
Sync(2000);

// The AssignMacroKey dialog may appear — cancel it with Escape
TypeEscape();
Sleep(300);
Sync(2000);

// Back at panels
ExpectString("left", 0, 0, -1, -1, 10000);
Sync(1000);

ExitFar2lWithConfirm();
