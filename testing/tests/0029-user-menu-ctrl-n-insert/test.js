LoadJS("../common.js");
var dirs = SetupTestDirs();

// 0029-user-menu-ctrl-n-insert — Regression test for PR #21:
// "Ctrl+N for adding new items to User Menu"
//
// Bug (before PR): The User Menu (F2) only accepted Ins (and Numpad0)
// to insert a new menu item. Ctrl+N did nothing — it was not wired into
// ProcessSingleMenu's key handler. Users expected Ctrl+N as an alias
// for Ins (issue #3418, #1945).
//
// Fix (PR #21): Adds `case KEY_CTRLN:` alongside `case KEY_INS:` and
// includes it in the `bInsertNew` condition, so Ctrl+N opens the
// "Insert command / Insert menu" choice dialog just like Ins.
//
// This test verifies:
//  Phase 1: Ctrl+N from the User Menu opens the insert choice dialog
//           ("Insert command" button appears), then "Edit user menu".
//  Phase 2: Fill the new item, confirm, and verify it appears in the
//           menu and its command executes.
//  Phase 3: Ins still works (backwards compatibility — not broken).
//
// Existing test 0024-user-menu-edit covers Ins-based insertion but does
// NOT exercise Ctrl+N, so this test fills that gap.
//
// BEFORE the PR: Ctrl+N is not handled, so the menu interprets it as a
// literal 'n' hotkey lookup or does nothing — "Insert command" never
// appears and the test fails at Phase 1.

StartTestApp(dirs.profile, dirs.left, dirs.right);
DismissHelpAndOSC52();
EnsurePanelFocus();

// ========================================
// Phase 1: Ctrl+N opens the insert choice dialog
// ========================================
TypeFKey(2);
Sleep(500);
Sync(2000);

// The user menu should be open (title "Main menu" or "Local menu")
BeCalm();
var menuFound = ExpectString("menu", 0, 0, -1, -1, 5000);
BePanic();
if (menuFound.I < 1) {
	Log("Phase 1: User menu opened with F2 — correct");
} else {
	Panic("Phase 1: User menu did not open with F2");
}
Sync(1000);

// Press Ctrl+N (hold Left Ctrl, press N = virtual key 0x4E)
ToggleLCtrl(true);
TypeVK(0x4E);
ToggleLCtrl(false);
Sleep(500);
Sync(2000);

// The insert choice dialog should appear with the "Insert command"
// button. Use the specific button text — NOT just "insert" which also
// matches the menu's "Ins" bottom-hint and causes false positives.
BeCalm();
var insertFound = ExpectString("Insert command", 0, 0, -1, -1, 5000);
BePanic();
if (insertFound.I < 1) {
	Log("Phase 1: Ctrl+N opened insert choice dialog — correct");
} else {
	// Before the PR: Ctrl+N does nothing (or activates a menu item),
	// so the "Insert command" button never appears.
	Panic("Phase 1: Ctrl+N did NOT open insert dialog — PR #21 not applied");
}

// Select "Insert command" (Enter on the default button)
TypeEnter();
Sleep(500);
Sync(2000);

// The Edit user menu dialog should appear
ExpectString("Edit user menu", 0, 0, -1, -1, 5000);
Sync(1000);
ExpectString("Hot key:", 0, 0, -1, -1, 5000);
Sync(1000);
Log("Phase 1: PASSED — Ctrl+N opens insert dialog");


// ========================================
// Phase 2: Fill the new item and verify it appears
// ========================================
// Cursor is in the Hot key field
TypeText("n");
Sleep(300);
Sync(500);

// Tab to Label
TypeVK(9);
Sleep(300);
Sync(500);
TypeText("CtrlN Test Item");
Sleep(300);
Sync(500);

// Tab to Commands memo
TypeVK(9);
Sleep(300);
Sync(500);
TypeText("echo CTRL_N_WORKS");
Sleep(300);
Sync(500);

// Ctrl+Enter to confirm (Enter inserts text in DI_MEMOEDIT)
TypeCtrlEnter();
Sleep(500);
Sync(2000);

// Back in user menu — new item should be visible
BeCalm();
var itemFound = ExpectString("CtrlN Test Item", 0, 0, -1, -1, 5000);
BePanic();
if (itemFound.I < 1) {
	Log("Phase 2: New item created via Ctrl+N appears in menu — correct");
} else {
	Panic("Phase 2: New item not found in user menu after Ctrl+N insert");
}

// Execute the item — press Enter on it
TypeEnter();
Sleep(1000);
Sync(3000);

// The command should have run — look for the echo output
BeCalm();
var echoed = ExpectString("CTRL_N_WORKS", 0, 0, -1, -1, 5000);
BePanic();
if (echoed.I < 1) {
	Log("Phase 2: Command from Ctrl+N-created item executed — correct");
} else {
	Panic("Phase 2: Command did not execute from CtrlN-created item");
}
Sync(1000);
ExpectString("left", 0, 0, -1, -1, 5000);
Sync(2000);
Log("Phase 2: PASSED");


// ========================================
// Phase 3: Ins still works (backwards compatibility)
// ========================================
TypeFKey(2);
Sleep(500);
Sync(2000);

BeCalm();
var menu2 = ExpectString("menu", 0, 0, -1, -1, 5000);
BePanic();
if (menu2.I < 1) {
	Log("Phase 3: User menu reopened — correct");
} else {
	Panic("Phase 3: User menu did not reopen with F2");
}
Sync(1000);

// Press Ins — should also open insert dialog
TypeIns();
Sleep(500);
Sync(2000);

BeCalm();
var insertIns = ExpectString("Insert command", 0, 0, -1, -1, 5000);
BePanic();
if (insertIns.I < 1) {
	Log("Phase 3: Ins still opens insert dialog — backwards compatible — correct");
} else {
	Panic("Phase 3: Ins no longer opens insert dialog — regression");
}

// Cancel out of the insert choice dialog
TypeEscape();
Sleep(300);
Sync(1000);
// Close user menu
TypeEscape();
Sleep(300);
Sync(2000);
Log("Phase 3: PASSED");


ExitFar2lWithConfirm();
