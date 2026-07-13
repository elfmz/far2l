LoadJS("../common.js");
var dirs = SetupTestDirs();

// 0015-vt-kitty-state-leak — Test kitty keyboard protocol state management
// across VT shell session boundaries.
//
// The key concern: _kitty_kb_flags is reset to 0 in ExecuteCommandCommonTail,
// but kitty_flag_stack (vtansi.cpp:306) is NOT cleared. Stale stack entries
// from a previous session could corrupt flag state when popped in a subsequent
// session.
//
// Cleanup: Exit bash first (TTYWriteRaw exit), then F10 to quit far2l.

// ========================================
// Phase 1: Enable kitty mode, exit shell, verify next session is clean
// ========================================
StartTestApp(dirs.profile, dirs.left, dirs.right);
DismissHelpAndOSC52();
StartBashShell();

// Enable kitty mode with push (so stack has an entry)
TTYWriteRaw("printf '\\033[>1u\\033[=1u' > /dev/tty\n")
Sync(3000)
Sleep(500)

// Verify kitty mode is active
TTYWriteRaw("cat; echo SESSION1_KITTY_OK\n")
Sleep(500)
ToggleLCtrl(true)
TypeVK(0x44)
ToggleLCtrl(false)
ExpectString("SESSION1_KITTY_OK", 0, 0, -1, -1, 10000)
Sync(2000)

// Exit bash and far2l
ExitFar2lWithConfirm()

// ========================================
// Phase 2: Start new session — verify kitty flags are clean
// ========================================
StartTestApp(dirs.profile, dirs.left, dirs.right, "left", false);
StartBashShell();

// Without enabling kitty mode, Ctrl+D should still work via legacy path
// (this verifies _kitty_kb_flags == 0 — if flags were stale, Ctrl+D
// might produce a CSI u sequence instead of raw 0x04)
TTYWriteRaw("cat; echo SESSION2_CLEAN_OK\n")
Sleep(500)
ToggleLCtrl(true)
TypeVK(0x44)
ToggleLCtrl(false)
ExpectString("SESSION2_CLEAN_OK", 0, 0, -1, -1, 10000)
Sync(2000)

// ========================================
// Phase 3: Verify kitty flag stack is clean — push/pop works correctly
// ========================================
// Push current flags (should be 0), set new flags, pop — should restore 0
TTYWriteRaw("printf '\\033[>1u\\033[=2u\\033[<0u' > /dev/tty\n")
Sync(3000)
Sleep(500)

// After pop, flags should be 0 (restored from stack which had 0 pushed).
// Verify by checking that Ctrl+D produces raw 0x04 (legacy path),
// not a kitty CSI u sequence.
TTYWriteRaw("cat; echo STACK_CLEAN_OK\n")
Sleep(500)
ToggleLCtrl(true)
TypeVK(0x44)
ToggleLCtrl(false)
ExpectString("STACK_CLEAN_OK", 0, 0, -1, -1, 10000)
Sync(2000)

// Reset kitty mode and verify bash is alive before exiting
TTYWriteRaw("printf '\\033[=0u' > /dev/tty\n")
Sync(2000)
Sleep(500)
TTYWriteRaw("echo ALIVE_BEFORE_EXIT\n")
ExpectString("ALIVE_BEFORE_EXIT", 0, 0, -1, -1, 10000)
Sync(2000)

ExitFar2lWithConfirm()


///////////////////
///////////////////
// CORNER CASES
///////////////////
///////////////////

///////////////////
// Corner case: Ctrl+C during kitty mode leaves shell responsive
// Verifies that SIGINT delivery via raw 0x03 doesn't corrupt terminal state
StartTestApp(dirs.profile, dirs.left, dirs.right, "left", false);
StartBashShell();

TTYWriteRaw("printf '\\033[=1u' > /dev/tty\n")
Sync(3000)
Sleep(500)

// Start a sleep and interrupt with Ctrl+C
TTYWriteRaw("sleep 30\n")
Sleep(500)
// Send raw 0x03 via TTYWriteRaw (bypasses all key translation)
TTYWriteRaw("\x03")
Sleep(500)

// Shell should still be alive — verify with a command
TTYWriteRaw("echo CTRL_C_RESPONSIVE\n")
ExpectString("CTRL_C_RESPONSIVE", 0, 0, -1, -1, 10000)

// Reset kitty mode
TTYWriteRaw("printf '\\033[=0u' > /dev/tty\n")
Sync(2000)

ExitFar2lWithConfirm()


///////////////////
// Corner case: Multiple rapid Ctrl+C under kitty mode don't corrupt state
StartTestApp(dirs.profile, dirs.left, dirs.right, "left", false);
StartBashShell();

TTYWriteRaw("printf '\\033[=1u' > /dev/tty\n")
Sync(3000)
Sleep(500)

// Start a sleep and send multiple rapid Ctrl+C
TTYWriteRaw("sleep 30\n")
Sleep(500)
TTYWriteRaw("\x03")
Sleep(100)
TTYWriteRaw("\x03")
Sleep(100)
TTYWriteRaw("\x03")
Sleep(500)

// Shell should still be alive
TTYWriteRaw("echo RAPID_CTRL_C_OK\n")
ExpectString("RAPID_CTRL_C_OK", 0, 0, -1, -1, 10000)

TTYWriteRaw("printf '\\033[=0u' > /dev/tty\n")
Sync(2000)

ExitFar2lWithConfirm()


///////////////////
// Corner case: Kitty mode + bracketed paste interaction
// Verifies that kitty keyboard protocol and bracketed paste don't conflict
StartTestApp(dirs.profile, dirs.left, dirs.right, "left", false);
StartBashShell();

// Enable both kitty mode and bracketed paste
TTYWriteRaw("printf '\\033[=1u\\033[?2004h' > /dev/tty\n")
Sync(3000)
Sleep(500)

// Start cat and paste some text — should work with both modes active
TTYWriteRaw("cat; echo KITTY_PASTE_OK\n")
Sleep(500)

// Send text via TTYWriteRaw (simulates paste content)
TTYWriteRaw("\x1b[200~hello world\x1b[201~\n")
Sleep(300)

// End cat with Ctrl+D
ToggleLCtrl(true)
TypeVK(0x44)
ToggleLCtrl(false)
ExpectString("KITTY_PASTE_OK", 0, 0, -1, -1, 10000)

// Disable both modes
TTYWriteRaw("printf '\\033[=0u\\033[?2004l' > /dev/tty\n")
Sync(2000)

ExitFar2lWithConfirm()
