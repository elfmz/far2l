mydir=WorkDir()
profile=mydir + "/profile"
left=mydir + "/left"
left_sub1=mydir + "/left/sub1"
left_sub2=mydir + "/left/sub2"
right=mydir + "/right"
MkdirsAll([profile, left, left_sub1, left_sub2, right], 0700)

left_files = [left + "/file1", left + "/file2", left + "/file3"]
left_sub_files = [left + "/sub1/aaa", left + "/sub1/bbb", left + "/sub1/ccc", left + "/sub2/ddd"]
Mkfiles(left_files, 0666, 0, 1024)
Mkfiles(left_sub_files, 0752, 10 * 1024 * 1024, 20 * 1024 * 1024)

left_items = [left + "/file1", left + "/file2", left + "/file3", left + "/sub1", left + "/sub2"]
right_items = [right + "/file1", right + "/file2", right + "/file3", right + "/sub1", right + "/sub2"]
left_hash = HashPathes(left_items, true, true, true, true, true)

StartApp(["--tty", "--nodetect", "--mortal", "-u", profile, "-cd", left, "-cd", right]);
ExpectString("Help - FAR2L", 0, 0, -1, -1, 10000);

status = AppStatus();

TypeEscape(10)
TypeDown()
TypeIns()
TypeIns()
TypeIns()
TypeIns()
TypeIns()
TypeFKey(5)
ExpectString("════ Copy ═════", 0, 0, -1, -1, 10000)
TypeEnter()
for (i = 0; ; ++i) {
	Sleep(100)
	ExpectNoString("════ Copy ═════", 0, 0, -1, -1, 10000)
	right_hash = HashPathes(right_items, true, true, true, true, true)
	if (right_hash == left_hash) {
		break
	}
	if (i == 100) {
		Log("Lhash: " + left_hash)
		Log("Rhash: " + right_hash)
		Panic("Hashes mismatched")
	}
}

recent_left_hash = HashPathes(right_items, true, true, true, true, true)
if (left_hash != recent_left_hash) {
	Log("Lhash: " + left_hash + " -> " + recent_left_hash)
	Panic("Source files had changed!")
}

TypeFKey(10)
ExpectString("Do you want to quit FAR?", 0, 0, -1, -1, 10000)
TypeEnter()
ExpectAppExit(0, 10000)
0;
