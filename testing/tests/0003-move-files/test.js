LoadJS("../common.js");
var dirs = SetupTestDirs();

var left_sub1 = dirs.left + "/sub1";
var left_sub2 = dirs.left + "/sub2";
MkdirsAll([left_sub1, left_sub2], 0o700);

var left_files = [dirs.left + "/file1", dirs.left + "/file2", dirs.left + "/file3"];
var left_sub_files = [dirs.left + "/sub1/aaa", dirs.left + "/sub1/bbb", dirs.left + "/sub1/ccc", dirs.left + "/sub2/ddd"];
Mkfiles(left_files, 0o666, 0, 1024);
Mkfiles(left_sub_files, 0o752, 10 * 1024 * 1024, 20 * 1024 * 1024);

var left_items = [dirs.left + "/file1", dirs.left + "/file2", dirs.left + "/file3", dirs.left + "/sub1", dirs.left + "/sub2"];
var right_items = [dirs.right + "/file1", dirs.right + "/file2", dirs.right + "/file3", dirs.right + "/sub1", dirs.right + "/sub2"];
var left_hash = HashPathes(left_items, true, true, true, true, true);

StartTestApp(dirs.profile, dirs.left, dirs.right);
var status = AppStatus();
DismissHelpAndOSC52();

// Select all 5 items in left panel and press F6 to move
TypeDown()
TypeIns()
TypeIns()
TypeIns()
TypeIns()
TypeIns()
TypeFKey(6)
ExpectString("════ Rename/Move ═════", 0, 0, -1, -1, 10000)
TypeEnter()

// Wait for move to complete
for (var i = 0; ; ++i) {
	Sleep(100)
	ExpectNoString("════ Rename/Move ═════", 0, 0, -1, -1, 10000)
	var right_hash = HashPathes(right_items, true, true, true, true, true)
	if (right_hash == left_hash) {
		break
	}
	if (i == 100) {
		Log("Lhash: " + left_hash)
		Log("Rhash: " + right_hash)
		Panic("Hashes mismatched")
	}
}

// Verify source files were moved (no longer exist)
if (CountExisting(left_items) != 0) {
	Panic("Some source files still existing!")
}

ExitFar2lWithConfirm()


///////////////////
///////////////////
// CORNER CASES
///////////////////
///////////////////
var mydir = WorkDir();

///////////////////
// Corner case: Move empty file (0 bytes)
// Verifies move works when source file has no content
var dirsEF_profile = mydir + "/profile-emptyfile";
var dirsEF_left = mydir + "/left-ef";
var dirsEF_right = mydir + "/right-ef";
MkdirsAll([dirsEF_profile, dirsEF_left, dirsEF_right], 0o700);
Mkfile(dirsEF_left + "/empty.txt", 0o666, 0, 0);

StartTestApp(dirsEF_profile, dirsEF_left, dirsEF_right);
DismissHelpAndOSC52();

// Select empty.txt and move
TypeDown()
TypeIns()
TypeFKey(6)
ExpectString("════ Rename/Move ═════", 0, 0, -1, -1, 10000)
TypeEnter()
Sleep(500);

// Verify moved: source gone, dest exists
if (Exists(dirsEF_left + "/empty.txt")) {
    Panic("Empty file move: source still exists")
}
if (!Exists(dirsEF_right + "/empty.txt")) {
    Panic("Empty file move: dest not created")
}
ExitFar2lWithConfirm()


///////////////////
// Corner case: Move single file
// Verifies minimal selection (1 file) move works
var dirsSF_profile = mydir + "/profile-singlefile";
var dirsSF_left = mydir + "/left-sf";
var dirsSF_right = mydir + "/right-sf";
MkdirsAll([dirsSF_profile, dirsSF_left, dirsSF_right], 0o700);
var sf_left_items = [dirsSF_left + "/only.txt"];
var sf_right_items = [dirsSF_right + "/only.txt"];
Mkfiles(sf_left_items, 0o666, 100, 1000);
var sf_src = HashPathes(sf_left_items, true, true, true, true, true);

StartTestApp(dirsSF_profile, dirsSF_left, dirsSF_right);
DismissHelpAndOSC52();

// Select only.txt and move
TypeDown()
TypeIns()
TypeFKey(6)
ExpectString("════ Rename/Move ═════", 0, 0, -1, -1, 10000)
TypeEnter()
Sleep(500);

// Verify
if (CountExisting(sf_left_items) != 0) {
    Panic("Single file move: source still exists")
}
var sf_dst = HashPathes(sf_right_items, true, true, true, true, true);
if (sf_src != sf_dst) {
    Panic("Single file move: hashes mismatched")
}
ExitFar2lWithConfirm()


///////////////////
// Corner case: Move file with spaces in name
// Verifies path handling when filename contains whitespace
var dirsSP_profile = mydir + "/profile-spaces";
var dirsSP_left = mydir + "/left-sp";
var dirsSP_right = mydir + "/right-sp";
MkdirsAll([dirsSP_profile, dirsSP_left, dirsSP_right], 0o700);
var sp_left_items = [dirsSP_left + "/file with spaces.txt"];
var sp_right_items = [dirsSP_right + "/file with spaces.txt"];
Mkfiles(sp_left_items, 0o666, 100, 1000);
var sp_src = HashPathes(sp_left_items, true, true, true, true, true);

StartTestApp(dirsSP_profile, dirsSP_left, dirsSP_right);
DismissHelpAndOSC52();

// Select "file with spaces.txt" and move
TypeDown()
TypeIns()
TypeFKey(6)
ExpectString("════ Rename/Move ═════", 0, 0, -1, -1, 10000)
TypeEnter()
Sleep(500);

// Verify
if (CountExisting(sp_left_items) != 0) {
    Panic("Spaces file move: source still exists")
}
var sp_dst = HashPathes(sp_right_items, true, true, true, true, true);
if (sp_src != sp_dst) {
    Panic("Spaces file move: hashes mismatched")
}
ExitFar2lWithConfirm()


///////////////////
// Corner case: Move file with Unicode name
// Verifies encoding boundary for non-ASCII filenames
var dirsUC_profile = mydir + "/profile-unicode";
var dirsUC_left = mydir + "/left-uc";
var dirsUC_right = mydir + "/right-uc";
MkdirsAll([dirsUC_profile, dirsUC_left, dirsUC_right], 0o700);
var uc_left_items = [dirsUC_left + "/тест-файл.txt"];
var uc_right_items = [dirsUC_right + "/тест-файл.txt"];
Mkfiles(uc_left_items, 0o666, 100, 1000);
var uc_src = HashPathes(uc_left_items, true, true, true, true, true);

StartTestApp(dirsUC_profile, dirsUC_left, dirsUC_right);
DismissHelpAndOSC52();

// Select "тест-файл.txt" and move
TypeDown()
TypeIns()
TypeFKey(6)
ExpectString("════ Rename/Move ═════", 0, 0, -1, -1, 10000)
TypeEnter()
Sleep(500);

// Verify
if (CountExisting(uc_left_items) != 0) {
    Panic("Unicode file move: source still exists")
}
var uc_dst = HashPathes(uc_right_items, true, true, true, true, true);
if (uc_src != uc_dst) {
    Panic("Unicode file move: hashes mismatched")
}
ExitFar2lWithConfirm()
