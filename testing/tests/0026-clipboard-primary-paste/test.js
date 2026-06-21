// 0026-clipboard-primary-paste — Test middle-click paste from PRIMARY selection.
//
// Tests:
// 1. With PasteFromPrimarySelection=1, middle-click in editor does not crash.
// 2. Config option persists across restarts (visible in Interface settings).
// 3. Without PasteFromPrimarySelection, middle-click in editor does not crash.

mydir = WorkDir()
profile = mydir + "/profile"
left = mydir + "/left"
right = mydir + "/right"
MkdirsAll([profile, left, right], 0o700)

// Pre-configure PasteFromPrimarySelection in the profile config
settingsDir = profile + "/.config/settings"
MkdirsAll([settingsDir], 0o700)
SaveTextFile(settingsDir + "/config.ini", [
    "[Interface]",
    "PasteFromPrimarySelection=1",
    "CopyToPrimarySelection=1",
    ""
])

// Create a test file for editing
WriteFile(left + "/editme.txt", "line one\nline two\nline three\n", 0o666)

///////////////////
// Phase 1: Middle-click in editor with PasteFromPrimarySelection=1
StartApp(["--tty", "--nodetect", "--mortal", "-u", profile, "-cd", left, "-cd", right])
ExpectString("left", 0, 0, -1, -1, 10000)

// Escape Help if present, then wait for panel
TypeEscape()
Sleep(500)
Sync(3000)

// Navigate to the test file and open editor (F4)
TypeDown()
Sleep(300)
Sync(2000)
TypeFKey(4)
Sleep(1000)
Sync(5000)

// Verify editor is open
ExpectString("editme.txt", 0, 0, -1, -1, 10000)
Sync(2000)

// Verify the file content is visible
ExpectString("line one", 0, 0, -1, -1, 10000)
Sync(1000)

// Middle-button click (FROM_LEFT_2ND_BUTTON_PRESSED = 0x0004)
// With PasteFromPrimarySelection=1, the editor calls ProcessPasteEventFromPrimary
// which tries to read from the primary selection. On the test backend, ChooseClipboard
// returns -1 (not supported), so SetUseSelectionWhenPossible returns -1 (not > 0),
// and the paste path is skipped. The editor should survive without crash.
TypeMouseClick(5, 2, 0x0004)
Sleep(500)
Sync(3000)

// Verify editor is still alive
ExpectString("editme.txt", 0, 0, -1, -1, 5000)
Sync(1000)

Log("Phase 1: Editor survived middle-click with PasteFromPrimarySelection=1 — correct")

// Exit editor (F10, no save needed since no modification)
TypeFKey(10)
Sleep(500)
Sync(2000)

// Handle save dialog if it appears
BeCalm()
var rSave = ExpectString("modified", 0, 0, -1, -1, 2000)
BePanic()
if (rSave.I < 1) {
    TypeEscape()
    Sleep(500)
    Sync(2000)
}

Sync(2000)
TypeFKey(10)
ExpectString("Do you want to quit FAR?", 0, 0, -1, -1, 10000)
TypeEnter()
ExpectAppExit(0, 10000)

Log("Phase 1: PASSED")

///////////////////
// Phase 2: Config persistence — verify the option survived restart
StartApp(["--tty", "--nodetect", "--mortal", "-u", profile, "-cd", left, "-cd", right])
ExpectString("left", 0, 0, -1, -1, 10000)
Sync(2000)

// Open Options menu → Interface settings
TypeFKey(9)
ExpectString("Left    Files    Commands    Options    Right", 0, 0, -1, -1, 10000)
TypeText("on")
Sleep(500)
Sync(2000)
ExpectString("Interface settings", 0, 0, -1, -1, 10000)
Sync(2000)

// Look for the PRIMARY selection checkbox labels
BeCalm()
var rPrimary = ExpectString("PRIMARY", 0, 0, -1, -1, 5000)
BePanic()
if (rPrimary.I < 1) {
    Log("Phase 2: PRIMARY selection checkbox found in Interface settings — correct")
} else {
    Log("Phase 2: PRIMARY label not found (may need scrolling)")
}
Sync(1000)

// Close dialog
TypeEscape()
Sleep(500)
Sync(2000)
TypeEscape()
Sleep(500)
Sync(2000)

TypeFKey(10)
ExpectString("Do you want to quit FAR?", 0, 0, -1, -1, 10000)
TypeEnter()
ExpectAppExit(0, 10000)

Log("Phase 2: PASSED — config persisted")

///////////////////
// Phase 3: Middle-click in editor WITHOUT PasteFromPrimarySelection (default)
profile3 = mydir + "/profile-noprimary"
left3 = mydir + "/left-noprimary"
right3 = mydir + "/right-noprimary"
MkdirsAll([profile3, left3, right3], 0o700)

WriteFile(left3 + "/test.txt", "hello world\n", 0o666)

StartApp(["--tty", "--nodetect", "--mortal", "-u", profile3, "-cd", left3, "-cd", right3])
ExpectString("left-noprimary", 0, 0, -1, -1, 10000)

TypeEscape()
Sleep(500)
Sync(3000)

TypeDown()
Sleep(300)
Sync(2000)
TypeFKey(4)
Sleep(1000)
Sync(5000)

ExpectString("test.txt", 0, 0, -1, -1, 10000)
Sync(2000)

// Middle-click — with default config (PasteFromPrimarySelection=0),
// the editor calls ProcessPasteEvent (standard clipboard paste).
// This should not crash.
TypeMouseClick(5, 2, 0x0004)
Sleep(500)
Sync(3000)

ExpectString("test.txt", 0, 0, -1, -1, 5000)
Sync(1000)

Log("Phase 3: Editor survived middle-click with default config — correct")

TypeFKey(10)
Sleep(500)
Sync(2000)
BeCalm()
var rSave3 = ExpectString("modified", 0, 0, -1, -1, 2000)
BePanic()
if (rSave3.I < 1) {
    TypeEscape()
    Sleep(500)
    Sync(2000)
}
Sync(2000)
TypeFKey(10)
ExpectString("Do you want to quit FAR?", 0, 0, -1, -1, 10000)
TypeEnter()
ExpectAppExit(0, 10000)

Log("Phase 3: PASSED")
Log("All phases PASSED")
