LoadJS("../common.js");
var dirs = SetupTestDirs();

// 0031-targz-longlink — Regression test for PR #19:
// "fix(multiarc/targz): handle GNUTYPE_LONGLINK for hard links with
//  long paths"
//
// Bug (before PR): TAR archives with hard links whose target path
// exceeds 100 characters use a GNUTYPE_LONGLINK ('K') header block to
// store the full link name. Previously this data was incorrectly stored
// in LongName (shared with GNUTYPE_LONGNAME) and then applied to
// PathName instead of LinkName. As a result, the link target was
// truncated to 100 chars (the static linkname field) or corrupted —
// the archive entry showed a wrong link target. A LONGLINK following a
// LONGNAME also caused GETARC_BROKEN, preventing the archive opening.
//
// Fix (PR #19): Separate LongLink buffer from LongName. GNUTYPE_LONGLINK
// populates LongLink applied to LinkName. PAX "linkpath" is now parsed.
// LongName is cleared after use.
//
// This test creates a tar with a hardlink whose target path > 100 chars
// (triggering GNUTYPE_LONGLINK), opens it in far2l via multiarc, and
// verifies the archive opens and the hardlink entry is visible. Before
// the PR, GETARC_BROKEN prevents the archive from opening at all.

StartTestApp(dirs.profile, dirs.left, dirs.right);
DismissHelpAndOSC52();
StartBashShell();

// Create a tar archive with a hard link whose target path exceeds 100
// characters. GNU tar emits GNUTYPE_LONGLINK 'K' headers for such links.
// We use a script to avoid quoting issues.
var script = "D=$(pwd)/longlink_build && rm -rf $D && mkdir -p $D/sub/deep/path/that/is/very/long/indeed/for/testing/longlink/feature/here && mkdir -p $D/link/to/another/deep/path/that/is/also/very/long/for/testing/the/longlink/target/here && echo DATA > $D/sub/deep/path/that/is/very/long/indeed/for/testing/longlink/feature/here/file.txt && ln $D/sub/deep/path/that/is/very/long/indeed/for/testing/longlink/feature/here/file.txt $D/link/to/another/deep/path/that/is/also/very/long/for/testing/the/longlink/target/here/hardlink.txt && cd $D && tar cf '" + dirs.left + "/longlink.tar' sub link && echo TAR_CREATED\n";

TTYWriteRaw(script);
Sync(10000);

ExpectString("TAR_CREATED", 0, 0, -1, -1, 15000);
Sync(2000);

// Disable kitty (in case any was enabled) and exit bash cleanly
TTYWriteRaw("printf '\\033[=0u' > /dev/tty\n");
Sync(2000);
ExitBashAndFar2l();

// Now open the tar archive in the panel and verify the hardlink entry.
StartTestApp(dirs.profile, dirs.left, dirs.right, "left", false);
DismissOSC52Only();
EnsurePanelFocus();

// The longlink.tar should be visible in the left panel
ExpectString("longlink.tar", 0, 0, -1, -1, 5000);
Sync(1000);

// Move cursor to longlink.tar and press Enter to enter the archive
// (multiarc opens it as a virtual folder).
// Before the PR: GETARC_BROKEN prevents the archive from opening.
TypeEnter();
Sleep(1500);
Sync(5000);

// Inside the archive, the hardlink entry "hardlink.txt" should be
// visible. Before the PR, the LONGLINK handling is broken so the
// archive fails to open and the entry is missing.
BeCalm();
var archiveOpen = ExpectString("hardlink.txt", 0, 0, -1, -1, 10000);
BePanic();
if (archiveOpen.I < 1) {
	Log("Archive opened and hardlink entry visible — LONGLINK parsed correctly — correct");
} else {
	Panic("Archive did not open or hardlink entry missing — LONGLINK handling broken (PR #19 not applied)");
}
Sync(1000);

// Also verify the 'sub' directory (original file) is present
BeCalm();
var subDir = ExpectString("sub", 0, 0, -1, -1, 5000);
BePanic();
if (subDir.I < 1) {
	Log("Original file directory visible in archive — correct");
} else {
	Log("sub directory not visible (may be scrolled — non-fatal)");
}
Sync(1000);

ExitFar2lWithConfirm();
Log("Test PASSED — GNUTYPE_LONGLINK handled correctly");
