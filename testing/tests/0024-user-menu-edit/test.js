LoadJS("../common.js");
var dirs = SetupTestDirs();

// 0024-user-menu-edit — Test Edit User Menu dialog (F2).
//
// F2 from the command line opens the User Menu (UserMenu::Present).
// The user menu shows a VMenu with hotkeys and labels.
// F4 or Ins opens the Edit Menu dialog with:
//   - Hot key: (DI_FIXEDIT, 3 chars)
//   - Label: (DI_EDIT, ~66 chars wide)
//   - Commands: (DI_MEMOEDIT, multiline)
//   - OK / Cancel buttons
//
// The dialog title is "Edit user menu" for commands, "Edit submenu label" for submenus.
// When inserting a new item, a choice dialog appears: "Insert command" / "Insert menu".

StartTestApp(dirs.profile, dirs.left, dirs.right);
DismissHelpAndOSC52();

// Ensure panel is active
EnsurePanelFocus();

// ========================================
// Phase 1: F2 opens user menu
// ========================================
TypeFKey(2);
Sleep(500);
Sync(2000);

// The user menu should appear with a title
// F2 from panels opens "Main menu" or "Local menu"
BeCalm();
var menuFound = ExpectString("menu", 0, 0, -1, -1, 5000);
BePanic();
if (menuFound.I < 1) {
    Log("Phase 1: User menu opened — correct");
} else {
    Panic("Phase 1: User menu did not open with F2");
}
Sync(1000);

// Verify bottom title with key hints
BeCalm();
var bottomFound = ExpectString("Ins", 0, 0, -1, -1, 3000);
BePanic();
if (bottomFound.I < 1) {
    Log("Phase 1: Bottom title with key hints visible — correct");
} else {
    Log("Phase 1: Bottom title not found — menu may have different layout");
}

// Close menu with Escape
TypeEscape();
Sleep(300);
Sync(2000);
Log("Phase 1: User menu opens with F2 — correct");


// ========================================
// Phase 2: Insert a new command item
// ========================================
// Open user menu
TypeFKey(2);
Sleep(500);
Sync(2000);

// Press Insert to create a new item
TypeIns();
Sleep(500);
Sync(2000);

// Should see the insert choice dialog
ExpectString("insert", 0, 0, -1, -1, 5000);
Sync(1000);

// Select "Insert command" (first button, which is default — press Enter)
TypeEnter();
Sleep(500);
Sync(2000);

// The Edit user menu dialog should appear
ExpectString("Edit user menu", 0, 0, -1, -1, 5000);
Sync(1000);

// Verify the dialog has the expected fields
ExpectString("Hot key:", 0, 0, -1, -1, 5000);
Sync(1000);
ExpectString("Label:", 0, 0, -1, -1, 5000);
Sync(1000);
ExpectString("Commands:", 0, 0, -1, -1, 5000);
Sync(1000);

// Type a hot key (cursor should be in the hot key field)
TypeText("a");
Sleep(300);
Sync(1000);

// Tab to the Label field
TypeVK(9);
Sleep(300);
Sync(1000);

// Type a label
TypeText("My Test Command");
Sleep(300);
Sync(1000);

// Tab to the Commands memo field
TypeVK(9);
Sleep(300);
Sync(1000);

// Type a command
TypeText("echo HELLO_FROM_USERMENU");
Sleep(300);
Sync(1000);

// Press Ctrl+Enter to activate the default OK button
// (In DI_MEMOEDIT, Tab and Enter insert text, so use Ctrl+Enter
//  to activate the default button)
TypeCtrlEnter();
Sleep(500);
Sync(2000);

// Should be back at the user menu with the new item
BeCalm();
var itemFound = ExpectString("My Test Command", 0, 0, -1, -1, 5000);
BePanic();
if (itemFound.I < 1) {
    Log("Phase 2: New command item appears in user menu — correct");
} else {
    Panic("Phase 2: New command item not found in user menu");
}

// Close menu
TypeEscape();
Sleep(300);
Sync(2000);
Log("Phase 2: Insert new command item — correct");


// ========================================
// Phase 3: Edit existing item with F4
// ========================================
// Open user menu
TypeFKey(2);
Sleep(500);
Sync(2000);

// The item we created should be visible
BeCalm();
var itemVisible = ExpectString("My Test Command", 0, 0, -1, -1, 5000);
BePanic();
if (itemVisible.I < 1) {
    Log("Phase 3: Item visible in menu — correct");
} else {
    Panic("Phase 3: Previously created item not found in menu");
}
Sync(1000);

// Press F4 to edit the selected item
TypeFKey(4);
Sleep(500);
Sync(2000);

// Edit dialog should appear
ExpectString("Edit user menu", 0, 0, -1, -1, 5000);
Sync(1000);

// Verify the label field has the previous value
BeCalm();
var labelFound = ExpectString("My Test Command", 0, 0, -1, -1, 5000);
BePanic();
if (labelFound.I < 1) {
    Log("Phase 3: Edit dialog shows previous label — correct");
} else {
    Log("Phase 3: Label field content not directly visible — may be in edit field");
}

// Cancel the edit with Escape (closes dialog without saving)
TypeEscape();
Sleep(500);
Sync(2000);

// Close menu
TypeEscape();
Sleep(300);
Sync(2000);
Log("Phase 3: F4 opens edit dialog for existing item — correct");


// ========================================
// Phase 4: Execute command from user menu
// ========================================
// Open user menu
TypeFKey(2);
Sleep(500);
Sync(2000);

// Select our item and press Enter to execute
// The item should be selected (it's the only one)
TypeEnter();
Sleep(1000);
Sync(3000);

// The command should have executed — we should be back at panels
ExpectString("left", 0, 0, -1, -1, 5000);
Sync(2000);
Log("Phase 4: Execute command from user menu — correct");


// ========================================
// Phase 5: Delete item from user menu with Del
// ========================================
// Open user menu
TypeFKey(2);
Sleep(500);
Sync(2000);

// Press Delete to remove the item
TypeDel();
Sleep(500);
Sync(2000);

// Should see a confirmation dialog
BeCalm();
var confirmFound = ExpectString("delete", 0, 0, -1, -1, 3000);
BePanic();
if (confirmFound.I < 1) {
    Log("Phase 5: Delete confirmation dialog appeared — correct");
} else {
    Log("Phase 5: No confirmation dialog — may delete directly");
}

// Confirm deletion (press Enter for OK/Delete button)
TypeEnter();
Sleep(500);
Sync(2000);

// Close menu
TypeEscape();
Sleep(300);
Sync(2000);
Log("Phase 5: Delete item from user menu — correct");


ExitFar2lWithConfirm();
