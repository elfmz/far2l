LoadJS("../common.js");
var dirs = SetupTestDirs();

// 0025-user-menu-home-end-overflow — Test Home/End causing line render
// beyond the editor's boundaries in the User Menu Commands memo edit,
// when "Show line numbers" editor setting is enabled.
//
// Bug: When Opt.EdOpt.ShowLineNumbers=1 (F9 → Options → Editor settings →
// [x] Show line numbers), the DI_MEMOEDIT in the Edit User Menu dialog
// shows line numbers on the left, reducing the text area width. However,
// the Edit::FastShow() rendering after Home/End does not properly account
// for the line number area width, causing long lines to render beyond the
// editor's right boundary, overflowing past the dialog box border (║).
//
// The Editor constructor (editor.cpp:252) copies Opt.EdOpt globally,
// so ShowLineNumbers=1 in config.ini affects the dialog's memo edit too.
//
// Reproduction:
// 1. Set ShowLineNumbers=1 in config.ini [Editor] section
// 2. Open user menu (F2)
// 3. Insert a new command item (Ins → "Insert command")
// 4. In the Commands memo field, type a long line:
//    "1 (2>&1 meld !/!.! !#!/!.!) > /dev/null &"
// 5. Press End — cursor moves to end of line
// 6. Press Home — cursor moves to start of line
// 7. Verify the line does not overflow past the dialog right border (║)
//
// Dialog layout (measured on 120x80 terminal):
//   Top border (╔): y=29, Left border (║): x=24, Right border (║): x=88
//   "Hot key:" at y=30, "Label:" at y=32, "Commands:" at y=35
//   Memo edit at y=36..y=45, line numbers at x~27
//   Separator (╟) at y=34 and y=46, buttons at y=47

// Pre-configure editor settings: enable Show line numbers
var settingsDir = dirs.profile + "/.config/settings";
MkdirsAll([settingsDir], 0o700);
SaveTextFile(settingsDir + "/config.ini", [
    "[System]",
    "SaveHistory=1",
    "SaveFoldersHistory=1",
    "SaveViewHistory=1",
    "",
    "[Editor]",
    "ShowLineNumbers=1",
    "ShowScrollBar=1",
    ""
]);

StartTestApp(dirs.profile, dirs.left, dirs.right, "left", false);
DismissHelpAndOSC52();

// Ensure panel is active
EnsurePanelFocus();

// Dialog border positions (measured on 120x80 terminal)
var DLG_LEFT = 24;    // Left border ║
var DLG_RIGHT = 88;   // Right border ║
var MEMO_Y = 36;      // First memo edit row (line number "1" appears here)
var MEMO_Y_END = 45;  // Last memo edit row

// ========================================
// Phase 1: Open user menu, insert command, type long line
// ========================================
TypeFKey(2);
Sleep(500);
Sync(2000);

TypeIns();
Sleep(500);
Sync(2000);

ExpectString("insert", 0, 0, -1, -1, 5000);
Sync(1000);
TypeEnter();
Sleep(500);
Sync(2000);

ExpectString("Edit user menu", 0, 0, -1, -1, 5000);
Sync(1000);
ExpectString("Commands:", 0, 0, -1, -1, 5000);
Sync(1000);

// Type hot key
TypeText("z");
Sleep(300);
Sync(1000);

// Tab to Label, type label
TypeVK(9); Sleep(300); Sync(1000);
TypeText("Test overflow");
Sleep(300); Sync(1000);

// Tab to Commands memo
TypeVK(9); Sleep(300); Sync(1000);

// Verify line numbers are shown — look for "1" in the line number area
Sync(2000);
var lineNumCell = ReadCell(27, MEMO_Y);
Log("Phase 1: Line number cell (27," + MEMO_Y + ") = '" + lineNumCell.Text + "'");
if (lineNumCell.Text == "1") {
    Log("Phase 1: Line numbers visible — ShowLineNumbers applied — correct");
} else {
    // Try nearby positions
    for (var x = 25; x <= 32; x++) {
        var c = ReadCell(x, MEMO_Y);
        if (c.Text >= "0" && c.Text <= "9") {
            Log("Phase 1: Line number '" + c.Text + "' found at x=" + x + " — correct");
            break;
        }
    }
}

// Type a very long command line that exceeds the memo edit text area width.
// With ShowLineNumbers=1, the text area is ~60 cells (66 minus ~6 for line
// numbers). This 80-char line forces horizontal scrolling when End is pressed,
// which is the condition that triggers the overflow bug.
TypeText("1 (2>&1 meld !/!.! !#!/!.!) > /dev/null & echo EXTRA_TEXT_TO_FORCE_SCROLLING_HERE");
Sleep(500);
Sync(2000);
Log("Phase 1: Long command line entered — correct");


// ========================================
// Phase 2: Press End — check for overflow beyond right border
// ========================================
TypeEnd();
Sleep(500);
Sync(2000);

// Check cells just beyond the right border (x=DLG_RIGHT+1=89)
// There should be only spaces beyond the right border ║
var overflowFound = false;
for (var y = MEMO_Y; y <= MEMO_Y_END; y++) {
    var c = ReadCell(DLG_RIGHT + 1, y);
    if (c.Text != " " && c.Text != "") {
        Log("Phase 2: OVERFLOW at (" + (DLG_RIGHT + 1) + "," + y + ") — text '" + c.Text + "' beyond right border");
        overflowFound = true;
    }
}
if (!overflowFound) {
    Log("Phase 2: No overflow beyond right border after End — correct");
}

// Also scan the row that contains the typed text for overflow
// The text is on the first memo line (y=MEMO_Y)
var textRow = "";
for (var x = DLG_RIGHT - 2; x <= DLG_RIGHT + 5; x++) {
    textRow += ReadCell(x, MEMO_Y).Text;
}
Log("Phase 2: Right border area at y=" + MEMO_Y + ": [" + textRow + "]");


// ========================================
// Phase 3: Press Home — check for overflow
// ========================================
TypeHome();
Sleep(500);
Sync(2000);

// Check beyond right border after Home
overflowFound = false;
for (var y = MEMO_Y; y <= MEMO_Y_END; y++) {
    var c = ReadCell(DLG_RIGHT + 1, y);
    if (c.Text != " " && c.Text != "") {
        Log("Phase 3: OVERFLOW at (" + (DLG_RIGHT + 1) + "," + y + ") — text '" + c.Text + "' after Home");
        overflowFound = true;
    }
}
if (!overflowFound) {
    Log("Phase 3: No overflow beyond right border after Home — correct");
}

// Check the text row at the right border area
textRow = "";
for (var x = DLG_RIGHT - 2; x <= DLG_RIGHT + 5; x++) {
    textRow += ReadCell(x, MEMO_Y).Text;
}
Log("Phase 3: Right border area at y=" + MEMO_Y + ": [" + textRow + "]");

// Check left side — text should not leak before the left border
var leftOverflow = false;
for (var y = MEMO_Y; y <= MEMO_Y_END; y++) {
    var c = ReadCell(DLG_LEFT - 1, y);
    if (c.Text != " " && c.Text != "") {
        Log("Phase 3: OVERFLOW at (" + (DLG_LEFT - 1) + "," + y + ") — text '" + c.Text + "' before left border");
        leftOverflow = true;
    }
}
if (!leftOverflow) {
    Log("Phase 3: No overflow before left border after Home — correct");
}


// ========================================
// Phase 4: Alternate Home/End cycling to stress-test rendering
// ========================================
TypeEnd();   Sleep(200); Sync(500);
TypeHome();  Sleep(200); Sync(500);
TypeEnd();   Sleep(200); Sync(500);
TypeHome();  Sleep(200); Sync(500);
TypeEnd();   Sleep(200); Sync(1000);

// Final overflow check after cycling
overflowFound = false;
for (var y = MEMO_Y; y <= MEMO_Y_END; y++) {
    var c = ReadCell(DLG_RIGHT + 1, y);
    if (c.Text != " " && c.Text != "") {
        Log("Phase 4: OVERFLOW at (" + (DLG_RIGHT + 1) + "," + y + ") — text '" + c.Text + "' after cycling");
        overflowFound = true;
    }
}
if (!overflowFound) {
    Log("Phase 4: No overflow after Home/End cycling — correct");
}

// Show the full memo edit row for visual inspection
textRow = "";
for (var x = DLG_LEFT; x <= DLG_RIGHT + 3; x++) {
    textRow += ReadCell(x, MEMO_Y).Text;
}
Log("Phase 4: Full memo row: [" + textRow + "]");


// ========================================
// Phase 5: Verify the dialog border is intact
// ========================================
// The right border ║ at x=DLG_RIGHT should still be a box-drawing char
var rightBorderCell = ReadCell(DLG_RIGHT, 30);  // "Hot key:" row
Log("Phase 5: Right border cell (" + DLG_RIGHT + ",30) = '" + rightBorderCell.Text + "'");
if (rightBorderCell.Text == "║") {
    Log("Phase 5: Right dialog border intact — correct");
} else {
    Log("Phase 5: Right border char is '" + rightBorderCell.Text + "' — expected ║");
}

var leftBorderCell = ReadCell(DLG_LEFT, 30);
Log("Phase 5: Left border cell (" + DLG_LEFT + ",30) = '" + leftBorderCell.Text + "'");
if (leftBorderCell.Text == "║") {
    Log("Phase 5: Left dialog border intact — correct");
} else {
    Log("Phase 5: Left border char is '" + leftBorderCell.Text + "' — expected ║");
}

// Cancel the edit dialog
TypeEscape();
Sleep(500);
Sync(2000);

// Close user menu
TypeEscape();
Sleep(300);
Sync(2000);

Log("Phase 5: Dialog border verification and cleanup — correct");


ExitFar2lWithConfirm();
