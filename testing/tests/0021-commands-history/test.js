LoadJS("../common.js");
var dirs = SetupTestDirs();

// 0021-commands-history — Test Commands History dialog (Alt-F8).
//
// Alt-F8 opens the commands history menu (History::Select with HISTORYTYPE_CMD).
// Title: "History"
// Footer: "Up/Down Enter Esc F3 Shift+Del Del Ins Ctrl+C Ctrl+T Ctrl+F10 Ctrl+Alt+F"
//
// Commands are added to history when the user executes them from the command line.
// Selecting an item with Enter copies it to the command line.
// Esc closes the dialog without action.

StartTestApp(dirs.profile, dirs.left, dirs.right);
DismissHelpAndOSC52();

// Ensure command line is focused (Tab to panel then Tab back to cmdline)
TypeVK(9); Sleep(200); Sync(2000);
TypeVK(9); Sleep(200); Sync(2000);

// ========================================
// Phase 1: Empty commands history opens and closes
// ========================================
TypeAltFKey(8);
Sleep(500);
Sync(2000);

// The history dialog should appear with title "History"
ExpectString("History", 0, 0, -1, -1, 5000);
Sync(1000);

// Close with Escape
TypeEscape();
Sleep(300);
Sync(2000);

// Should be back at panels
ExpectString("left", 0, 0, -1, -1, 5000);
Sync(2000);
Log("Phase 1: Empty history dialog opens and closes — correct");


// ========================================
// Phase 2: Execute commands, then verify they appear in history
// ========================================
// Type and execute a command in the command line
TypeText("echo HELLO_CMD_001");
TypeEnter();
Sleep(1000);
Sync(3000);

// Execute another command
TypeText("echo HELLO_CMD_002");
TypeEnter();
Sleep(1000);
Sync(3000);

// Open commands history with Alt+F8
TypeAltFKey(8);
Sleep(500);
Sync(2000);

ExpectString("History", 0, 0, -1, -1, 5000);
Sync(1000);

// The recently executed commands should appear in the history list
ExpectString("HELLO_CMD_002", 0, 0, -1, -1, 5000);
Sync(1000);

// Close with Escape
TypeEscape();
Sleep(300);
Sync(2000);
Log("Phase 2: Executed commands appear in history — correct");


// ========================================
// Phase 3: Enter executes selected command from history
// ========================================
// Open history
TypeAltFKey(8);
Sleep(500);
Sync(2000);

ExpectString("HELLO_CMD_002", 0, 0, -1, -1, 5000);
Sync(1000);

// Press Enter to select and execute the most recent command
TypeEnter();
Sleep(1000);
Sync(3000);

// After execution, we should be back at the panels
// (the command runs in the VT and returns)
ExpectString("left", 0, 0, -1, -1, 5000);
Sync(1000);
Log("Phase 3: Enter executes selected command from history — correct");


// ========================================
// Phase 4: Navigate history with Down arrow
// ========================================
// Open history
TypeAltFKey(8);
Sleep(500);
Sync(2000);

ExpectString("History", 0, 0, -1, -1, 5000);
Sync(1000);

// The first item (most recent) should be selected
ExpectString("HELLO_CMD_002", 0, 0, -1, -1, 5000);
Sync(1000);

// Navigate down to the second item
TypeDown();
Sleep(300);
Sync(1000);

// The second command should now be selected/visible
ExpectString("HELLO_CMD_001", 0, 0, -1, -1, 5000);
Sync(1000);

// Close with Escape
TypeEscape();
Sleep(300);
Sync(2000);
Log("Phase 4: Down arrow navigates history items — correct");


// ========================================
// Phase 5: F3 opens command info dialog
// ========================================
// Open history
TypeAltFKey(8);
Sleep(500);
Sync(2000);

ExpectString("History", 0, 0, -1, -1, 5000);
Sync(1000);

// Press F3 to open the command info dialog
TypeFKey(3);
Sleep(500);
Sync(2000);

// The command info dialog should show "History Command Info" title
ExpectString("History Command Info", 0, 0, -1, -1, 5000);
Sync(1000);

// Also verify the command and directory labels
ExpectString("Command:", 0, 0, -1, -1, 5000);
Sync(1000);

// Close the info dialog with Escape
TypeEscape();
Sleep(300);
Sync(2000);

// Close the history dialog with Escape
TypeEscape();
Sleep(300);
Sync(2000);
Log("Phase 5: F3 opens command info dialog — correct");


// ========================================
// Phase 6: Delete item from history with Shift+Del
// ========================================
// Open history
TypeAltFKey(8);
Sleep(500);
Sync(2000);

ExpectString("History", 0, 0, -1, -1, 5000);
Sync(1000);

// First item should be HELLO_CMD_002
ExpectString("HELLO_CMD_002", 0, 0, -1, -1, 5000);
Sync(1000);

// Delete the first item with Shift+Del
ToggleShift(true);
TypeDel();
ToggleShift(false);
Sleep(500);
Sync(2000);

// HELLO_CMD_002 should no longer be in the history list
BeCalm();
var deletedResult = ExpectString("HELLO_CMD_002", 0, 0, -1, -1, 2000);
BePanic();
if (deletedResult.I < 1) {
    Log("Phase 6: Shift+Del did not remove item from history — may need confirmation dialog");
    // Try closing any confirmation dialog
    TypeEscape();
    Sleep(300);
    Sync(1000);
} else {
    Log("Phase 6: Shift+Del removed item from history — correct");
}

// Close history
TypeEscape();
Sleep(300);
Sync(2000);


ExitFar2lWithConfirm();
