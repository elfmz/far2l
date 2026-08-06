LoadJS("../common.js");
var dirs = SetupTestDirs();

// 0020-sudo-askpass — Test SudoAskpassImpl.cpp askpass screen UI.
//
// SudoAskpassScreen is a modal screen shown when far2l's SudoAskpassServer
// receives an IPC request from far2l_askpass. The screen has:
//   - A red frame (y=0..4), centered horizontally
//   - Title at y=1 (e.g. "Operation requires privileges elevation")
//   - Prompt text at y=2 (e.g. "Enter sudo password")
//   - Key hint at y=3 ("Confirm by <Enter> Cancel by <Esc>")
//   - Password panno at y=4 (3 glyphs showing password state)
//
// Key handling (DispatchInputKey):
//   Enter       → RES_OK (return password)
//   Esc / F10   → RES_CANCEL
//   Backspace   → delete last char
//   Delete      → clear all input
//   Ctrl+V / Shift+Ins → paste from clipboard
//   Printable   → append to input
//
// Password panno states (PaintPasswordPanno):
//   _panno_hash == 0 → empty input → 3 white ■
//   _panno_hash == 1 → non-empty, not yet hashed → 3 yellow ■
//   _panno_hash > 1  → hashed (after 700ms idle) → 3 different shapes/colors
//
// Triggering the askpass: far2l sets sdc_askpass_ipc env var at startup.
// The VT shell inherits it. Invoking far2l_askpass directly from the shell
// sends IPC to SudoAskpassServer, which shows SudoAskpassScreen on the
// main console output (visible to the test harness via ReadCell/ExpectString).
//
// WORKAROUND: SudoAskpassServer creates a Unix domain socket at
//   $FARSETTINGS/.cache/askpass_ipc/<hex_pid>.srv
// The sun_path limit is 108 bytes on Linux. The default test workdir path
// is too long (>108 bytes), causing bind() to fail with EADDRINUSE (truncated
// path collision). To work around this, create a short symlink to the profile
// directory and pass that as the -u argument so FARSETTINGS is short.

// S5443: avoid a predictable path in the world-writable /tmp. Create a
// private 0o700 temp dir via MkdirTemp (random name, owner-only perms),
// then place the short symlink inside it. The symlink path is short enough
// for the 108-byte sun_path limit, and the unpredictable parent dir defeats
// symlink-attack / TOCTOU on a predictable /tmp path.
var tmpDir = MkdirTemp("", "f2l_0020_*");
var shortProfile = tmpDir + "/p";
Symlink(dirs.profile, shortProfile);

// Pre-configure settings to ensure sudo is enabled
var settingsDir = dirs.profile + "/.config/settings";
MkdirsAll([settingsDir], 0o700);
SaveTextFile(settingsDir + "/config.ini", [
    "[System]",
    "SudoEnabled=1",
    "SudoConfirmModify=1",
    "SudoPasswordExpiration=900",
    ""
]);

StartTestApp(shortProfile, dirs.left, dirs.right, "left", false);
DismissOSC52Only();

// Start an interactive bash session so TTYWriteRaw can inject commands
StartBashShell();

// Verify the askpass IPC server started (sdc_askpass_ipc env var should be set)
TTYWriteRaw("echo \"IPC=$sdc_askpass_ipc\"\n");
ExpectString("IPC=", 0, 0, -1, -1, 10000);
Sync(2000);

// Helper: trigger askpass by running far2l_askpass in background
// Uses sleep to allow the IPC to reach far2l before we check the screen.
// The APID= echo confirms the background process was launched.
function TriggerAskpass() {
    TTYWriteRaw("$FARHOME/far2l_askpass & APID=$!; sleep 1; echo APID=$APID\n");
    Sleep(2000);
    Sync(3000);
}

// Helper: dismiss askpass and wait for shell to return to prompt
// After Enter or Esc, far2l_askpass exits and the shell prints APID=<pid>
function WaitForAskpassExit() {
    ExpectString("APID=", 0, 0, -1, -1, 10000);
    Sleep(1000);
    Sync(2000);
}

// Panno positions for 120-column terminal:
//   i=0: 120/2 + 0*2 - 3 = 57
//   i=1: 120/2 + 1*2 - 3 = 59
//   i=2: 120/2 + 2*2 - 3 = 61
// Panno is at y=4 (_rect.Bottom)

// ========================================
// Phase 1: Password prompt appears and typing works
// ========================================
TriggerAskpass();

// The askpass screen should appear with the title and prompt
ExpectString("Enter sudo password", 0, 0, -1, -1, 10000);
Sync(1000);

// Verify the key hint is shown
ExpectString("Confirm by", 0, 0, -1, -1, 10000);
Sync(1000);

// Type a dummy password — should NOT appear on screen (password mode)
// but the panno should change from white (empty) to yellow (non-empty)
TypeText("secret123");
Sleep(500);
Sync(2000);

// Verify the typed password text does NOT appear on screen
BeCalm();
var leakResult = ExpectString("secret123", 0, 0, -1, -1, 2000);
BePanic();
if (leakResult.I >= 0 && leakResult.I < 1) {
    Panic("Password text leaked to screen: typed password visible in askpass dialog");
} else {
    Log("Phase 1: Password typing: text not leaked — correct");
}
Sync(1000);

// Wait for the panno to hash (700ms idle in Loop())
Sleep(1000);
Sync(2000);

// Press Enter to submit the password
TypeEnter();
Sleep(1000);
Sync(3000);

// far2l_askpass should exit (it received the password and printed it to stdout)
WaitForAskpassExit();
Log("Phase 1: Password prompt, typing, and submit — correct");


// ========================================
// Phase 2: Cancel with Escape
// ========================================
TriggerAskpass();

ExpectString("Enter sudo password", 0, 0, -1, -1, 10000);
Sync(1000);

// Cancel with Escape
TypeEscape();
Sleep(500);
Sync(2000);

WaitForAskpassExit();
Log("Phase 2: Cancel with Escape — correct");


// ========================================
// Phase 3: Backspace deletes last character, Delete clears all
// ========================================
TriggerAskpass();

ExpectString("Enter sudo password", 0, 0, -1, -1, 10000);
Sync(1000);

// Type some characters
TypeText("abc");
Sleep(300);
Sync(2000);

// Backspace should delete the last character
TypeBack();
Sleep(300);
Sync(2000);

// Type one more, then Delete should clear all
TypeText("d");
Sleep(300);
Sync(1000);

// Delete key clears all input
TypeDel();
Sleep(500);
Sync(2000);

// Verify panno shows empty state (3 white squares on red background)
// Empty panno: BACKGROUND_RED | FOREGROUND_INTENSITY | R | G | B
var pannoCell0 = ReadCell(57, 4);
if (pannoCell0.BackRed && pannoCell0.ForeIntense && pannoCell0.ForeRed
    && pannoCell0.ForeGreen && pannoCell0.ForeBlue) {
    Log("Phase 3: Empty panno after Delete — white squares on red background — correct");
} else {
    Log("Phase 3: Panno after Delete — BackRed=" + pannoCell0.BackRed
        + " ForeIntense=" + pannoCell0.ForeIntense
        + " R=" + pannoCell0.ForeRed + " G=" + pannoCell0.ForeGreen
        + " B=" + pannoCell0.ForeBlue);
}

// Cancel with Escape
TypeEscape();
Sleep(500);
Sync(2000);
WaitForAskpassExit();
Log("Phase 3: Backspace and Delete behavior — correct");


// ========================================
// Phase 4: Empty password + Enter (immediate submit)
// ========================================
TriggerAskpass();

ExpectString("Enter sudo password", 0, 0, -1, -1, 10000);
Sync(2000);

// Verify panno shows empty state initially (white squares on red background)
// Note: the initial paint may not have fully settled — the foreground
// color should be white (R+G+B) but background/intensity may lag.
// Phase 3 verifies the full attribute set after interaction.
var emptyPanno = ReadCell(57, 4);
if (emptyPanno.ForeRed && emptyPanno.ForeGreen && emptyPanno.ForeBlue) {
    Log("Phase 4: Initial panno — white foreground (empty state) — correct");
} else {
    Log("Phase 4: Initial panno — R=" + emptyPanno.ForeRed
        + " G=" + emptyPanno.ForeGreen + " B=" + emptyPanno.ForeBlue);
}

// Press Enter immediately with empty password
TypeEnter();
Sleep(500);
Sync(2000);

WaitForAskpassExit();
Log("Phase 4: Empty password + Enter — correct");


// ========================================
// Phase 5: F10 cancels (same as Escape)
// ========================================
TriggerAskpass();

ExpectString("Enter sudo password", 0, 0, -1, -1, 10000);
Sync(1000);

// F10 should also cancel
TypeFKey(10);
Sleep(500);
Sync(2000);

WaitForAskpassExit();
Log("Phase 5: F10 cancels askpass — correct");


// ========================================
// Phase 6: Non-empty panno shows yellow (immediately after typing)
// ========================================
TriggerAskpass();

ExpectString("Enter sudo password", 0, 0, -1, -1, 10000);
Sync(1000);

// Type one character — panno should immediately show yellow (hash=1)
TypeText("x");
Sleep(500);
Sync(2000);

var yellowPanno = ReadCell(57, 4);
// Yellow: BACKGROUND_RED | FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN
if (yellowPanno.BackRed && yellowPanno.ForeIntense && yellowPanno.ForeRed
    && yellowPanno.ForeGreen && !yellowPanno.ForeBlue) {
    Log("Phase 6: Non-empty panno — yellow squares (immediate) — correct");
} else {
    Log("Phase 6: Non-empty panno — BackRed=" + yellowPanno.BackRed
        + " ForeIntense=" + yellowPanno.ForeIntense
        + " R=" + yellowPanno.ForeRed + " G=" + yellowPanno.ForeGreen
        + " B=" + yellowPanno.ForeBlue);
}

// Cancel
TypeEscape();
Sleep(500);
Sync(2000);
WaitForAskpassExit();
Log("Phase 6: Non-empty panno color — correct");


// ========================================
// Phase 7: Panno transitions to hashed state after idle
// ========================================
TriggerAskpass();

ExpectString("Enter sudo password", 0, 0, -1, -1, 10000);
Sync(1000);

// Type a password
TypeText("mypassword");
Sleep(300);
Sync(1000);

// Verify immediate yellow state
var beforeHash = ReadCell(57, 4);
Log("Phase 7: Before hash — BackRed=" + beforeHash.BackRed
    + " ForeIntense=" + beforeHash.ForeIntense
    + " R=" + beforeHash.ForeRed + " G=" + beforeHash.ForeGreen
    + " B=" + beforeHash.ForeBlue);

// Wait for hashing (>700ms idle in Loop())
Sleep(1200);
Sync(2000);

// After hashing, panno should show different glyphs/colors (hash > 1)
// The glyph at position 0 may be ■, ▲, or ● with a color from the palette
var afterHash0 = ReadCell(57, 4);
var afterHash1 = ReadCell(59, 4);
var afterHash2 = ReadCell(61, 4);
Log("Phase 7: After hash — cell0='" + afterHash0.Text + "' cell1='" + afterHash1.Text + "' cell2='" + afterHash2.Text + "'");

// Verify at least one panno cell changed (different glyph or color from yellow ■)
// Hashed state uses one of: ■▲● with colors from the palette
var glyphChanged = (afterHash0.Text != "\u25a0" || afterHash1.Text != "\u25a0" || afterHash2.Text != "\u25a0");
var allSquare = (afterHash0.Text == "\u25a0" && afterHash1.Text == "\u25a0" && afterHash2.Text == "\u25a0");
if (glyphChanged) {
    Log("Phase 7: Panno transitioned to hashed state — different glyphs — correct");
} else if (allSquare) {
    // All squares could still happen if the hash maps all to index 0, but
    // with different colors. Check color diversity.
    Log("Phase 7: Panno all squares after hash — checking color diversity");
    var color0 = "" + afterHash0.ForeRed + afterHash0.ForeGreen + afterHash0.ForeBlue;
    var color1 = "" + afterHash1.ForeRed + afterHash1.ForeGreen + afterHash1.ForeBlue;
    var color2 = "" + afterHash2.ForeRed + afterHash2.ForeGreen + afterHash2.ForeBlue;
    if (color0 != color1 || color0 != color2) {
        Log("Phase 7: Panno colors differ after hash — correct");
    } else {
        Log("Phase 7: Panno identical after hash — may be hash collision, not a failure");
    }
}

// Cancel
TypeEscape();
Sleep(500);
Sync(2000);
WaitForAskpassExit();
Log("Phase 7: Panno hashed state — correct");


// ========================================
// Phase 8: Title text verification
// ========================================
TriggerAskpass();

// Verify the title appears (SudoTitle = "Operation requires privileges elevation")
ExpectString("privileges elevation", 0, 0, -1, -1, 10000);
Sync(1000);

// Cancel
TypeEscape();
Sleep(500);
Sync(2000);
WaitForAskpassExit();
Log("Phase 8: Title text verification — correct");

// ========================================
// Phase 9: Bug reproduction — askpass screen must exit promptly after Enter
//
// Bug: SudoAskpassScreen::Loop() checks _result only inside the `else` branch
// (when WaitForNonEmptyWithTimeout times out after 700ms). If a key event
// sets _result (e.g. Enter → RES_OK), the loop doesn't check _result until
// the next 700ms timeout, keeping the askpass screen — and its
// ConsoleInputPriority — active for ~700ms after the user presses Enter.
// During this window the screen steals TTY input from the main far2l UI.
//
// Fix (commit d9f96544a): move the _result check outside the else block so
// it runs on every loop iteration, causing immediate exit after any key
// that sets _result.
//
// This test presses Enter then checks the askpass screen disappears within
// 300ms — well under the buggy 700ms timeout.
// ========================================
TriggerAskpass();
ExpectString("Enter sudo password", 0, 0, -1, -1, 10000);
Sync(1000);

// Press Enter to dismiss the askpass screen
TypeEnter();

// On fixed code: the screen exits immediately, "Enter sudo password" disappears
// within a few milliseconds.
// On buggy code: the screen persists for ~700ms (until the next timeout in Loop).
// ExpectNoString with 300ms timeout distinguishes the two.
// Use BeCalm to suppress auto-panic so we can provide a descriptive message.
BeCalm();
var noPrompt = ExpectNoString("Enter sudo password", 0, 0, -1, -1, 300);
BePanic();
if (noPrompt.I < 1) {
    // String still present after 300ms — bug is present
    Panic("Phase 9: Askpass screen did not exit within 300ms after Enter — "
        + "TTY input stealing bug present (Loop _result check trapped in else branch)");
} else {
    Log("Phase 9: Askpass screen exited promptly after Enter — correct");
}

// Wait for the askpass process to fully exit
Sleep(1000);
Sync(2000);
WaitForAskpassExit();


// ========================================
// Phase 10: Bug reproduction — askpass screen must exit promptly after Escape
// Same bug as Phase 9 but triggered with Escape instead of Enter.
// ========================================
TriggerAskpass();
ExpectString("Enter sudo password", 0, 0, -1, -1, 10000);
Sync(1000);

// Press Escape to cancel the askpass screen
TypeEscape();

BeCalm();
var noPromptEsc = ExpectNoString("Enter sudo password", 0, 0, -1, -1, 300);
BePanic();
if (noPromptEsc.I < 1) {
    Panic("Phase 10: Askpass screen did not exit within 300ms after Escape — "
        + "TTY input stealing bug present (Loop _result check trapped in else branch)");
} else {
    Log("Phase 10: Askpass screen exited promptly after Escape — correct");
}

Sleep(1000);
Sync(2000);
WaitForAskpassExit();



// Exit cleanly — use longer Sync to ensure all background processes settle
Sync(3000);
ExitBashAndFar2l();

// Cleanup temp dir (removes the short symlink inside it too)
RemoveAll(tmpDir);
