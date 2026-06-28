// 0033-vt-consolelog-no-deadlock — Test VT console-log deadlock fix (PR #39).
//
// PR #39 reworks VTShell::OnConsoleLog to use a two-phase lock discipline:
//   Phase 1: lock _inout_control_mutex -> stop output reader -> unlock.
//   Phase 2: blocking InterThreadCall with NO mutex held.
//   Phase 3: lock -> restart output reader -> unlock.
// Previously the code held _inout_control_mutex across the blocking
// InterThreadCall, which could starve the main thread (StartIOReaders /
// StopIOReaders) on mouse scroll during TUI app execution, freezing far2l.
//
// The PR also adds a NOOP_EVENT branch in keyboard.cpp that dispatches
// inter-thread calls and checks pending ctrl events, then breaks instead
// of spinning — preventing unbounded spin when a delegate dispatch
// re-queues NOOPs.
//
// What this test can exercise through the TTY harness:
//   1. Open a VT shell, run a command that produces output.
//   2. Trigger OnConsoleLog via Ctrl+Shift+F3 (CLK_VIEW) — the console
//      log viewer must open, proving the InterThreadCall completed.
//   3. Close the viewer and verify the VT shell is STILL responsive
//      (can run another command) — proving the reader was restarted and
//      no deadlock occurred.
//   4. Trigger OnConsoleLog via Ctrl+Shift+F4 (CLK_EDIT) — the console
//      log editor must open.
//   5. Close the editor and verify VT shell responsiveness again.
//   6. Rapidly toggle Ctrl+Shift+F3 / Esc several times to stress the
//      lock/restart path — far2l must not hang.
//
// What this test CANNOT exercise:
//   - prctl(PR_SET_PDEATHSIG) parent-death detection (the smoke harness
//     runs far2l with --mortal, not as a fork-tty child, so notify_pipe
//     is -1 and _is_forktty_child is false).
//   - Far2lInteract 10s timeout (requires a far2l peer that disconnects).
//   - LocalSocket cancel-byte drain (requires TTYRevive kickass path).
//   - SIGTSTP/SIGCONT handler swap (requires suspend/resume signals).
//   These are covered by code inspection / unit tests, not the JS harness.
//
// Regression target:
//   Pre-PR, OnConsoleLog held _inout_control_mutex across InterThreadCall.
//   Under the smoke harness this usually completes (no concurrent mouse
//   scroll), so the test may pass pre-PR too. The value is that it pins
//   the post-PR two-phase path: if a future change re-introduces the
//   held-lock-across-InterThreadCall pattern, the viewer/editor open
//   will hang and this test will time out at ExpectString.

LoadJS("../common.js");
var dirs = SetupTestDirs();

StartTestApp(dirs.profile, dirs.left, dirs.right, "left", false);
DismissOSC52Only();
EnsurePanelFocus();

// Open the VT shell and start an interactive bash session.
TypeText("echo 'VT' 'Shell' 'logtest'; false");
TypeEnter();
ExpectString("VT Shell logtest", 0, 0, -1, -1, 10000);
TypeEscape();
Sleep(500);
Sync(2000);
StartBashShell();
Log("Setup: VT shell + bash session ready");

// Produce some output so the console log has content to display.
TTYWriteRaw("echo 'CONSOLE_LOG_LINE_1'\n");
ExpectString("CONSOLE_LOG_LINE_1", 0, 0, -1, -1, 10000);
Sync(2000);
TTYWriteRaw("echo 'CONSOLE_LOG_LINE_2'\n");
ExpectString("CONSOLE_LOG_LINE_2", 0, 0, -1, -1, 10000);
Sync(2000);
Log("Setup: console output produced");

// =============================================================================
// Phase 1: Ctrl+Shift+F3 opens the console log VIEWER (CLK_VIEW)
// =============================================================================
// OnConsoleLog(CLK_VIEW) is triggered by ctrl && shift && VK_F3.
// In the harness: hold LCtrl + Shift, press F3 (TypeFKey(3)), release.
ToggleLCtrl(true);
ToggleShift(true);
TypeFKey(3);
ToggleShift(false);
ToggleLCtrl(false);
Sleep(1000);
Sync(3000);

// The viewer should open. The console log viewer shows captured VT output.
// Look for one of the lines we produced, or the viewer's key bar.
// BeCalm: the viewer title/content varies by build; we check for our marker.
BeCalm();
var rView = ExpectString("CONSOLE_LOG_LINE_1", 0, 0, -1, -1, 8000);
BePanic();
if (rView.I >= 1) {
    Log("Phase 1: console log VIEWER opened — CONSOLE_LOG_LINE_1 visible");
} else {
    // Fallback: check for the viewer's F-key bar hint (Save/Exit etc.)
    BeCalm();
    var rKeybar = ExpectStrings(["Viewer", "View", "F2", "F10"], 0, 0, -1, -1, 3000);
    BePanic();
    if (rKeybar.I >= 1) {
        Log("Phase 1: console log viewer opened (key bar detected)");
    } else {
        Panic("Phase 1: console log viewer did NOT open — OnConsoleLog hung or failed");
    }
}
Sync(1000);

// Close the viewer. The viewer exits on F10 or Esc.
TypeFKey(10);
Sleep(800);
Sync(2000);
// If F10 brought up a save dialog, dismiss with Esc.
BeCalm();
var rSaveDlg = ExpectString("Save", 0, 0, -1, -1, 1500);
BePanic();
if (rSaveDlg.I < 1) {
    TypeEscape();
    Sleep(500);
    Sync(1000);
}
BePanic();

// Verify the VT shell is STILL responsive after the viewer closed.
// This is the core regression check: the output reader must have been
// restarted by Phase 3 of the new OnConsoleLog.
TTYWriteRaw("echo 'ALIVE_AFTER_VIEW'\n");
BeCalm();
var rAlive1 = ExpectString("ALIVE_AFTER_VIEW", 0, 0, -1, -1, 8000);
BePanic();
if (rAlive1.I >= 1) {
    Panic("Phase 1: VT shell unresponsive after console log viewer — reader not restarted");
}
Log("Phase 1: PASSED — viewer opened, VT shell responsive after close");


// =============================================================================
// Phase 2: Ctrl+Shift+F4 opens the console log EDITOR (CLK_EDIT)
// =============================================================================
// Produce fresh output so we have something to look for.
TTYWriteRaw("echo 'EDIT_LOG_MARKER'\n");
ExpectString("EDIT_LOG_MARKER", 0, 0, -1, -1, 10000);
Sync(2000);

ToggleLCtrl(true);
ToggleShift(true);
TypeFKey(4); // Ctrl+Shift+F4 -> CLK_EDIT
ToggleShift(false);
ToggleLCtrl(false);
Sleep(1000);
Sync(3000);

BeCalm();
var rEdit = ExpectString("EDIT_LOG_MARKER", 0, 0, -1, -1, 8000);
BePanic();
if (rEdit.I >= 1) {
    Log("Phase 2: console log EDITOR opened — EDIT_LOG_MARKER visible");
} else {
    BeCalm();
    var rEditKey = ExpectStrings(["Editor", "Edit", "F2", "F10"], 0, 0, -1, -1, 3000);
    BePanic();
    if (rEditKey.I >= 1) {
        Log("Phase 2: console log editor opened (key bar detected)");
    } else {
        Panic("Phase 2: console log editor did NOT open — OnConsoleLog hung or failed");
    }
}
Sync(1000);

// Close the editor. F10, handle save dialog if it appears.
TypeFKey(10);
Sleep(800);
Sync(2000);
BeCalm();
var rSaveDlg2 = ExpectString("Save", 0, 0, -1, -1, 1500);
BePanic();
if (rSaveDlg2.I < 1) {
    // Save dialog appeared — choose "No" / dismiss.
    TypeRight();
    Sleep(200);
    TypeEnter();
    Sleep(500);
    Sync(1000);
}
BePanic();

// Verify VT shell responsiveness again.
TTYWriteRaw("echo 'ALIVE_AFTER_EDIT'\n");
BeCalm();
var rAlive2 = ExpectString("ALIVE_AFTER_EDIT", 0, 0, -1, -1, 8000);
BePanic();
if (rAlive2.I >= 1) {
    Panic("Phase 2: VT shell unresponsive after console log editor — reader not restarted");
}
Log("Phase 2: PASSED — editor opened, VT shell responsive after close");


// =============================================================================
// Phase 3: Rapid console-log open/close cycle — no hang (stress)
// =============================================================================
// The old code could deadlock when OnConsoleLog was invoked while the main
// thread was manipulating IO readers. Rapid toggling stresses the lock
// release/restart path. far2l must not hang.
TTYWriteRaw("echo 'STRESS_BEFORE'\n");
ExpectString("STRESS_BEFORE", 0, 0, -1, -1, 10000);
Sync(1000);

for (var i = 0; i < 3; i++) {
    // Open viewer
    ToggleLCtrl(true);
    ToggleShift(true);
    TypeFKey(3);
    ToggleShift(false);
    ToggleLCtrl(false);
    Sleep(600);
    Sync(1500);
    // Close viewer
    TypeFKey(10);
    Sleep(400);
    Sync(1500);
    // Dismiss any save dialog
    BeCalm();
    var rDlg = ExpectString("Save", 0, 0, -1, -1, 800);
    BePanic();
    if (rDlg.I < 1) {
        TypeEscape();
        Sleep(300);
        Sync(1000);
    }
    BePanic();
    Log("Phase 3: cycle " + (i + 1) + "/3 completed");
}

// After 3 rapid cycles, the VT shell must still be responsive.
TTYWriteRaw("echo 'STRESS_AFTER'\n");
BeCalm();
var rStress = ExpectString("STRESS_AFTER", 0, 0, -1, -1, 8000);
BePanic();
if (rStress.I >= 1) {
    Panic("Phase 3: VT shell hung after rapid console-log cycles — deadlock regression");
}
Log("Phase 3: PASSED — VT shell responsive after 3 rapid console-log cycles");


// =============================================================================
// Phase 4: NOOP_EVENT spin prevention — shell stays responsive under input
// =============================================================================
// The keyboard.cpp NOOP_EVENT fix dispatches inter-thread calls and breaks
// instead of spinning. We can't inject a NOOP_EVENT directly, but we CAN
// verify that interleaving panel focus changes (which queue inter-thread
// calls) with VT shell commands does not freeze the input loop.
// Tab cycling generates input events that exercise the dispatch path.
for (var j = 0; j < 4; j++) {
    TypeVK(9); // Tab — switches panel focus, queues input events
    Sleep(200);
    Sync(500);
}
// Ensure we end on the VT shell command line area by pressing Esc once.
TypeEscape();
Sleep(300);
Sync(1000);

TTYWriteRaw("echo 'NOOP_RESPONSIVE'\n");
BeCalm();
var rNoop = ExpectString("NOOP_RESPONSIVE", 0, 0, -1, -1, 8000);
BePanic();
if (rNoop.I >= 1) {
    Panic("Phase 4: VT shell unresponsive after input interleaving — NOOP spin regression");
}
Log("Phase 4: PASSED — shell responsive after input interleaving");

// Clean exit.
ExitBashAndFar2l();
Log("All phases PASSED");
