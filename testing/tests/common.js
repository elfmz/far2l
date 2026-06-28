// common.js — Shared test functions for far2l smoke tests.
// Load with: LoadJS("../common.js")

// SetupTestDirs creates the standard profile/left/right directory layout.
// leftName/rightName default to "left"/"right" if omitted.
// Returns {mydir, profile, left, right}.
function SetupTestDirs(leftName, rightName) {
    leftName = leftName || "left";
    rightName = rightName || "right";
    var mydir = WorkDir();
    var profile = mydir + "/profile";
    var left = mydir + "/" + leftName;
    var right = mydir + "/" + rightName;
    MkdirsAll([profile, left, right], 0o700);
    return {mydir: mydir, profile: profile, left: left, right: right};
}

// FreshTestDirs creates a fresh set of profile/left/right directories
// under the test workdir with the given suffix. Reduces duplication in
// corner-case test sections that each need their own far2l instance.
// Returns {mydir, profile, left, right}.
function FreshTestDirs(suffix, leftName, rightName) {
    leftName = leftName || ("left-" + suffix);
    rightName = rightName || ("right-" + suffix);
    var mydir = WorkDir();
    var profile = mydir + "/profile-" + suffix;
    var left = mydir + "/" + leftName;
    var right = mydir + "/" + rightName;
    MkdirsAll([profile, left, right], 0o700);
    return {mydir: mydir, profile: profile, left: left, right: right};
}

// StartTestApp launches far2l with standard flags and expects the left panel
// label. leftLabel is the text shown in the left panel title bar (defaults
// to "left"). Pass null to skip the panel label check. If expectHelp is true
// (default), also expects the Help window. Optional size = [cols, rows] for
// non-default terminal dimensions.
function StartTestApp(profile, left, right, leftLabel, expectHelp, size) {
    if (leftLabel === undefined) leftLabel = "left";
    if (expectHelp === undefined) expectHelp = true;
    var args = ["--tty", "--nodetect", "--mortal", "-u", profile, "-cd", left, "-cd", right];
    if (size) {
        StartAppWithSize(args, size[0], size[1]);
    } else {
        StartApp(args);
    }
    if (leftLabel) {
        ExpectString(leftLabel, 0, 0, -1, -1, 10000);
    }
    if (expectHelp) {
        ExpectString("Help - FAR2L", 0, 0, -1, -1, 10000);
    }
}

// DismissHelpAndOSC52 closes the Help window and, on first start,
// dismisses the OSC52 clipboard dialog if it appears.
function DismissHelpAndOSC52() {
    TypeEscape(10);
    Sync(5000);
    BeCalm();
    var r = ExpectString("OSC52", 0, 0, -1, -1, 2000);
    BePanic();
    if (r.I < 1) {
        TypeEnter();
        Sleep(500);
    }
}

// ExitFar2lWithConfirm presses F10 and confirms the exit dialog.
function ExitFar2lWithConfirm() {
    TypeFKey(10);
    ExpectString("Do you want to quit FAR?", 0, 0, -1, -1, 10000);
    TypeEnter();
    ExpectAppExit(0, 10000);
}

// StartBashShell opens an interactive bash session inside far2l's VT shell.
// Used by tests that need TTYWriteRaw to inject commands.
function StartBashShell() {
    TypeText("echo 'READY'; exec bash --norc --noprofile");
    TypeEnter();
    ExpectString("READY", 0, 0, -1, -1, 10000);
    Sync(5000);
    Sleep(1000);
}

// ExitBashAndFar2l exits the interactive bash session, then exits far2l.
function ExitBashAndFar2l() {
    TTYWriteRaw("exit\n");
    Sync(5000);
    Sleep(1000);
    TypeEscape();
    TypeText("exit far");
    TypeEnter();
    ExpectAppExit(0, 10000);
}
// StartVtCornerShell launches far2l and opens a VT shell with a bash session,
// ready for corner-case testing. dirs = {profile, left, right} from SetupTestDirs.
function StartVtCornerShell(dirs) {
    StartTestApp(dirs.profile, dirs.left, dirs.right, dirs.left.split("/").pop(), false);
    TypeText("echo 'VT' 'corner'; false")
    TypeEnter()
    ExpectString("VT corner", 0, 0, -1, -1, 10000)
    TypeEscape()
    Sleep(500)
    StartBashShell()
}

// TypeAltFKey sends an Alt+F<n> key combination (e.g., Alt+F8, Alt+F11, Alt+F12).
function TypeAltFKey(n) {
    ToggleLAlt(true);
    TypeFKey(n);
    ToggleLAlt(false);
}

// EnsurePanelFocus cycles Tab twice to ensure the file panel is active.
// Many tests need this after DismissHelpAndOSC52 to recover focus.
function EnsurePanelFocus() {
    TypeVK(9); Sleep(200); TypeVK(9); Sleep(200); Sync(5000);
}

// DismissOSC52Only dismisses the OSC52 clipboard dialog on first start
// when Help is already suppressed (e.g., pre-configured profile).
// Different from DismissHelpAndOSC52 which also closes Help.
function DismissOSC52Only() {
    TypeEscape();
    Sleep(300);
    Sync(2000);
    BeCalm();
    var r = ExpectString("OSC52", 0, 0, -1, -1, 2000);
    BePanic();
    if (r.I < 1) {
        TypeEnter();
        Sleep(500);
    }
    Sync(5000);
}

// TypeCtrlEnter sends Ctrl+Enter to activate the default button in
// dialogs where plain Enter inserts text (e.g., DI_MEMOEDIT).
function TypeCtrlEnter() {
    ToggleLCtrl(true);
    TypeEnter();
    ToggleLCtrl(false);
}

// OpenMacroBrowser opens the Macro Browser via F9 → Commands menu.
// Requires far2l to be running with file panels visible.
function OpenMacroBrowser() {
    TypeFKey(9);
    ExpectString("Left    Files    Commands    Options    Right", 0, 0, -1, -1, 10000);
    Sync(1000);
    TypeRight(); Sleep(200);
    TypeRight(); Sleep(200);
    Sync(1000);
    TypeDown(); Sleep(500); Sync(2000);
    TypeEnd(); Sleep(200); Sync(1000);
    TypeUp(); Sleep(200); Sync(1000);
    ExpectString("Macro Browser", 0, 0, -1, -1, 5000);
    TypeEnter(); Sleep(500); Sync(5000);
    ExpectString("Macro Browser", 0, 0, -1, -1, 10000);
    Sync(2000);
}

// CloseMacroBrowser closes the Macro Browser and verifies return to panels.
function CloseMacroBrowser() {
    TypeEscape(); Sleep(300); Sync(2000);
    ExpectString("left", 0, 0, -1, -1, 10000);
    Sync(1000);
}

// GotoFirstMacro navigates the Macro Browser cursor to the first
// focusable macro entry. One Down from [0] "Total macros" skips
// separators to the first macro.
function GotoFirstMacro() {
    TypeDown(); Sleep(200); Sync(1000);
}

// FillNewMacroDialog opens the "New Macro" edit dialog (via Ins) and fills
// the Key, Description, and Sequence fields using the standard tab-walk.
// The caller must already be in the Macro Browser. If sequence is omitted
// (or empty), the Sequence field is left untouched (used by tests that
// exercise the empty-sequence error path). Does not press OK — the caller
// decides whether to confirm (TypeCtrlEnter) or cancel (TypeEscape), so
// error-path tests can inspect the dialog state before dismissing it.
function FillNewMacroDialog(key, description, sequence) {
    TypeIns(); Sleep(500); Sync(3000);
    ExpectString("New Macro", 0, 0, -1, -1, 10000);
    Sync(1000);
    TypeVK(9); Sleep(200); Sync(1000);    // Tab → Key field
    TypeText(key); Sleep(200); Sync(1000);
    TypeVK(9); Sleep(100); Sync(500);     // Tab → Assign Key button
    TypeVK(9); Sleep(100); Sync(500);     // Tab → Description
    TypeText(description); Sleep(200); Sync(1000);
    TypeVK(9); Sleep(200); Sync(1000);    // Tab → Sequence
    if (sequence) {
        TypeText(sequence); Sleep(200); Sync(1000);
    }
}
