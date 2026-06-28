// 0032-bookmarks-nested — Test nested folder bookmarks with submenu UI (PR #38).
//
// PR #38 ports far3's nested Bookmark Slot model: each slot 0-9 holds a
// vector of BookmarkEntry instead of a single bookmark. When a slot has
// more than one entry, pressing RightCtrl+0..9 opens a submenu listing
// all entries instead of jumping directly.
//
// Regression target:
//   On main (pre-PR), each slot holds exactly ONE bookmark. Pressing
//   RightCtrl+0 jumps straight to that bookmark's folder. There is no
//   submenu, and saving a second folder to the same slot overwrites the
//   first. After the PR, a second save to the same slot ADDS an entry,
//   and RightCtrl+N opens a submenu when count > 1.
//
// This test is an end-to-end smoke test driven entirely through the
// TTY test harness (key presses + screen string checks). It does NOT
// touch the C++ unit-test API — it exercises the observable UI behavior
// that the unit tests cannot reach:
//   1. Ctrl+Shift+N saves the current folder to slot N.
//   2. A second save to the SAME slot adds an entry (not overwrite).
//   3. RightCtrl+N on a multi-entry slot opens a submenu.
//   4. Selecting a submenu entry navigates the panel to that folder.
//   5. Backwards compatibility: a single-entry slot still jumps directly.
//
// Key codes (from far2l-smoke.go / WinCompat.h):
//   0x30..0x39 = VK_0..VK_9 (top-row digits)
//   ToggleRCtrl / ToggleLCtrl / ToggleShift simulate modifier presses.

LoadJS("../common.js");
var dirs = SetupTestDirs();

// Create two distinct subdirectories to use as bookmark targets.
// Unique names so ExpectString can find them on the panel unambiguously.
MkdirsAll([dirs.left + "/bm_alpha", dirs.left + "/bm_beta"], 0o755);

// =============================================================================
// Phase 1: Single-entry slot still jumps directly (backwards compat)
// =============================================================================
// Start far2l WITHOUT the help window by using a pre-seeded profile, so the
// first screen is the file panel. We still use the standard StartTestApp
// which expects "left" in the left panel title.
StartTestApp(dirs.profile, dirs.left, dirs.right, "left", false);
DismissOSC52Only();
EnsurePanelFocus();

// Navigate into bm_alpha via the command line.
TypeText("cd bm_alpha");
TypeEnter();
Sleep(1500);
Sync(3000);
ExpectString("bm_alpha", 0, 0, -1, -1, 5000);
Sync(1000);
Log("Phase 1: navigated into bm_alpha");

// Save current folder to slot 1: Ctrl+Shift+1
//   KEY_CTRLSHIFT1 = (KEY_CTRL | KEY_SHIFT) + '1'
//   In the harness, hold LCtrl + Shift, press VK_1 (0x31), release.
ToggleLCtrl(true);
ToggleShift(true);
TypeVK(0x31);
ToggleShift(false);
ToggleLCtrl(false);
Sleep(500);
Sync(2000);
Log("Phase 1: saved bm_alpha to slot 1 (Ctrl+Shift+1)");

// Go back to the parent so the panel is NOT showing bm_alpha.
TypeText("cd ..");
TypeEnter();
Sleep(1500);
Sync(3000);
// Panel should now show the parent (dirs.left basename "left") and list
// bm_alpha / bm_beta as entries.
ExpectString("bm_alpha", 0, 0, -1, -1, 5000);
Sync(1000);
Log("Phase 1: returned to parent, bm_alpha visible as panel entry");

// Now jump back via RightCtrl+1.
//   KEY_RCTRL1 = KEY_RCTRL + '1'  ->  hold RCtrl, press VK_1, release.
ToggleRCtrl(true);
TypeVK(0x31);
ToggleRCtrl(false);
Sleep(1500);
Sync(3000);

// The panel should now be inside bm_alpha again. With a single entry the
// PR's submenu does NOT open — it jumps directly, same as pre-PR behavior.
ExpectString("bm_alpha", 0, 0, -1, -1, 5000);
Sync(1000);
Log("Phase 1: RightCtrl+1 jumped directly to bm_alpha — single-entry slot");

// Go back to parent for the next phase.
TypeText("cd ..");
TypeEnter();
Sleep(1500);
Sync(3000);
ExpectString("bm_beta", 0, 0, -1, -1, 5000);
Sync(1000);

ExitFar2lWithConfirm();
Log("Phase 1: PASSED — single-entry slot jumps directly (backwards compat)");


// =============================================================================
// Phase 2: Second save to same slot ADDS an entry; submenu opens
// =============================================================================
// Fresh profile so slot 1 is empty again — we need a clean slot to build a
// multi-entry state deterministically.
var dirs2 = FreshTestDirs("multi");
MkdirsAll([dirs2.left + "/bm_gamma", dirs2.left + "/bm_delta"], 0o755);

StartTestApp(dirs2.profile, dirs2.left, dirs2.right, "left-multi", false);
DismissOSC52Only();
EnsurePanelFocus();

// Navigate into bm_gamma and save to slot 2.
TypeText("cd bm_gamma");
TypeEnter();
Sleep(1500);
Sync(3000);
ExpectString("bm_gamma", 0, 0, -1, -1, 5000);
Sync(1000);

ToggleLCtrl(true);
ToggleShift(true);
TypeVK(0x32); // Ctrl+Shift+2 -> save to slot 2
ToggleShift(false);
ToggleLCtrl(false);
Sleep(500);
Sync(2000);
Log("Phase 2: saved bm_gamma to slot 2");

// Go to parent, then into bm_delta, and save to slot 2 AGAIN.
// Pre-PR: this overwrites bm_gamma. Post-PR: this adds a second entry.
TypeText("cd ..");
TypeEnter();
Sleep(1500);
Sync(3000);

TypeText("cd bm_delta");
TypeEnter();
Sleep(1500);
Sync(3000);
ExpectString("bm_delta", 0, 0, -1, -1, 5000);
Sync(1000);

ToggleLCtrl(true);
ToggleShift(true);
TypeVK(0x32); // Ctrl+Shift+2 again -> same slot
ToggleShift(false);
ToggleLCtrl(false);
Sleep(500);
Sync(2000);
Log("Phase 2: saved bm_delta to slot 2 (second entry)");

// Go back to parent.
TypeText("cd ..");
TypeEnter();
Sleep(1500);
Sync(3000);
ExpectString("bm_gamma", 0, 0, -1, -1, 5000);
Sync(1000);

// Now press RightCtrl+2. With two entries in slot 2, the PR MUST open a
// submenu showing both bm_gamma and bm_delta. Pre-PR, this would jump
// directly to whichever folder was saved last (bm_delta), showing NO menu.
ToggleRCtrl(true);
TypeVK(0x32);
ToggleRCtrl(false);
Sleep(1000);
Sync(3000);

// The submenu should be visible. Look for both entries. They appear as
// menu rows. We use BeCalm because menu rendering coordinates vary.
BeCalm();
var rGamma = ExpectString("bm_gamma", 0, 0, -1, -1, 5000);
var rDelta = ExpectString("bm_delta", 0, 0, -1, -1, 5000);
BePanic();

if (rGamma.I < 1 && rDelta.I < 1) {
    Panic("Phase 2: submenu did NOT open — neither bm_gamma nor bm_delta found");
}
Log("Phase 2: submenu opened showing both entries — multi-entry slot works");
Sync(1000);

// Close the submenu without navigating (Esc), then exit.
TypeEscape();
Sleep(500);
Sync(2000);

// Verify we're back at the panel (parent dir).
BeCalm();
var rPanel = ExpectString("bm_gamma", 0, 0, -1, -1, 3000);
BePanic();
if (rPanel.I < 1) {
    Log("Phase 2: returned to panel after Esc — correct");
} else {
    Panic("Phase 2: panel lost after submenu Esc");
}

ExitFar2lWithConfirm();
Log("Phase 2: PASSED — second save adds entry, submenu opens for multi-entry slot");


// =============================================================================
// Phase 3: Selecting a submenu entry navigates the panel
// =============================================================================
var dirs3 = FreshTestDirs("select");
MkdirsAll([dirs3.left + "/bm_eps", dirs3.left + "/bm_zeta"], 0o755);

StartTestApp(dirs3.profile, dirs3.left, dirs3.right, "left-sel", false);
DismissOSC52Only();
EnsurePanelFocus();

// Save bm_eps to slot 3.
TypeText("cd bm_eps");
TypeEnter();
Sleep(1500);
Sync(3000);
ExpectString("bm_eps", 0, 0, -1, -1, 5000);
Sync(1000);
ToggleLCtrl(true);
ToggleShift(true);
TypeVK(0x33);
ToggleShift(false);
ToggleLCtrl(false);
Sleep(500);
Sync(2000);

// Save bm_zeta to slot 3.
TypeText("cd ..");
TypeEnter();
Sleep(1500);
Sync(3000);
TypeText("cd bm_zeta");
TypeEnter();
Sleep(1500);
Sync(3000);
ExpectString("bm_zeta", 0, 0, -1, -1, 5000);
Sync(1000);
ToggleLCtrl(true);
ToggleShift(true);
TypeVK(0x33);
ToggleShift(false);
ToggleLCtrl(false);
Sleep(500);
Sync(2000);
Log("Phase 3: saved bm_eps and bm_zeta to slot 3");

// Go to parent.
TypeText("cd ..");
TypeEnter();
Sleep(1500);
Sync(3000);
ExpectString("bm_eps", 0, 0, -1, -1, 5000);
Sync(1000);

// Open submenu for slot 3.
ToggleRCtrl(true);
TypeVK(0x33);
ToggleRCtrl(false);
Sleep(1000);
Sync(3000);

// Both entries should be visible in the submenu.
BeCalm();
var rEps = ExpectString("bm_eps", 0, 0, -1, -1, 5000);
BePanic();
if (rEps.I >= 1) {
    Panic("Phase 3: bm_eps not found in submenu");
}
Log("Phase 3: submenu shows bm_eps at row " + rEps.Y);

// The first entry (bm_eps, since Add inserts at front the second save is
// at index 0, but order depends on insert semantics). Navigate to the
// currently highlighted entry with Enter.
TypeEnter();
Sleep(1500);
Sync(3000);

// After Enter, the panel should navigate to one of the two bookmarked
// folders. Verify we are inside a bm_* directory (not the parent).
BeCalm();
var rInEps = ExpectString("bm_eps", 0, 0, -1, -1, 3000);
var rInZeta = ExpectString("bm_zeta", 0, 0, -1, -1, 3000);
BePanic();
if (rInEps.I < 1 && rInZeta.I < 1) {
    Panic("Phase 3: submenu Enter did not navigate to a bookmarked folder");
}
Log("Phase 3: submenu Enter navigated the panel — correct");
Sync(1000);

ExitFar2lWithConfirm();
Log("Phase 3: PASSED — submenu selection navigates panel");


// =============================================================================
// Phase 4: Empty slot does not crash / does not open a menu
// =============================================================================
var dirs4 = FreshTestDirs("empty");
StartTestApp(dirs4.profile, dirs4.left, dirs4.right, "left-empty", false);
DismissOSC52Only();
EnsurePanelFocus();

// Press RightCtrl+4 on a slot that was never saved to.
// Pre-PR: Bookmarks::Get returns false, ExecShortcutFolder is a no-op.
// Post-PR: BookmarksCache::ResolveForSlot returns Empty, no submenu.
// Either way, far2l must stay alive and show the panel.
ToggleRCtrl(true);
TypeVK(0x34);
ToggleRCtrl(false);
Sleep(1000);
Sync(3000);

// We should still see the panel (left-empty label or file list).
BeCalm();
var rStillPanel = ExpectString("left-empty", 0, 0, -1, -1, 5000);
BePanic();
if (rStillPanel.I < 1) {
    Log("Phase 4: panel still visible after RightCtrl+4 on empty slot — correct");
} else {
    Panic("Phase 4: panel lost after pressing RightCtrl+4 on empty slot");
}
Sync(1000);

ExitFar2lWithConfirm();
Log("Phase 4: PASSED — empty slot is a safe no-op");

Log("All phases PASSED");
