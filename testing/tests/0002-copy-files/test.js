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

// Select all 5 items in left panel and press F5 to copy
TypeDown()
TypeIns()
TypeIns()
TypeIns()
TypeIns()
TypeIns()
TypeFKey(5)
ExpectString("════ Copy ═════", 0, 0, -1, -1, 10000)
TypeEnter()

// Wait for copy to complete
for (var i = 0; ; ++i) {
	Sleep(100)
	ExpectNoString("════ Copy ═════", 0, 0, -1, -1, 10000)
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

// Verify source files unchanged
var recent_left_hash = HashPathes(left_items, true, true, true, true, true)
if (left_hash != recent_left_hash) {
	Log("Lhash: " + left_hash + " -> " + recent_left_hash)
	Panic("Source files had changed!")
}

ExitFar2lWithConfirm()


///////////////////
///////////////////
// CORNER CASES
///////////////////
///////////////////
var mydir = WorkDir();

///////////////////
// Corner case: Copy empty file (0 bytes)
// Verifies copy works when source file has no content
var dirsEF_profile = mydir + "/profile-emptyfile";
var dirsEF_left = mydir + "/left-ef";
var dirsEF_right = mydir + "/right-ef";
MkdirsAll([dirsEF_profile, dirsEF_left, dirsEF_right], 0o700);
Mkfile(dirsEF_left + "/empty.txt", 0o666, 0, 0);

StartTestApp(dirsEF_profile, dirsEF_left, dirsEF_right);
DismissHelpAndOSC52();

// Select empty.txt and copy
TypeDown()
TypeIns()
TypeFKey(5)
ExpectString("════ Copy ═════", 0, 0, -1, -1, 10000)
TypeEnter()
Sleep(500);

// Verify empty file was copied (0-byte file should exist)
var left_hash_ef = HashPath(dirsEF_left + "/empty.txt", true, true, true, true, true);
var right_hash_ef = HashPath(dirsEF_right + "/empty.txt", true, true, true, true, true);
if (left_hash_ef != right_hash_ef) {
    Panic("Empty file copy: hashes mismatched")
}
ExitFar2lWithConfirm()


///////////////////
// Corner case: Copy single file
// Verifies minimal selection (1 file) copy works
var dirsSF_profile = mydir + "/profile-singlefile";
var dirsSF_left = mydir + "/left-sf";
var dirsSF_right = mydir + "/right-sf";
MkdirsAll([dirsSF_profile, dirsSF_left, dirsSF_right], 0o700);
Mkfiles([dirsSF_left + "/only.txt"], 0o666, 100, 1000);

StartTestApp(dirsSF_profile, dirsSF_left, dirsSF_right);
DismissHelpAndOSC52();

// Select only.txt and copy
TypeDown()
TypeIns()
TypeFKey(5)
ExpectString("════ Copy ═════", 0, 0, -1, -1, 10000)
TypeEnter()
Sleep(500);

// Verify
var left_hash_sf = HashPath(dirsSF_left + "/only.txt", true, true, true, true, true);
var right_hash_sf = HashPath(dirsSF_right + "/only.txt", true, true, true, true, true);
if (left_hash_sf != right_hash_sf) {
    Panic("Single file copy: hashes mismatched")
}
ExitFar2lWithConfirm()


///////////////////
// Corner case: Copy file with spaces in name
// Verifies path handling when filename contains whitespace
var dirsSP_profile = mydir + "/profile-spaces";
var dirsSP_left = mydir + "/left-sp";
var dirsSP_right = mydir + "/right-sp";
MkdirsAll([dirsSP_profile, dirsSP_left, dirsSP_right], 0o700);
Mkfiles([dirsSP_left + "/file with spaces.txt"], 0o666, 100, 1000);

StartTestApp(dirsSP_profile, dirsSP_left, dirsSP_right);
DismissHelpAndOSC52();

// Select "file with spaces.txt" and copy
TypeDown()
TypeIns()
TypeFKey(5)
ExpectString("════ Copy ═════", 0, 0, -1, -1, 10000)
TypeEnter()
Sleep(500);

// Verify
var left_hash_sp = HashPath(dirsSP_left + "/file with spaces.txt", true, true, true, true, true);
var right_hash_sp = HashPath(dirsSP_right + "/file with spaces.txt", true, true, true, true, true);
if (left_hash_sp != right_hash_sp) {
    Panic("Spaces file copy: hashes mismatched")
}
ExitFar2lWithConfirm()


///////////////////
// Corner case: Copy file with Unicode name
// Verifies encoding boundary for non-ASCII filenames
var dirsUC_profile = mydir + "/profile-unicode";
var dirsUC_left = mydir + "/left-uc";
var dirsUC_right = mydir + "/right-uc";
MkdirsAll([dirsUC_profile, dirsUC_left, dirsUC_right], 0o700);
Mkfiles([dirsUC_left + "/тест-файл.txt"], 0o666, 100, 1000);

StartTestApp(dirsUC_profile, dirsUC_left, dirsUC_right);
DismissHelpAndOSC52();

// Select "тест-файл.txt" and copy
TypeDown()
TypeIns()
TypeFKey(5)
ExpectString("════ Copy ═════", 0, 0, -1, -1, 10000)
TypeEnter()
Sleep(500);

// Verify
var left_hash_uc = HashPath(dirsUC_left + "/тест-файл.txt", true, true, true, true, true);
var right_hash_uc = HashPath(dirsUC_right + "/тест-файл.txt", true, true, true, true, true);
if (left_hash_uc != right_hash_uc) {
    Panic("Unicode file copy: hashes mismatched")
}
ExitFar2lWithConfirm()
