mydir=WorkDir()
profile=mydir + "/profile"
left=mydir + "/left"
right=mydir + "/right"

StartAppWithSize(["--tty", "--nodetect", "--mortal", "-u", profile, "-cd", left, "-cd", right], 95, 24);
ExpectString("Help - FAR2L");
TypeEscape()
ExpectString("OSC52");
status = AppStatus();

TypeEscape()
TypeDown()
TypeFKey(3)
ExpectString("left/viewme.txt")

ToggleShift(true)
TypeFKey(2)
ToggleShift(false)

TypePageDown()
BoundedLinesMatchTextFile(0, 1, 0, -2, mydir + '/test1.txt')

TypePageDown()
BoundedLinesMatchTextFile(0, 1, 0, -2, mydir + '/test2.txt')

TypeDown()
BoundedLinesMatchTextFile(0, 1, 0, -2, mydir + '/test3.txt')

TypeHome()
BoundedLinesMatchTextFile(0, 1, 0, -2, mydir + '/test4.txt')

TypeFKey(7)
ExpectString("═══ Search ═══")
TypeText("::setselectpos")
TypeEnter()

BoundedLinesMatchTextFile(0, 1, 0, -2, mydir + '/test5.txt')

TypeUp()
BoundedLinesMatchTextFile(0, 1, 0, -2, mydir + '/test6.txt')

TypeUp()
BoundedLinesMatchTextFile(0, 1, 0, -2, mydir + '/test7.txt')

TypeHome()
BoundedLinesMatchTextFile(0, 1, 0, -2, mydir + '/test4.txt')

TypeFKey(7)
ExpectString("═══ Search ═══")
TypeText("VMenu::SetUserData")
TypeEnter()

BoundedLinesMatchTextFile(0, 1, 0, -2, mydir + '/test8.txt')

TypeUp()
BoundedLinesMatchTextFile(0, 1, 0, -2, mydir + '/test9.txt')

TypeUp()
TypeUp()
BoundedLinesMatchTextFile(0, 1, 0, -2, mydir + '/test10.txt')

TypeDown()
BoundedLinesMatchTextFile(0, 1, 0, -2, mydir + '/test11.txt')

TypeDown()
BoundedLinesMatchTextFile(0, 1, 0, -2, mydir + '/test12.txt')

TypeDown()
BoundedLinesMatchTextFile(0, 1, 0, -2, mydir + '/test13.txt')

TypePageDown()
BoundedLinesMatchTextFile(0, 1, 0, -2, mydir + '/test14.txt')

TypePageUp()
BoundedLinesMatchTextFile(0, 1, 0, -2, mydir + '/test15.txt')

TypeEscape()

TypeFKey(10)
ExpectString("Do you want to quit FAR?")
TypeEnter()
ExpectAppExit(0)

// Distinct tokens expose a shift by even one whole repeated pattern. The last
// few short lines keep the physical line break visible when scrolling from End.
var asciiText = "", longText = "";
for (var i = 0; i < 960; ++i) {
    asciiText += "int<'x'>{z}\"z%s\"'';";
    longText += i + "\tint<'界'>{é}e\u0301,🙂; ";
}
var tail = ["4071 next physical line", "", "tail 1", "tail 2", "tail 3"];
var lines = [longText].concat(tail);

function CheckPage(page) {
    for (var row = 0; row < page.length; ++row)
        CheckBoundedLine(page[row], 0, row + 1, 0, "");
}

function CheckWrappedScroll(name) {
    var longProfile = mydir + "/profile-" + name;
    MkdirsAll([longProfile], 0700);
    StartAppWithSize(["--tty", "--nodetect", "--mortal", "-u", longProfile,
        "-v", mydir + "/" + name], 111, 24);
    ExpectString(name);

    for (var mode = 0; mode < 2; ++mode) {
        if (mode == 1) {
            ToggleShift(true);
            TypeFKey(2);
            ToggleShift(false);
        }

        TypeHome();
        TypePageDown(5);
        var forward = BoundedLines(0, 1, 0, -2, "");
        TypeDown(3);
        TypeUp(3);
        CheckPage(forward);

        TypeEnd();
        ExpectString("4071 next physical line");
        var endPage = BoundedLines(0, 1, 0, -2, "");
        TypeUp();
        for (var row = 0; row + 1 < endPage.length; ++row)
            CheckBoundedLine(endPage[row], 0, row + 2, 0, "");
        TypeDown();
        CheckPage(endPage);

        // Toggle the key bar twice to force a full redraw at the same width.
        TypeUp(2);
        var scrolled = BoundedLines(0, 1, 0, -2, "");
        ToggleLCtrl(true);
        TypeText("bb");
        ToggleLCtrl(false);
        CheckPage(scrolled);
    }
    TypeEscape();
    ExpectAppExit(0);
}

SaveTextFile(mydir + "/wrapped-ascii.cpp", [asciiText].concat(tail));
CheckWrappedScroll("wrapped-ascii.cpp");
SaveTextFile(mydir + "/wrapped-utf8.txt", lines);
CheckWrappedScroll("wrapped-utf8.txt");

// With a BOM the viewer selects the file encoding even in a fresh profile.
var utf16 = "\ufeff" + lines.join("\r\n") + "\r\n";
for (var be = 0; be < 2; ++be) {
    var bytes = [];
    for (var i = 0; i < utf16.length; ++i) {
        var unit = utf16.charCodeAt(i);
        bytes.push(be ? unit >> 8 : unit & 255, be ? unit & 255 : unit >> 8);
    }
    var name = be ? "wrapped-utf16be.txt" : "wrapped-utf16le.txt";
    WriteFile(mydir + "/" + name, bytes, 0600);
    CheckWrappedScroll(name);
}

// A literal semicolon permits a word break. The semicolon inside bold red SGR
// must be skipped, including when the row contains no other delimiter.
var ansiProfile = mydir + "/profile-ansi";
var ansiFile = mydir + "/word-wrap.ans";
var prefix = "A".repeat(70) + ";";
var unbroken = "B".repeat(79);
var red = "\x1b[1;31m", reset = "\x1b[0m";
var ansiLines = [prefix + "xy" + red + "ABCDEFGHIJKL" + reset + "!",
    unbroken + red + "XYZ" + reset + "!"];
for (var i = 0; i < 24; ++i)
    ansiLines.push("tail " + i);
SaveTextFile(ansiFile, ansiLines);
MkdirsAll([ansiProfile], 0700);
StartAppWithSize(["--tty", "--nodetect", "--mortal", "-u", ansiProfile, "-v", ansiFile], 80, 24);
ExpectString("word-wrap.ans");
ToggleShift(true);
TypeFKey(2);
ToggleShift(false);

function CheckAnsiWrap() {
    CheckBoundedLine(prefix, 0, 1, 0, " ");
    CheckBoundedLine("xyABCDEFGHIJKL!", 0, 2, 0, " ");
    CheckBoundedLine(unbroken + "X", 0, 3, 0, " ");
    CheckBoundedLine("YZ!", 0, 4, 0, " ");
    if (ReadCell(2, 2).Fore != 12 || ReadCell(0, 4).Fore != 12 || ReadCell(2, 4).Fore != 7)
        Panic("Expected bold red letters and a reset to white at '!'");
}
CheckAnsiWrap();
TypeDown();
TypeUp();
CheckAnsiWrap();
TypeEscape();
ExpectAppExit(0);
0;
