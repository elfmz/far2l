LoadJS("../common.js");
var dirs = SetupTestDirs();

// Pre-configure one macro so Edit-noop and Duplicate-rejection have a target.
// far2l stores macros in <profile>/.config/settings/key_macros.ini
var macrosIni = dirs.profile + "/.config/settings";
MkdirsAll([macrosIni], 0o700);
SaveTextFile(macrosIni + "/key_macros.ini", [
    "[KeyMacros]",
    "MacroVersion=1",
    "",
    "[KeyMacros/Common/F2]",
    "Sequence=F3",
    "Description=Pre-existing macro",
    ""
]);

// Start far2l. Pre-existing .config/settings suppresses first-run Help,
// but the OSC52 clipboard dialog may still appear on first start.
StartTestApp(dirs.profile, dirs.left, dirs.right, "left", false);
DismissOSC52Only();




// ========================================
// Phase 1: MacroReplaceAdd — Add new macro
// ========================================
// Open Macro Browser, press Ins to add, fill dialog, OK, verify macro appears.
OpenMacroBrowser();
Sync(2000);

ExpectString("Pre-existing macro", 0, 0, -1, -1, 10000);
Sync(1000);

// Press Ins to open the "New Macro" edit dialog and fill all fields.
FillNewMacroDialog("F5", "Added by smoke test", "CtrlO");

// Press Ctrl+Enter to activate the OK button (DIF_DEFAULT).
// Plain Enter on a DI_MEMOEDIT inserts a newline instead of activating OK.
TypeCtrlEnter();
Sleep(500);
Sync(5000);

// Dialog should close and return to Macro Browser list.
ExpectString("Macro Browser", 0, 0, -1, -1, 10000);
Sync(2000);
ExpectString("Added by smoke test", 0, 0, -1, -1, 10000);
Sync(1000);

CloseMacroBrowser();

// ========================================
// Phase 2: MacroReplaceAdd — Edit with NoChanges (noop)
// ========================================
// Open browser, navigate to the pre-existing macro, F4 to edit,
// OK without changing anything. MacroReplaceAdd returns NoChanges (8)
// which the dialog treats as success (return TRUE), so the dialog closes
// and the list remains unchanged.
OpenMacroBrowser();
Sync(2000);

// Navigate to the first macro entry (the pre-existing F2 macro)
GotoFirstMacro();

// Press F4 to edit the selected macro
TypeFKey(4);
Sleep(500);
Sync(3000);

ExpectString("Edit Macro", 0, 0, -1, -1, 10000);
Sync(1000);
ExpectString("Pre-existing macro", 0, 0, -1, -1, 10000);
Sync(1000);

// Press Ctrl+Enter to OK without changing anything — NoChanges path.
// Ctrl+Enter activates the DIF_DEFAULT button regardless of focus.
TypeCtrlEnter();
Sleep(500);
Sync(3000);

// Dialog should close (NoChanges returns TRUE => dialog exits with OK)
ExpectString("Macro Browser", 0, 0, -1, -1, 10000);
Sync(2000);
ExpectString("Pre-existing macro", 0, 0, -1, -1, 10000);
Sync(1000);

CloseMacroBrowser();

// ========================================
// Phase 3: MacroReplaceAdd — Duplicate key rejection
// ========================================
// Open browser, Ins to add a new macro with the same key (F2) in the same
// area (Common) as the pre-existing macro. MacroReplaceAdd returns
// DuplicateAreaKey (9) and the dialog shows an error message.
OpenMacroBrowser();
Sync(2000);

// Press Ins to open the "New Macro" edit dialog and fill all fields.
FillNewMacroDialog("F2", "Duplicate macro", "F4");
// Press Ctrl+Enter to OK — should trigger DuplicateAreaKey error
TypeCtrlEnter();
Sleep(500);
Sync(3000);

// The error message should appear
ExpectString("already another macro", 0, 0, -1, -1, 10000);
Sync(1000);

// Dismiss the error dialog with Enter (Ok)
TypeEnter();
Sleep(300);
Sync(2000);

// We should still be in the Edit dialog (the error does not close it)
ExpectString("New Macro", 0, 0, -1, -1, 10000);
Sync(1000);

// Cancel the edit dialog
TypeEscape();
Sleep(300);
Sync(2000);

// Back in Macro Browser
ExpectString("Macro Browser", 0, 0, -1, -1, 10000);
Sync(1000);

CloseMacroBrowser();

ExitFar2lWithConfirm();
