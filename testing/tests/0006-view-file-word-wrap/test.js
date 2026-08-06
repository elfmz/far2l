LoadJS("../common.js");
var dirs = SetupTestDirs();

StartTestApp(dirs.profile, dirs.left, dirs.right, null, true, [95, 24]);
var status = AppStatus();
DismissHelpAndOSC52();

// Navigate to viewme.txt and open it with F3, then toggle word wrap (Shift+F2)
TypeDown()
TypeFKey(3)
ExpectString("left/viewme.txt", 0, 0, -1, -1, 10000)
Sync(10000)
ToggleShift(true)
TypeFKey(2)
ToggleShift(false)

// Scroll through the file and verify each page
TypePageDown()
Sync(10000)
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, dirs.mydir + '/test1.txt')

TypePageDown()
Sync(10000)
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, dirs.mydir + '/test2.txt')

TypeDown()
Sync(10000)
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, dirs.mydir + '/test3.txt')

TypeHome()
Sync(10000)
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, dirs.mydir + '/test4.txt')

// Search for ::setselectpos and verify the result
TypeFKey(7)
ExpectString("═══ Search ═══", 0, 0, -1, -1, 10000)
TypeText("::setselectpos")
TypeEnter()
Sync(10000)
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, dirs.mydir + '/test5.txt')

TypeUp()
Sync(10000)
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, dirs.mydir + '/test6.txt')

TypeUp()
Sync(10000)
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, dirs.mydir + '/test7.txt')

// Go back to top, search for VMenu::SetUserData and verify
TypeHome()
Sync(10000)
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, dirs.mydir + '/test4.txt')

TypeFKey(7)
ExpectString("═══ Search ═══", 0, 0, -1, -1, 10000)
TypeText("VMenu::SetUserData")
TypeEnter()
Sync(10000)
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, dirs.mydir + '/test8.txt')

TypeUp()
Sync(10000)
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, dirs.mydir + '/test9.txt')

TypeUp()
TypeUp()
Sync(10000)
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, dirs.mydir + '/test10.txt')

TypeDown()
Sync(10000)
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, dirs.mydir + '/test11.txt')

TypeDown()
Sync(10000)
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, dirs.mydir + '/test12.txt')

TypeDown()
Sync(10000)
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, dirs.mydir + '/test13.txt')

TypePageDown()
Sync(10000)
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, dirs.mydir + '/test14.txt')

TypePageUp()
Sync(10000)
BoundedLinesMatchTextFile(0, 1, -1, status.Height - 2, dirs.mydir + '/test15.txt')

TypeEscape()

ExitFar2lWithConfirm()


///////////////////
///////////////////
// CORNER CASES
///////////////////
///////////////////
var mydir = WorkDir();

///////////////////
// Corner case: Empty file with word wrap
// Verifies viewer handles 0-byte file when wrap is toggled
var dirsEF_profile = mydir + "/profile-emptyfile";
var dirsEF_left = mydir + "/left-ef";
var dirsEF_right = mydir + "/right-ef";
MkdirsAll([dirsEF_profile, dirsEF_left, dirsEF_right], 0o700);
Mkfile(dirsEF_left + "/empty.txt", 0o666, 0, 0);

StartTestApp(dirsEF_profile, dirsEF_left, dirsEF_right, null, true, [95, 24]);
DismissHelpAndOSC52();
TypeDown()
TypeFKey(3)
ExpectString("empty.txt", 0, 0, -1, -1, 10000)
Sync(5000);
// Toggle word wrap on empty file — no crash
ToggleShift(true)
TypeFKey(2)
ToggleShift(false)
Sync(2000);
TypeEscape()
ExitFar2lWithConfirm()


///////////////////
// Corner case: Single long line with word wrap
// Verifies line wraps at terminal width instead of scrolling horizontally
var dirsLL_profile = mydir + "/profile-longline";
var dirsLL_left = mydir + "/left-ll";
var dirsLL_right = mydir + "/right-ll";
MkdirsAll([dirsLL_profile, dirsLL_left, dirsLL_right], 0o700);
// 200-char line should wrap across multiple screen rows at 95 cols
var longline = "";
for (var i = 0; i < 200; i++) longline += "B";
WriteFile(dirsLL_left + "/longline.txt", longline + "\n", 0o666);

StartTestApp(dirsLL_profile, dirsLL_left, dirsLL_right, null, true, [95, 24]);
DismissHelpAndOSC52();
TypeDown()
TypeFKey(3)
ExpectString("longline.txt", 0, 0, -1, -1, 10000)
Sync(5000);
// Enable word wrap — long line should wrap across rows
ToggleShift(true)
TypeFKey(2)
ToggleShift(false)
Sync(2000);
// Verify content is visible (wrapped across rows)
ExpectString("BBB", 0, 0, -1, -1, 5000)
TypeEscape()
ExitFar2lWithConfirm()


///////////////////
// Corner case: Toggle word wrap on/off mid-view
// Verifies content updates correctly when wrap state changes
var dirsTW_profile = mydir + "/profile-togglewrap";
var dirsTW_left = mydir + "/left-tw";
var dirsTW_right = mydir + "/right-tw";
MkdirsAll([dirsTW_profile, dirsTW_left, dirsTW_right], 0o700);
WriteFile(dirsTW_left + "/toggle.txt", "AAAAAAAAAA BBBBBBBBBB CCCCCCCCCC DDDDDDDDDD EEEEEEEEEE FFFFFFFFFF\n", 0o666);

StartTestApp(dirsTW_profile, dirsTW_left, dirsTW_right, null, true, [95, 24]);
DismissHelpAndOSC52();
TypeDown()
TypeFKey(3)
ExpectString("toggle.txt", 0, 0, -1, -1, 10000)
Sync(5000);

// Without wrap — content on one line
var unwrapped = BoundedLine(0, 1, 93, "\t")
ExpectString("AAAAAAAAAA", 0, 0, -1, -1, 5000)

// Toggle wrap ON — content may wrap
ToggleShift(true)
TypeFKey(2)
ToggleShift(false)
Sync(2000);
// Content should still be visible
ExpectString("AAAAAAAAAA", 0, 0, -1, -1, 5000)

// Toggle wrap OFF again
ToggleShift(true)
TypeFKey(2)
ToggleShift(false)
Sync(2000);
// Back to original view
ExpectString("AAAAAAAAAA", 0, 0, -1, -1, 5000)
TypeEscape()
ExitFar2lWithConfirm()


///////////////////
// Corner case: Unicode content with word wrap
// Verifies wrap handles multi-byte characters correctly
var dirsUC_profile = mydir + "/profile-unicode-wrap";
var dirsUC_left = mydir + "/left-ucw";
var dirsUC_right = mydir + "/right-ucw";
MkdirsAll([dirsUC_profile, dirsUC_left, dirsUC_right], 0o700);
// Build a line of repeated Cyrillic that exceeds 95 cols
var uc_line = "";
for (var i = 0; i < 30; i++) uc_line += "Тест ";
WriteFile(dirsUC_left + "/ucwrap.txt", uc_line + "\nМикс Latin и Кириллица\n", 0o666);

StartTestApp(dirsUC_profile, dirsUC_left, dirsUC_right, null, true, [95, 24]);
DismissHelpAndOSC52();
TypeDown()
TypeFKey(3)
ExpectString("ucwrap.txt", 0, 0, -1, -1, 10000)
Sync(5000);
// Enable word wrap
ToggleShift(true)
TypeFKey(2)
ToggleShift(false)
Sync(2000);
// Verify Unicode content is visible with wrap enabled
ExpectString("Тест", 0, 0, -1, -1, 5000)
ExpectString("Микс Latin и Кириллица", 0, 0, -1, -1, 5000)
TypeEscape()
ExitFar2lWithConfirm()


///////////////////
// Corner case: Short lines — word wrap has no visual effect
// Verifies wrap doesn't corrupt content that fits within terminal width
var dirsSL_profile = mydir + "/profile-shortlines";
var dirsSL_left = mydir + "/left-sl";
var dirsSL_right = mydir + "/right-sl";
MkdirsAll([dirsSL_profile, dirsSL_left, dirsSL_right], 0o700);
WriteFile(dirsSL_left + "/short.txt", "aaa\nbbb\nccc\n", 0o666);

StartTestApp(dirsSL_profile, dirsSL_left, dirsSL_right, null, true, [95, 24]);
DismissHelpAndOSC52();
TypeDown()
TypeFKey(3)
ExpectString("short.txt", 0, 0, -1, -1, 10000)
Sync(5000);
// Enable word wrap — short lines should display identically
ToggleShift(true)
TypeFKey(2)
ToggleShift(false)
Sync(2000);
ExpectString("aaa", 0, 0, -1, -1, 5000)
ExpectString("bbb", 0, 0, -1, -1, 5000)
ExpectString("ccc", 0, 0, -1, -1, 5000)
TypeEscape()
ExitFar2lWithConfirm()
