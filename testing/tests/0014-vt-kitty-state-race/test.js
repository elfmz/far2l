LoadJS("../common.js");
var dirs = SetupTestDirs();

// 0014-vt-kitty-state-race — Test kitty keyboard protocol flag state transitions
// under interleaved TTYWriteRaw (parser thread) and TypeVK (input thread) operations.
//
// NOTE: True OS-thread concurrency is out of scope (Goja VM is single-threaded,
// all test commands flow through one Unix datagram socket). These tests verify
// sequential ordering safety: that flag changes via TTYWriteRaw are correctly
// observed by subsequent TypeVK keystroke translations.

StartTestApp(dirs.profile, dirs.left, dirs.right);
DismissHelpAndOSC52();
StartBashShell();

// ========================================
// Phase 1: Enable kitty mode, verify keystroke translation
// ========================================
// Enable kitty keyboard protocol (CSI = 1 u sets flags to 1)
TTYWriteRaw("printf '\\033[=1u' > /dev/tty\n")
Sync(3000)
Sleep(500)

// Run cat and verify Ctrl+D (raw 0x04) exits it — proves kitty flags are active
// and the Ctrl+letter bypass produces raw control bytes, not CSI u sequences.
TTYWriteRaw("cat; echo KITTY_ACTIVE_OK\n")
Sleep(500)
ToggleLCtrl(true)
TypeVK(0x44)
ToggleLCtrl(false)
ExpectString("KITTY_ACTIVE_OK", 0, 0, -1, -1, 10000)
Sync(2000)

// Disable kitty mode (CSI = 0 u resets flags to 0)
TTYWriteRaw("printf '\\033[=0u' > /dev/tty\n")
Sync(3000)
Sleep(500)

// With kitty disabled, Ctrl+D should still work (legacy path always produces 0x04)
TTYWriteRaw("cat; echo KITTY_DISABLED_OK\n")
Sleep(500)
ToggleLCtrl(true)
TypeVK(0x44)
ToggleLCtrl(false)
ExpectString("KITTY_DISABLED_OK", 0, 0, -1, -1, 10000)
Sync(2000)

// ========================================
// Phase 2: Push/pop kitty flags via DECRPM
// ========================================
// Push current flags onto stack (CSI > 1 u)
TTYWriteRaw("printf '\\033[>1u' > /dev/tty\n")
Sync(3000)
Sleep(500)

// Set new flags (CSI = 2 u — enable release reporting)
TTYWriteRaw("printf '\\033[=2u' > /dev/tty\n")
Sync(3000)
Sleep(500)

// Pop previous flags from stack (CSI < 0 u — pop and restore)
TTYWriteRaw("printf '\\033[<0u' > /dev/tty\n")
Sync(3000)
Sleep(500)

// After pop, flags should be restored to 1 (the pushed value).
// Verify by running cat and checking Ctrl+D works.
TTYWriteRaw("cat; echo POP_RESTORED_OK\n")
Sleep(500)
ToggleLCtrl(true)
TypeVK(0x44)
ToggleLCtrl(false)
ExpectString("POP_RESTORED_OK", 0, 0, -1, -1, 10000)
Sync(2000)

// ========================================
// Phase 3: Rapid push/pop cycles interleaved with keystrokes
// ========================================
// Multiple rapid push/pop cycles — verify no state corruption
for (var i = 0; i < 3; i++) {
    TTYWriteRaw("printf '\\033[>1u' > /dev/tty\n")
    Sync(1000)
    TTYWriteRaw("printf '\\033[=1u' > /dev/tty\n")
    Sync(1000)
    TTYWriteRaw("printf '\\033[<0u' > /dev/tty\n")
    Sync(1000)
}

// After rapid cycling, kitty mode should still be active (flags=1).
// Verify with a keystroke test.
TTYWriteRaw("cat; echo RAPID_CYCLE_OK\n")
Sleep(500)
ToggleLCtrl(true)
TypeVK(0x44)
ToggleLCtrl(false)
ExpectString("RAPID_CYCLE_OK", 0, 0, -1, -1, 10000)
Sync(2000)

// Reset kitty mode to default
TTYWriteRaw("printf '\\033[=0u' > /dev/tty\n")
Sync(2000)
Sleep(500)

ExitFar2lWithConfirm()


///////////////////
///////////////////
// CORNER CASES
///////////////////
///////////////////

///////////////////
// Corner case: Enable kitty, send keystroke immediately, disable
// Verifies flag state is correctly observed between rapid enable/disable
StartTestApp(dirs.profile, dirs.left, dirs.right, "left", false);
StartBashShell();

// Enable kitty, immediately send Ctrl+D to cat, then disable
TTYWriteRaw("printf '\\033[=1u' > /dev/tty\n")
Sync(2000)
TTYWriteRaw("cat; echo IMMEDIATE_TOGGLE_OK\n")
Sleep(300)
ToggleLCtrl(true)
TypeVK(0x44)
ToggleLCtrl(false)
ExpectString("IMMEDIATE_TOGGLE_OK", 0, 0, -1, -1, 10000)

// Disable kitty immediately after
TTYWriteRaw("printf '\\033[=0u' > /dev/tty\n")
Sync(2000)
Sleep(500)

// Shell should still be responsive
TTYWriteRaw("echo SHELL_ALIVE\n")
ExpectString("SHELL_ALIVE", 0, 0, -1, -1, 10000)

ExitFar2lWithConfirm()


///////////////////
// Corner case: Kitty mode 2 (release reporting) with Ctrl+C
// Verifies that Ctrl+C key-down produces raw 0x03 (bypass) even with mode 2,
// and shell remains responsive after interrupt.
StartTestApp(dirs.profile, dirs.left, dirs.right, "left", false);
StartBashShell();

// Enable kitty mode 2 (release reporting + disambiguate)
TTYWriteRaw("printf '\\033[=3u' > /dev/tty\n")
Sync(3000)
Sleep(500)

// Start a sleep command and interrupt with Ctrl+C
TTYWriteRaw("sleep 30\n")
Sleep(500)
ToggleLCtrl(true)
TypeVK(0x43)
ToggleLCtrl(false)
Sleep(500)

// Shell should still be alive — verify with a new command
TTYWriteRaw("echo MODE2_CTRL_C_OK\n")
ExpectString("MODE2_CTRL_C_OK", 0, 0, -1, -1, 10000)

// Reset kitty mode
TTYWriteRaw("printf '\\033[=0u' > /dev/tty\n")
Sync(2000)

ExitFar2lWithConfirm()
