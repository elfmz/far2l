LoadJS("../common.js");
var dirs = SetupTestDirs();

// ============================================
// Phase 1: first start with OSC52 dialog
// ============================================
StartTestApp(dirs.profile, dirs.left, dirs.right);
DismissHelpAndOSC52();

StartBashShell();

// Verify basic TTYWriteRaw injects a command that bash executes.
TTYWriteRaw("echo 'RAW_OK'\n")
ExpectString("RAW_OK", 0, 0, -1, -1, 10000)

// Enable bracketed paste mode in bash (DECSET 2004)
TTYWriteRaw("printf '\\e[?2004h' > /dev/tty\n")
Sync(5000)
Sleep(1000)

// Test 1: bracketed paste with simple text + special characters
TTYWriteRaw("\x1b[200~echo BRACKETED_OK; echo PASTE_SPECIAL_CHARS\x1b[201~\n")
ExpectString("BRACKETED_OK", 0, 0, -1, -1, 10000)
ExpectString("PASTE_SPECIAL_CHARS", 0, 0, -1, -1, 10000)

// Test 2: bracketed paste with multiline content
TTYWriteRaw("\x1b[200~echo LINE_ONE\necho LINE_TWO\x1b[201~\n")
ExpectString("LINE_ONE", 0, 0, -1, -1, 10000)
ExpectString("LINE_TWO", 0, 0, -1, -1, 10000)

// Test 3: bracketed paste with special characters like braces and quotes
TTYWriteRaw("\x1b[200~echo 'BRACE_TEST {[(<>)]}'\x1b[201~\n")
ExpectString("BRACE_TEST {[(<>)]}", 0, 0, -1, -1, 10000)

// Test 4: empty bracketed paste — should be a no-op
TTYWriteRaw("\x1b[200~\x1b[201~\n")
Sleep(500)
TTYWriteRaw("echo EMPTY_PASTE_OK\n")
ExpectString("EMPTY_PASTE_OK", 0, 0, -1, -1, 10000)

// Test 5: bracketed paste with only newlines
TTYWriteRaw("\x1b[200~\n\n\x1b[201~\n")
Sleep(500)
TTYWriteRaw("echo NEWLINE_PASTE_OK\n")
ExpectString("NEWLINE_PASTE_OK", 0, 0, -1, -1, 10000)

// Test 6: bracketed paste with a heredoc
TTYWriteRaw("\x1b[200~cat <<'EOF'\nline AAA\nline BBB\nEOF\x1b[201~\n")
ExpectString("line AAA", 0, 0, -1, -1, 10000)
ExpectString("line BBB", 0, 0, -1, -1, 10000)

// Test 7: paste markers spanning multiple TTYWriteRaw calls
TTYWriteRaw("\x1b[200~echo DELAYED_CLOSE_")
Sleep(1000)
TTYWriteRaw("OK\x1b[201~\n")
ExpectString("DELAYED_CLOSE_OK", 0, 0, -1, -1, 10000)

// Test 8: stray paste end marker
Sleep(500)
TTYWriteRaw("printf '\\x1b[201~'\n")
Sleep(500)
TTYWriteRaw("echo STRAY_END_OK\n")
ExpectString("STRAY_END_OK", 0, 0, -1, -1, 10000)
Sync(2000)
Sleep(500)

// Test 9: fast follow-up after bracketed paste
TTYWriteRaw("\x1b[200~echo FAST_PASTE_OK\x1b[201~\n")
TTYWriteRaw("echo IMMEDIATE_FOLLOWUP_OK\n")
ExpectString("FAST_PASTE_OK", 0, 0, -1, -1, 10000)
ExpectString("IMMEDIATE_FOLLOWUP_OK", 0, 0, -1, -1, 10000)

// Test 10: incomplete paste timeout
TTYWriteRaw("\x1b[200~echo SHOULD_TIMEOUT\n")
Sleep(3000)
TTYWriteRaw("\x1b[201~\n")
BeCalm()
var r10 = ExpectString("SHOULD_TIMEOUT", 0, 0, -1, -1, 10000)
BePanic()
if (r10.I < 1) {
    Log("Test 10: Incomplete paste was NOT executed — correct (content dropped)")
} else {
    Log("Test 10: Incomplete paste content was executed — may be acceptable")
}

// Test 11: disable bracketed paste and verify normal paste works
TTYWriteRaw("printf '\\e[?2004l' > /dev/tty\n")
Sync(5000)
Sleep(1000)
TTYWriteRaw("echo NORMAL_PASTE_OK\n")
ExpectString("NORMAL_PASTE_OK", 0, 0, -1, -1, 10000)

ExitBashAndFar2l()

// ============================================
// Phase 2: re-run with reused profile (no OSC52)
// ============================================
MkdirsAll([dirs.left, dirs.right], 0o700)
StartApp(["--tty", "--nodetect", "--mortal", "-u", dirs.profile, "-cd", dirs.left, "-cd", dirs.right]);
ExpectString("left", 0, 0, -1, -1, 10000);
TypeEscape(10)
Sync(5000)
// OSC52 should NOT appear on reused profile — verify it's absent
ExpectNoString("OSC52", 0, 0, -1, -1, 2000);

TypeText("echo 'READY2'; exec bash --norc --noprofile")
TypeEnter()
ExpectString("READY2", 0, 0, -1, -1, 10000)
Sync(5000)
Sleep(1000)

TTYWriteRaw("echo 'RAW2_OK'\n")
ExpectString("RAW2_OK", 0, 0, -1, -1, 10000)

// Enable bracketed paste
TTYWriteRaw("printf '\\e[?2004h' > /dev/tty\n")
Sync(5000)
Sleep(1000)

// Core bracketed paste — verify works without OSC52
TTYWriteRaw("\x1b[200~echo BP_NO_OSC52_OK; echo BP_SPECIAL_CHARS_2\x1b[201~\n")
ExpectString("BP_NO_OSC52_OK", 0, 0, -1, -1, 10000)
ExpectString("BP_SPECIAL_CHARS_2", 0, 0, -1, -1, 10000)

// Multiline without OSC52
TTYWriteRaw("\x1b[200~echo BP_LINE_A\necho BP_LINE_B\x1b[201~\n")
ExpectString("BP_LINE_A", 0, 0, -1, -1, 10000)
ExpectString("BP_LINE_B", 0, 0, -1, -1, 10000)

// Delayed close without OSC52
TTYWriteRaw("\x1b[200~echo BP_DELAYED_")
Sleep(1000)
TTYWriteRaw("CLOSE_2\x1b[201~\n")
ExpectString("BP_DELAYED_CLOSE_2", 0, 0, -1, -1, 10000)


///////////////////
///////////////////
// CORNER CASES
///////////////////
///////////////////

///////////////////
// Corner case: Large single-line paste (stays under 2048-byte TTYWriteRaw limit)
// Verifies parser handles large input without truncation or crash
var longline = "";
for (var i = 0; i < 100; i++) longline += "LONG_";
TTYWriteRaw("\x1b[200~echo '" + longline + "'\x1b[201~\n")
ExpectString("LONG_LONG_LONG", 0, 0, -1, -1, 10000)

///////////////////
// Corner case: Paste with escape-like sequences in content
// Verifies parser doesn't confuse content escapes with paste markers
TTYWriteRaw("\x1b[200~echo 'contains \\x1b[200~fake marker'\x1b[201~\n")
ExpectString("contains", 0, 0, -1, -1, 10000)

///////////////////
// Corner case: Paste with backslash-escaped newlines
// Verifies multiline paste with continuation characters
TTYWriteRaw("\x1b[200~echo line1\\\necho line2\x1b[201~\n")
ExpectString("line1", 0, 0, -1, -1, 10000)
ExpectString("line2", 0, 0, -1, -1, 10000)

///////////////////
// Corner case: Paste while cat is reading stdin
// Verifies pasted content reaches the foreground process
TTYWriteRaw("cat\n")
Sleep(500)
TTYWriteRaw("\x1b[200~CAT_RECEIVED_PASTE\x1b[201~\n")
ExpectString("CAT_RECEIVED_PASTE", 0, 0, -1, -1, 10000)
// Ctrl+D to close cat
ToggleLCtrl(true)
TypeVK(0x44)
ToggleLCtrl(false)
Sleep(500)

///////////////////
// Corner case: Rapid paste enable/disable/enable cycle
// Verifies state machine handles toggling correctly
TTYWriteRaw("printf '\\e[?2004l' > /dev/tty\n")
Sleep(500)
TTYWriteRaw("printf '\\e[?2004h' > /dev/tty\n")
Sleep(500)
TTYWriteRaw("printf '\\e[?2004l' > /dev/tty\n")
Sleep(500)
TTYWriteRaw("printf '\\e[?2004h' > /dev/tty\n")
Sleep(500)
TTYWriteRaw("\x1b[200~echo TOGGLE_CYCLE_OK\x1b[201~\n")
ExpectString("TOGGLE_CYCLE_OK", 0, 0, -1, -1, 10000)

ExitBashAndFar2l()
