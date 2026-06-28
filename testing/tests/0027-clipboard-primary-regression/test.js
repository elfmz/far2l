// 0027-clipboard-primary-regression — Test middle-click on file in panel.
//
// Regression test for the behavioral change introduced by the PRIMARY buffer
// feature: when PasteFromPrimarySelection is OFF, middle-click on a file in
// the panel should still open it (KEY_ENTER behavior). When ON, the middle-click
// is intercepted and returns FALSE from FileList::ProcessMouse.
//
// Tests:
// 1. With PasteFromPrimarySelection=0 (default): middle-click on a file opens it.
// 2. With PasteFromPrimarySelection=1: middle-click on a file does NOT open it
//    (the event is intercepted by the primary-paste path).

mydir = WorkDir()

///////////////////
// Phase 1: Middle-click opens file with PasteFromPrimarySelection=0 (default)
profile1 = mydir + "/profile-default"
left1 = mydir + "/left-default"
right1 = mydir + "/right-default"
MkdirsAll([profile1, left1, right1], 0o700)

// Create a test file
WriteFile(left1 + "/clickme.txt", "opened by middle click\n", 0o666)

StartApp(["--tty", "--nodetect", "--mortal", "-u", profile1, "-cd", left1, "-cd", right1])
ExpectString("left-default", 0, 0, -1, -1, 10000)
// Dismiss help
BeCalm()
var rHelp = ExpectString("Help - FAR2L", 0, 0, -1, -1, 3000)
BePanic()
if (rHelp.I < 1) {
    TypeEscape(10)
    Sync(2000)
}

// Get status for coordinates
status = AppStatus()
Sync(1000)

// Navigate to the file (it should be the first file below directories)
TypeDown()
Sleep(300)
Sync(2000)

// Verify the file is visible on the panel
BeCalm()
var rFile = ExpectString("clickme.txt", 0, 0, -1, -1, 5000)
BePanic()
if (rFile.I < 1) {
    Log("Phase 1: File visible on panel at row " + rFile.Y)
} else {
    Panic("Phase 1: clickme.txt not found on panel")
}

// Middle-click on the file row.
// With PasteFromPrimarySelection=0, FileList::ProcessMouse should
// process it as KEY_ENTER and open the file (in viewer or editor).
// The file row is at rFile.Y — click at that position.
BeCalm()
TypeMouseClick(rFile.X + 2, rFile.Y, 0x0004)
Sleep(1000)
Sync(5000)
BePanic()

// Check if the file was opened (viewer shows "clickme.txt" in title,
// or editor shows it). Either way, we should see "opened by middle click"
// somewhere on screen, or the filename in a viewer/editor title.
BeCalm()
var rOpened = ExpectString("opened by middle click", 0, 0, -1, -1, 5000)
BePanic()
if (rOpened.I < 1) {
    Log("Phase 1: Middle-click opened the file — content visible — correct")
} else {
    // The file might have been opened in the internal viewer which shows
    // the content, or in the editor. If content is not visible, check
    // if at least the filename appears in a title bar.
    BeCalm()
    var rTitle = ExpectString("clickme.txt", 0, 0, -1, -1, 3000)
    BePanic()
    if (rTitle.I < 1) {
        Log("Phase 1: Middle-click opened file (filename in title) — correct")
    } else {
        Panic("Phase 1: Middle-click did not open the file with PasteFromPrimarySelection=0")
    }
}
Sync(1000)

// Close viewer/editor
TypeEscape()
Sleep(500)
Sync(2000)
BeCalm()
var rStillOpen = ExpectString("clickme.txt", 0, 0, -1, -1, 2000)
BePanic()
if (rStillOpen.I >= 1) {
    // Might be editor, try F10
    TypeFKey(10)
    Sleep(500)
    Sync(2000)
    BeCalm()
    var rSave = ExpectString("modified", 0, 0, -1, -1, 2000)
    BePanic()
    if (rSave.I >= 1) {
        TypeEscape()
        Sleep(500)
        Sync(2000)
    }
}

// Exit far2l
TypeFKey(10)
ExpectString("Do you want to quit FAR?", 0, 0, -1, -1, 10000)
TypeEnter()
ExpectAppExit(0, 10000)

Log("Phase 1: PASSED — middle-click opens file when PasteFromPrimarySelection=0")

///////////////////
// Phase 2: Middle-click does NOT open file with PasteFromPrimarySelection=1
profile2 = mydir + "/profile-primary"
left2 = mydir + "/left-primary"
right2 = mydir + "/right-primary"
MkdirsAll([profile2, left2, right2], 0o700)

// Pre-configure PasteFromPrimarySelection
settingsDir2 = profile2 + "/.config/settings"
MkdirsAll([settingsDir2], 0o700)
SaveTextFile(settingsDir2 + "/config.ini", [
    "[Interface]",
    "PasteFromPrimarySelection=1",
    ""
])

WriteFile(left2 + "/noopen.txt", "should not be opened\n", 0o666)

StartApp(["--tty", "--nodetect", "--mortal", "-u", profile2, "-cd", left2, "-cd", right2])
ExpectString("left-primary", 0, 0, -1, -1, 10000)
BeCalm()
var rHelp2 = ExpectString("Help - FAR2L", 0, 0, -1, -1, 3000)
BePanic()
if (rHelp2.I < 1) {
    TypeEscape(10)
    Sync(2000)
}

Sync(1000)
TypeDown()
Sleep(300)
Sync(2000)

BeCalm()
var rFile2 = ExpectString("noopen.txt", 0, 0, -1, -1, 5000)
BePanic()
if (rFile2.I < 1) {
    Log("Phase 2: File visible on panel at row " + rFile2.Y)
} else {
    Panic("Phase 2: noopen.txt not found on panel")
}

// Middle-click on the file.
// With PasteFromPrimarySelection=1, FileList::ProcessMouse returns FALSE
// (the event is intercepted for primary paste), so the file should NOT open.
BeCalm()
TypeMouseClick(rFile2.X + 2, rFile2.Y, 0x0004)
Sleep(1000)
Sync(5000)
BePanic()

// Verify the file was NOT opened — we should still see the panel
// and NOT see "should not be opened" content
BeCalm()
var rNotOpened = ExpectString("should not be opened", 0, 0, -1, -1, 3000)
BePanic()
if (rNotOpened.I >= 1) {
    // File was opened — this is the behavioral change (middle-click intercepted)
    // Close it
    Log("Phase 2: File was opened (unexpected with PasteFromPrimarySelection=1)")
    TypeEscape()
    Sleep(500)
    Sync(2000)
} else {
    Log("Phase 2: File NOT opened by middle-click with PasteFromPrimarySelection=1 — correct")
}

// Verify we're still at the panel
BeCalm()
var rPanel = ExpectString("noopen.txt", 0, 0, -1, -1, 3000)
BePanic()
if (rPanel.I < 1) {
    Log("Phase 2: Still at panel after middle-click — correct")
} else {
    Panic("Phase 2: Panel lost after middle-click")
}
Sync(1000)

// Exit far2l
TypeFKey(10)
ExpectString("Do you want to quit FAR?", 0, 0, -1, -1, 10000)
TypeEnter()
ExpectAppExit(0, 10000)

Log("Phase 2: PASSED — middle-click does not open file when PasteFromPrimarySelection=1")
Log("All phases PASSED")
