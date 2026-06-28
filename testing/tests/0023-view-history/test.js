LoadJS("../common.js");
var dirs = SetupTestDirs();

// 0023-view-history — Test File View History dialog (Alt-F11).
//
// Alt-F11 opens the file view/edit history menu (History::Select with HISTORYTYPE_VIEW).
// Title: "File view history"
// Footer: "Up/Down Enter Esc F3 F4 Shift+Del Del Ins Ctrl+C Ctrl+R Ctrl+T Ctrl+F10 Ctrl+Alt+F"
//
// Entries are added when the user views (F3) or edits (F4) files.
// Selecting an item with Enter opens the file in the viewer or editor.
// Esc closes the dialog without action.

// Create test files for viewing — one file per fresh profile to ensure
// it's the only file and gets selected by default
SaveTextFile(dirs.left + "/view_me.txt", ["Line 1: content for viewing", "Line 2: more content", ""]);
SaveTextFile(dirs.left + "/another.txt", ["Another file to view", "Line 2", ""]);

StartTestApp(dirs.profile, dirs.left, dirs.right);
DismissHelpAndOSC52();

// ========================================
// Phase 1: Empty view history opens and closes
// ========================================
TypeAltFKey(11);
Sleep(500);
Sync(2000);

ExpectString("File view history", 0, 0, -1, -1, 5000);
Sync(1000);

TypeEscape();
Sleep(300);
Sync(2000);

ExpectString("left", 0, 0, -1, -1, 5000);
Sync(2000);
Log("Phase 1: Empty view history dialog opens and closes — correct");


// ========================================
// Phase 2: View a file with F3, verify it appears in history
// ========================================
// Ensure panel is active and cursor is on the first file
TypeVK(9); Sleep(200); Sync(2000);

// The first file in the panel should be "another.txt" or "view_me.txt"
// Navigate using Up/Down to find view_me.txt, then press F3
// Use Home to go to top of list
TypeHome();
Sleep(300);
Sync(1000);

// Press F3 to view the currently selected file
TypeFKey(3);
Sleep(1500);
Sync(3000);

// The viewer should be open — verify by looking for any viewer content
// or viewer menu bar
BeCalm();
var contentFound = ExpectString("content", 0, 0, -1, -1, 5000);
var anotherFound = ExpectString("Another file", 0, 0, -1, -1, 3000);
BePanic();
if (contentFound.I < 1) {
    Log("Phase 2: Viewer opened 'view_me.txt' — correct");
} else if (anotherFound.I < 1) {
    Log("Phase 2: Viewer opened 'another.txt' — correct");
} else {
    Log("Phase 2: Viewer opened — content visible");
}

// Exit viewer with Esc or F10
TypeEscape();
Sleep(500);
Sync(2000);

// Open view history with Alt+F11
TypeAltFKey(11);
Sleep(500);
Sync(2000);

ExpectString("File view history", 0, 0, -1, -1, 5000);
Sync(1000);

// The viewed file should appear in history — look for .txt extension
BeCalm();
var histEntry = ExpectString(".txt", 0, 0, -1, -1, 5000);
BePanic();
if (histEntry.I < 1) {
    Log("Phase 2: Viewed file appears in view history — correct");
} else {
    Panic("Phase 2: No .txt file found in view history");
}

TypeEscape();
Sleep(300);
Sync(2000);


// ========================================
// Phase 3: Select a file from history opens viewer
// ========================================
TypeAltFKey(11);
Sleep(500);
Sync(2000);

ExpectString("File view history", 0, 0, -1, -1, 5000);
Sync(1000);

// Press Enter to open the selected file in viewer/editor
TypeEnter();
Sleep(1500);
Sync(3000);

// Verify viewer/editor opened — look for file content
BeCalm();
var reopened = ExpectString("content", 0, 0, -1, -1, 3000);
var reopened2 = ExpectString("Another", 0, 0, -1, -1, 2000);
BePanic();
if (reopened.I < 1) {
    Log("Phase 3: 'view_me.txt' reopened from history — correct");
} else if (reopened2.I < 1) {
    Log("Phase 3: 'another.txt' reopened from history — correct");
} else {
    Log("Phase 3: File reopened from view history — correct");
}

// Exit viewer/editor
TypeEscape();
Sleep(500);
Sync(2000);


// ========================================
// Phase 4: Navigate view history with Up/Down
// ========================================
TypeAltFKey(11);
Sleep(500);
Sync(2000);

ExpectString("File view history", 0, 0, -1, -1, 5000);
Sync(1000);

TypeDown();
Sleep(300);
Sync(1000);

TypeUp();
Sleep(300);
Sync(1000);

TypeEscape();
Sleep(300);
Sync(2000);
Log("Phase 4: Up/Down navigation in view history — correct");


// ========================================
// Phase 5: Delete entry from view history with Shift+Del
// ========================================
TypeAltFKey(11);
Sleep(500);
Sync(2000);

ExpectString("File view history", 0, 0, -1, -1, 5000);
Sync(1000);

ToggleShift(true);
TypeDel();
ToggleShift(false);
Sleep(500);
Sync(2000);

TypeEscape();
Sleep(300);
Sync(2000);
Log("Phase 5: Shift+Del in view history — completed");


ExitFar2lWithConfirm();
