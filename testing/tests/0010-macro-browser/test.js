LoadJS("../common.js");
var dirs = SetupTestDirs();

// Pre-configure macros by writing key_macros.ini into the profile's config dir
// far2l stores macros in <profile>/.config/settings/key_macros.ini
var macrosIni = dirs.profile + "/.config/settings";
MkdirsAll([macrosIni], 0o700);
SaveTextFile(macrosIni + "/key_macros.ini", [
    "[KeyMacros]",
    "MacroVersion=1",
    "",
    "[KeyMacros/Shell/F1]",
    "Sequence=CtrlO",
    "Description=Test macro for smoke test",
    "",
    "[KeyMacros/Shell/ShiftF1]",
    "Sequence=F3",
    "Description=Another test macro",
    "",
    "[KeyMacros/Common/AltX]",
    "Sequence=$Exit",
    "Description=Common area macro",
    ""
]);

// Start far2l with the pre-configured profile
StartTestApp(dirs.profile, dirs.left, dirs.right);
DismissHelpAndOSC52();
Sync(5000);

// ========================================
// Phase 1: Open Macro Browser, verify list
// ========================================
// F9 → navigate to Commands → open dropdown → select Macro Browser
TypeFKey(9);
ExpectString("Left    Files    Commands    Options    Right", 0, 0, -1, -1, 10000);
Sync(1000);

// Move to Commands (3rd menu column: Left=0, Files=1, Commands=2)
TypeRight();
Sleep(200);
TypeRight();
Sleep(200);
Sync(1000);

// Open the Commands dropdown
TypeDown();
Sleep(500);
Sync(2000);

// Navigate to Macro Browser: End goes to last item (About FAR), Up once to Macro Browser
TypeEnd();
Sleep(200);
Sync(1000);
TypeUp();
Sleep(200);
Sync(1000);

// Verify "Macro Browser" is highlighted in the dropdown, then select it
ExpectString("Macro Browser", 0, 0, -1, -1, 5000);
TypeEnter();
Sleep(500);
Sync(5000);

// Macro Browser should now be open — verify title and macro count
ExpectString("Macro Browser", 0, 0, -1, -1, 10000);
ExpectString("Total macros", 0, 0, -1, -1, 10000);
Sync(2000);

// Verify our test macros appear in the list
ExpectString("Test macro for smoke test", 0, 0, -1, -1, 10000);
ExpectString("Another test macro", 0, 0, -1, -1, 10000);
ExpectString("Common area macro", 0, 0, -1, -1, 10000);
Sync(2000);

// Verify area separators are shown
ExpectString("Shell", 0, 0, -1, -1, 10000);
ExpectString("Common", 0, 0, -1, -1, 10000);
Sync(2000);

// ========================================
// Phase 2: View macro details (F3)
// ========================================
// Navigate to first macro entry under Shell area.
// VMenu Down skips separators, so one Down from [0] "Total macros"
// lands on the first focusable macro entry (Shell# 1: F1).
Sync(2000);
TypeDown();
Sleep(200);
Sync(1000);
TypeFKey(3);
Sleep(500);
Sync(5000);

// View dialog should show macro details
ExpectString("Details of Macro", 0, 0, -1, -1, 10000);
ExpectString("Test macro for smoke test", 0, 0, -1, -1, 10000);
ExpectString("Sequence:", 0, 0, -1, -1, 10000);
Sync(2000);

// Close the view dialog (Enter = OK)
TypeEnter();
Sleep(300);
Sync(2000);

// Verify we're back in the Macro Browser
ExpectString("Macro Browser", 0, 0, -1, -1, 10000);
Sync(1000);

// ========================================
// Phase 3: Toggle macro enable/disable with '-'
// ========================================
ExpectString("Test macro for smoke test", 0, 0, -1, -1, 5000);
TypeSub();
Sleep(300);
Sync(2000);

// The list should refresh — verify we're still in Macro Browser
ExpectString("Macro Browser", 0, 0, -1, -1, 5000);
Sync(1000);

// Re-enable with '+'
TypeAdd();
Sleep(300);
Sync(2000);
ExpectString("Macro Browser", 0, 0, -1, -1, 5000);
Sync(1000);

// ========================================
// Phase 4: Corner cases
// ========================================

// Corner case 4a: Statistics section at bottom of list
// Navigate to bottom to verify statistics counters
TypeEnd();
Sleep(200);
Sync(1000);
ExpectString("Statistics", 0, 0, -1, -1, 10000);
ExpectString("enabled=", 0, 0, -1, -1, 10000);
ExpectString("disabled=0", 0, 0, -1, -1, 10000);
ExpectString("deleted=0", 0, 0, -1, -1, 10000);
Sync(1000);

// Corner case 4b: Ctrl-H hides empty areas (areas with 0 macros)
// First go back to top, then verify an empty area is visible
TypeHome();
Sleep(200);
Sync(1000);
ExpectString("Other (0 macros)", 0, 0, -1, -1, 10000);
Sync(1000);

// Press Ctrl+H to hide empty areas
ToggleLCtrl(true);
TypeVK(0x48);
ToggleLCtrl(false);
Sleep(500);
Sync(2000);

// After Ctrl-H, empty areas should be hidden
BeCalm();
var r = ExpectString("Other (0 macros)", 0, 0, -1, -1, 3000);
BePanic();
if (r.I >= 1) {
    Log("Empty areas hidden after Ctrl-H — correct");
} else {
    Panic("Empty areas still visible after Ctrl-H");
}
Sync(1000);

// Toggle back — show empty areas again
ToggleLCtrl(true);
TypeVK(0x48);
ToggleLCtrl(false);
Sleep(500);
Sync(2000);
ExpectString("Other (0 macros)", 0, 0, -1, -1, 10000);
Sync(1000);

// Corner case 4c: F3 on a separator shows generic info (View nullptr)
// Cursor is on [0] "Total macros" — a separator/header item
TypeFKey(3);
Sleep(500);
Sync(5000);
ExpectString("Details of Macro", 0, 0, -1, -1, 10000);
// Generic info should NOT show per-macro "Area:" field
BeCalm();
var r2 = ExpectString("Area:", 0, 0, -1, -1, 3000);
BePanic();
if (r2.I >= 1) {
    Log("F3 on separator shows generic info (no Area:) — correct");
} else {
    Panic("F3 on separator shows per-macro details — wrong");
}
Sync(2000);
TypeEnter();
Sleep(300);
Sync(2000);

// Corner case 4d: Space toggles disable/enable
// Navigate to first macro entry
TypeDown();
Sleep(200);
Sync(1000);
ExpectString("Test macro for smoke test", 0, 0, -1, -1, 5000);
Sync(1000);

// Toggle disable with Space
TypeVK(0x20);
Sleep(300);
Sync(2000);
ExpectString("Macro Browser", 0, 0, -1, -1, 5000);
Sync(1000);

// Verify statistics now show disabled=1
TypeEnd();
Sleep(200);
Sync(1000);
ExpectString("disabled=1", 0, 0, -1, -1, 5000);
Sync(1000);

// Toggle back to enabled with Space — navigate to first macro first
TypeHome();
Sleep(200);
Sync(1000);
TypeDown();
Sleep(200);
Sync(1000);
TypeVK(0x20);
Sleep(300);
Sync(2000);

// Verify disabled=1 is gone (re-enabled)
TypeEnd();
Sleep(200);
Sync(1000);
BeCalm();
var r3 = ExpectString("disabled=1", 0, 0, -1, -1, 3000);
BePanic();
if (r3.I >= 1) {
    Log("Space toggle re-enabled macro (disabled=1 gone) — correct");
} else {
    Panic("Macro still disabled after Space toggle");
}
Sync(1000);

// Corner case 4e: Delete a macro with Del
// Navigate to first macro
TypeHome();
Sleep(200);
Sync(1000);
TypeDown();
Sleep(200);
Sync(1000);
ExpectString("Test macro for smoke test", 0, 0, -1, -1, 5000);
Sync(1000);

TypeDel();
Sleep(500);
Sync(2000);
// Confirm dialog "Mark macro as deleted?"
ExpectString("Mark macro as deleted", 0, 0, -1, -1, 5000);
TypeEnter();
Sleep(300);
Sync(2000);

// After deletion: macro still in list but grayed with 'D' marker
// Description should be freed (empty)
BeCalm();
var r4 = ExpectString("Test macro for smoke test", 0, 0, -1, -1, 3000);
BePanic();
if (r4.I >= 1) {
    Log("Deleted macro description removed — correct");
} else {
    Panic("Deleted macro description still visible");
}
Sync(1000);

// Verify statistics show deleted=1
TypeEnd();
Sleep(200);
Sync(1000);
ExpectString("deleted=1", 0, 0, -1, -1, 5000);
Sync(1000);

// ========================================
// Phase 5: Exit Macro Browser
// ========================================
TypeEscape();
Sleep(300);
Sync(2000);

// Should be back at the file panels
ExpectString("left", 0, 0, -1, -1, 10000);
Sync(2000);

ExitFar2lWithConfirm();
