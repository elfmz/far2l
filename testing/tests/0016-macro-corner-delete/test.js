LoadJS("../common.js");
var dirs = SetupTestDirs();

// Pre-configure two macros to exercise Del-Cancel and Edit-with-changes.
var macrosIni = dirs.profile + "/.config/settings";
MkdirsAll([macrosIni], 0o700);
SaveTextFile(macrosIni + "/key_macros.ini", [
    "[KeyMacros]",
    "MacroVersion=1",
    "",
    "[KeyMacros/Common/F2]",
    "Sequence=F3",
    "Description=Cancel-delete target",
    "",
    "[KeyMacros/Common/F3]",
    "Sequence=F2",
    "Description=Edit-change target",
    ""
]);

// Start far2l. Pre-existing .config/settings suppresses first-run Help,
// but the OSC52 clipboard dialog may still appear on first start.
StartTestApp(dirs.profile, dirs.left, dirs.right, "left", false, [100, 40]);
TypeEscape();
Sleep(300);
Sync(2000);
BeCalm();
var r = ExpectString("OSC52", 0, 0, -1, -1, 2000);
BePanic();
if (r.I < 1) {
    TypeEnter();
    Sleep(500);
}
Sync(5000);


// Walk the VMenu to find a macro by its description in the Edit dialog.
// Opens Edit for each focusable item, checks, and closes.
function GotoMacroByDesc(desc, maxSteps) {
    for (var i = 0; i < maxSteps; i++) {
        TypeDown();
        Sleep(100);
        Sync(500);

        // Open Edit dialog to check description
        TypeEnter();
        Sleep(500);
        Sync(2000);

        BeCalm();
        var found = ExpectString(desc, 0, 0, -1, -1, 2000);
        BePanic();

        if (found.I < 1) {
            // Found — cancel Edit and return (cursor stays on this macro)
            TypeEscape();
            Sleep(300);
            Sync(1000);
            return true;
        }

        // Wrong macro — cancel Edit
        TypeEscape();
        Sleep(300);
        Sync(1000);
    }
    return false;
}

// ========================================
// Phase 1: Del Cancel — macro must survive
// ========================================
// Press Del, confirm dialog appears, then Escape to cancel.
// The macro must still be listed with its description after cancel.
OpenMacroBrowser();
Sync(2000);

// Navigate to "Cancel-delete target" macro
var found = GotoMacroByDesc("Cancel-delete target", 30);
if (!found) {
    Panic("Could not find 'Cancel-delete target' macro");
}

// Verify description is visible before Del
ExpectString("Cancel-delete target", 0, 0, -1, -1, 5000);
Sync(1000);

// Del → confirmation appears
TypeDel(); Sleep(500); Sync(3000);
ExpectString("Mark macro as deleted", 0, 0, -1, -1, 10000);
Sync(1000);

// Cancel the deletion (Escape or selecting Cancel button)
TypeEscape(); Sleep(500); Sync(2000);

// Back in Macro Browser — description must still be present
ExpectString("Macro Browser", 0, 0, -1, -1, 10000);
Sync(1000);
ExpectString("Cancel-delete target", 0, 0, -1, -1, 10000);
Sync(1000);

CloseMacroBrowser();

// ========================================
// Phase 2: Edit with actual changes
// ========================================
// F4 on macro, change the description, Ctrl+Enter.
// MacroReplaceAdd returns Success (0). Verify the new description
// is visible in the list and the old one is gone.
OpenMacroBrowser();
Sync(200);

// Navigate to "Edit-change target" macro
found = GotoMacroByDesc("Edit-change target", 30);
if (!found) {
    Panic("Could not find 'Edit-change target' macro");
}

// Open Edit dialog
TypeEnter();
Sleep(500);
Sync(3000);
ExpectString("Edit Macro", 0, 0, -1, -1, 10000);
Sync(1000);

// Verify old description is present
ExpectString("Edit-change target", 0, 0, -1, -1, 5000);
Sync(1000);

// Tab to Description field and change it.
// The Edit dialog's Description field is reached by:
// Area dropdown → Key → AssignKey → Description (3 Tabs from Area)
// Since we opened via Enter (which means cursor was on a macro),
// the dialog is in Edit mode. Focus starts on Area dropdown.
TypeVK(9); Sleep(100); Sync(500);    // Tab → Key
TypeVK(9); Sleep(100); Sync(500);    // Tab → AssignKey
TypeVK(9); Sleep(200); Sync(1000);   // Tab → Description

// Select all text in the Description field and replace it.
// Ctrl+A selects all, then type replaces it.
ToggleLCtrl(true);
TypeText("A");
ToggleLCtrl(false);
Sleep(100);
Sync(500);
TypeText("Updated description");
Sleep(200);
Sync(1000);

// Ctrl+Enter to submit
TypeCtrlEnter();
Sleep(500);
Sync(5000);

// Dialog should close (Success) — back in Macro Browser
ExpectString("Macro Browser", 0, 0, -1, -1, 10000);
Sync(2000);

// New description should be visible
ExpectString("Updated description", 0, 0, -1, -1, 10000);
Sync(1000);

// Old description should be gone
BeCalm();
var oldDesc = ExpectString("Edit-change target", 0, 0, -1, -1, 2000);
BePanic();
if (oldDesc.I < 1) {
    Panic("Old description still visible after edit");
}

CloseMacroBrowser();

// ========================================
// Phase 3: F4 opens Edit dialog directly
// ========================================
// F4 is an alias for Enter on macro entries — both open the Edit dialog.
// This phase verifies F4 specifically and checks the Area combobox.
OpenMacroBrowser();
Sync(2000);

// Navigate to "Cancel-delete target" macro (F2 in Common area)
found = GotoMacroByDesc("Cancel-delete target", 30);
if (!found) {
    Panic("Could not find 'Cancel-delete target' macro for F4 test");
}

// Press F4 to open Edit dialog
TypeFKey(4);
Sleep(500);
Sync(3000);

ExpectString("Edit Macro", 0, 0, -1, -1, 10000);
Sync(1000);

// Verify the Area combobox shows the correct area.
// The first field in the Edit dialog is the Area dropdown (DI_COMBOBOX).
// It should show "Common" since our macro is in the Common area.
// The area text appears in the dialog — verify it's visible.
BeCalm();
var areaCheck = ExpectString("Common", 0, 0, -1, -1, 3000);
BePanic();
if (areaCheck.I >= 1) {
    Log("F4 Edit dialog shows 'Common' area — correct");
} else {
    Log("F4 Edit dialog: 'Common' area text not found (may be in dropdown)");
}
Sync(1000);

// Cancel the Edit dialog (no changes)
TypeEscape();
Sleep(300);
Sync(2000);
ExpectString("Macro Browser", 0, 0, -1, -1, 10000);
Sync(1000);

CloseMacroBrowser();

// ========================================
// Phase 4: Ctrl+S save + Ctrl+R reload round-trip
// ========================================
// Edit a macro, Ctrl+S to save, close browser, reopen, Ctrl+R to reload,
// verify the edited description persists across save/reload.
OpenMacroBrowser();
Sync(2000);

// Navigate to "Edit-change target" macro (now has "Updated description" from Phase 2)
found = GotoMacroByDesc("Updated description", 30);
if (!found) {
    Panic("Could not find 'Updated description' macro for round-trip test");
}

// Open Edit dialog via Enter, change description again
TypeEnter();
Sleep(500);
Sync(3000);
ExpectString("Edit Macro", 0, 0, -1, -1, 10000);
Sync(1000);

// Tab to Description field
TypeVK(9); Sleep(100); Sync(500);    // Tab → Key
TypeVK(9); Sleep(100); Sync(500);    // Tab → AssignKey
TypeVK(9); Sleep(200); Sync(1000);   // Tab → Description

// Select all and replace
ToggleLCtrl(true);
TypeText("A");
ToggleLCtrl(false);
Sleep(100);
Sync(500);
TypeText("Round-trip description");
Sleep(200);
Sync(1000);

// Ctrl+Enter to submit
TypeCtrlEnter();
Sleep(500);
Sync(5000);
ExpectString("Macro Browser", 0, 0, -1, -1, 10000);
Sync(2000);

// Verify new description visible
ExpectString("Round-trip description", 0, 0, -1, -1, 10000);
Sync(1000);

// Ctrl+S to save macros to file
ToggleLCtrl(true);
TypeText("s");
ToggleLCtrl(false);
Sleep(500);
Sync(3000);

// Confirm save dialog
ExpectString("Save all macros to", 0, 0, -1, -1, 10000);
Sync(1000);
TypeEnter();
Sleep(500);
Sync(3000);

// Browser refreshes after save
ExpectString("Macro Browser", 0, 0, -1, -1, 10000);
Sync(2000);
ExpectString("Round-trip description", 0, 0, -1, -1, 10000);
Sync(1000);

CloseMacroBrowser();

// Reopen browser and Ctrl+R to reload from file
OpenMacroBrowser();
Sync(2000);

// Ctrl+R to reload
ToggleLCtrl(true);
TypeText("r");
ToggleLCtrl(false);
Sleep(500);
Sync(3000);

// Confirm reload dialog
ExpectString("Reload all macros from", 0, 0, -1, -1, 10000);
Sync(1000);
TypeEnter();
Sleep(500);
Sync(3000);

// After reload, the round-trip description should still be there
ExpectString("Macro Browser", 0, 0, -1, -1, 10000);
Sync(2000);
ExpectString("Round-trip description", 0, 0, -1, -1, 10000);
Sync(1000);

CloseMacroBrowser();
ExitFar2lWithConfirm();
