LoadJS("../common.js");
var dirs = SetupTestDirs();

StartTestApp(dirs.profile, dirs.left, dirs.right);
var status = AppStatus();
DismissHelpAndOSC52();

// Cycle panel focus to ensure the file panel is active.
// Always runs — needed when OSC52 focus left focus in the wrong panel,
// and harmless (returns to file panel) when OSC52 was absent.
EnsurePanelFocus();

// Navigate to viewme.txt and open it with F3
TypeDown()
TypeFKey(3)
ExpectString("left/viewme.txt", 0, 0, -1, -1, 10000)
Sync(10000)

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

TypeEscape()

ExitFar2lWithConfirm()


///////////////////
///////////////////
// CORNER CASES
///////////////////
///////////////////
var mydir = WorkDir();

///////////////////
// Corner case: View empty file
// Verifies viewer handles 0-byte file without crash
var dirsEF_profile = mydir + "/profile-emptyfile";
var dirsEF_left = mydir + "/left-ef";
var dirsEF_right = mydir + "/right-ef";
MkdirsAll([dirsEF_profile, dirsEF_left, dirsEF_right], 0o700);
Mkfile(dirsEF_left + "/empty.txt", 0o666, 0, 0);

StartTestApp(dirsEF_profile, dirsEF_left, dirsEF_right);
DismissHelpAndOSC52();
EnsurePanelFocus();

// Navigate to empty.txt and open with F3
TypeDown()
TypeFKey(3)
ExpectString("empty.txt", 0, 0, -1, -1, 10000)
Sync(5000);
TypeEscape()
ExitFar2lWithConfirm()


///////////////////
// Corner case: View single-line file
// Verifies viewer handles minimal content
var dirsSL_profile = mydir + "/profile-singleline";
var dirsSL_left = mydir + "/left-sl";
var dirsSL_right = mydir + "/right-sl";
MkdirsAll([dirsSL_profile, dirsSL_left, dirsSL_right], 0o700);
WriteFile(dirsSL_left + "/oneline.txt", "012345\n", 0o666);

StartTestApp(dirsSL_profile, dirsSL_left, dirsSL_right);
DismissHelpAndOSC52();
EnsurePanelFocus();

TypeDown()
TypeFKey(3)
ExpectString("oneline.txt", 0, 0, -1, -1, 10000)
Sync(5000);
// Verify content is visible (viewer pads short lines with spaces)
ExpectString("012345", 0, 0, -1, -1, 5000)
TypeEscape()
ExitFar2lWithConfirm()


///////////////////
// Corner case: View file with line wider than terminal (120 cols)
// Verifies viewer handles horizontal overflow gracefully
var dirsWL_profile = mydir + "/profile-wideline";
var dirsWL_left = mydir + "/left-wl";
var dirsWL_right = mydir + "/right-wl";
MkdirsAll([dirsWL_profile, dirsWL_left, dirsWL_right], 0o700);
// Generate a line of 200 'A' characters + newline
var longline = "";
for (var i = 0; i < 200; i++) longline += "A";
WriteFile(dirsWL_left + "/wideline.txt", longline + "\n", 0o666);

StartTestApp(dirsWL_profile, dirsWL_left, dirsWL_right);
DismissHelpAndOSC52();
EnsurePanelFocus();

TypeDown()
TypeFKey(3)
ExpectString("wideline.txt", 0, 0, -1, -1, 10000)
Sync(5000);
// Viewer should show truncated or wrapped content without crash
TypeEscape()
ExitFar2lWithConfirm()


///////////////////
// Corner case: View file with Unicode content
// Verifies non-ASCII text renders correctly in viewer
var dirsUC_profile = mydir + "/profile-unicode";
var dirsUC_left = mydir + "/left-uc";
var dirsUC_right = mydir + "/right-uc";
MkdirsAll([dirsUC_profile, dirsUC_left, dirsUC_right], 0o700);
WriteFile(dirsUC_left + "/unicode.txt", "Кириллица\n日本語テスト\nМикс Latin и Кириллица\n", 0o666);

StartTestApp(dirsUC_profile, dirsUC_left, dirsUC_right);
DismissHelpAndOSC52();
EnsurePanelFocus();

TypeDown()
TypeFKey(3)
ExpectString("unicode.txt", 0, 0, -1, -1, 10000)
Sync(5000);
// Verify Unicode content is visible in the viewer
ExpectString("Кириллица", 0, 0, -1, -1, 5000)
ExpectString("日本語テスト", 0, 0, -1, -1, 5000)
TypeEscape()
ExitFar2lWithConfirm()


///////////////////
// Corner case: Search for nonexistent string
// Verifies F7 search handles no-match gracefully
var dirsSR_profile = mydir + "/profile-search";
var dirsSR_left = mydir + "/left-sr";
var dirsSR_right = mydir + "/right-sr";
MkdirsAll([dirsSR_profile, dirsSR_left, dirsSR_right], 0o700);
WriteFile(dirsSR_left + "/searchme.txt", "line one\nline two\nline three\n", 0o666);

StartTestApp(dirsSR_profile, dirsSR_left, dirsSR_right);
DismissHelpAndOSC52();
EnsurePanelFocus();

TypeDown()
TypeFKey(3)
ExpectString("searchme.txt", 0, 0, -1, -1, 10000)
Sync(5000);

// Search for a string that doesn't exist
TypeFKey(7)
ExpectString("═══ Search ═══", 0, 0, -1, -1, 10000)
TypeText("ZZZ_NOT_FOUND")
TypeEnter()
Sync(5000);
// No crash — viewer should handle no-match gracefully
TypeEscape()
Sleep(500)
TypeEscape()
ExitFar2lWithConfirm()
