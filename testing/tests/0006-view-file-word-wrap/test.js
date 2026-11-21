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

function OpenWrapFixture(name, width) {
    var longProfile = mydir + "/profile-" + name;
    MkdirsAll([longProfile], 0700);
    StartAppWithSize(["--tty", "--nodetect", "--mortal", "-u", longProfile,
        "-v", mydir + "/" + name], width, 24);
    ExpectString(name);
}

function CheckWrappedScroll(name) {
    OpenWrapFixture(name, 111);
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
function SaveUtf16(name, utf16, be) {
    var bytes = [];
    for (var i = 0; i < utf16.length; ++i) {
        var unit = utf16.charCodeAt(i);
        bytes.push(be ? unit >> 8 : unit & 255, be ? unit & 255 : unit >> 8);
    }
    WriteFile(mydir + "/" + name, bytes, 0600);
}
var utf16 = "\ufeff" + lines.join("\r\n") + "\r\n";
for (var be = 0; be < 2; ++be) {
    var name = be ? "wrapped-utf16be.txt" : "wrapped-utf16le.txt";
    SaveUtf16(name, utf16, be);
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

function GoToByte(offset) {
    ToggleLAlt(true); TypeFKey(8); ToggleLAlt(false);
    ExpectString("Go to");
    TypeEnd(); ToggleShift(true); TypeHome(); ToggleShift(false);
    TypeText("0x" + offset.toString(16));
    TypeEnter();
}

// Each full unwrapped row holds 8,176 decoded characters. These rows use
// three bytes per character and start with distinct full-width markers.
var rowChars = 8192 - 16;
// The test runner's Goja version drops the first character in a Unicode repeat().
var restOfRow = new Array(rowChars).join("界");
var unwrapped = ["甲" + restOfRow + "乙" + restOfRow + "丙"];
if (unwrapped[0].charAt(rowChars) != "乙" || unwrapped[0].charAt(2 * rowChars) != "丙")
    Panic("UTF-8 fixture markers must start the second and third unwrapped rows");
for (var i = 0; i < 30; ++i)
    unwrapped.push("tail " + i);
SaveTextFile(mydir + "/unwrapped-utf8.txt", unwrapped);
OpenWrapFixture("unwrapped-utf8.txt", 111);
TypeFKey(2);
CheckCellChar(0, 1, "甲");
TypeDown();
CheckCellChar(0, 1, "乙");
TypeUp();
CheckCellChar(0, 1, "甲");
// Invalidate the measured layout, then seek inside the third row's first character.
TypeFKey(2); TypeFKey(2);
GoToByte(2 * rowChars * 3 + 1);
CheckCellChar(0, 1, "丙");
TypeUp();
CheckCellChar(0, 1, "乙");
TypeDown();
CheckCellChar(0, 1, "丙");
TypeEscape(); ExpectAppExit(0);

var interval = 256 * 1024;
var width = 111;
// The first mark falls three bytes after a semicolon, the second inside a
// combining accent. The next physical line starts nine bytes before mark three.
var first = "x".repeat(interval - 4) + ";CUT" + "t".repeat(interval - 2)
    + "e\u0301UTF8" + "z".repeat(interval - 15);
SaveTextFile(mydir + "/wrap-marks.txt", [first, "123456789REGULAR" + "q".repeat(4000)]);
OpenWrapFixture("wrap-marks.txt", width);
for (var mode = 0; mode < 2; ++mode) {
    if (mode == 1) {
        ToggleShift(true); TypeFKey(2); ToggleShift(false);
    }
    GoToByte(interval - 4);
    var lastRowLength = (interval - 3) % width;
    CheckBoundedLine("x".repeat(lastRowLength - 1) + ";", 0, 1, width, " ");
    CheckBoundedLine("CUT", 0, 2, 3, "");
    TypeDown();
    CheckBoundedLine("CUT", 0, 1, 3, "");
    TypeUp();
    CheckBoundedLine("CUT", 0, 2, 3, "");
    GoToByte(2 * interval);
    CheckBoundedLine("e\u0301UTF8", 0, 1, 5, "");
    GoToByte(3 * interval);
    CheckBoundedLine("123456789REGULAR", 0, 1, 16, "");
}
// The same continuation is a restart point for buffer-sized unwrapped rows.
TypeFKey(2);
GoToByte(2 * interval);
CheckBoundedLine("e\u0301UTF8", 0, 1, 5, "");
TypeUp();
TypeDown();
CheckBoundedLine("e\u0301UTF8", 0, 1, 5, "");
TypeEscape(); ExpectAppExit(0);

// The low surrogate is at the mark in both byte orders.
for (var be = 0; be < 2; ++be) {
    var name = be ? "mark-utf16be.txt" : "mark-utf16le.txt";
    SaveUtf16(name, "\ufeff" + "x".repeat(interval / 2 - 2) + "🙂PAIR" + "y".repeat(4000), be);
    OpenWrapFixture(name, width);
    GoToByte(interval);
    if (ReadCell(0, 1).Text != "🙂")
        Panic("Split surrogate pair at the continuation in " + name);
    TypeEscape(); ExpectAppExit(0);
}

// The mark falls on the semicolon inside SGR. Keep the escape with RED.
SaveTextFile(mydir + "/mark.ans", ["a".repeat(interval - 3) + red + "RED" + reset].concat(tail));
OpenWrapFixture("mark.ans", width);
ToggleShift(true); TypeFKey(2); ToggleShift(false);
GoToByte(interval);
CheckBoundedLine("RED", 0, 1, 3, "");
if (ReadCell(0, 1).Fore != 12)
    Panic("Lost the ANSI color at the continuation");
TypeEscape(); ExpectAppExit(0);

// Word wrap may skip spaces only as far as the next continuation.
SaveTextFile(mydir + "/mark-spaces.txt", ["a".repeat(width) + " ".repeat(interval + 17 - width) + "END"]);
OpenWrapFixture("mark-spaces.txt", width);
ToggleShift(true); TypeFKey(2); ToggleShift(false);
CheckBoundedLine(" ".repeat(17) + "END", 0, 2, 20, "");
TypeEscape(); ExpectAppExit(0);

// End lands 41 bytes past the second continuation, so its page spans that boundary.
var sparse = "wrap-sparse.txt";
WriteFile(mydir + "/" + sparse, [0xef, 0xbb, 0xbf, 0x47, 0x4f], 0600);
Truncate(mydir + "/" + sparse, 2 * interval + 41);
OpenWrapFixture(sparse, width);
TypeEnd();
ExpectString("100%", 0, 0, 0, 1);
ToggleShift(true); TypeFKey(2); ToggleShift(false);
TypeEnd();
ExpectString("100%", 0, 0, 0, 1);
TypeHome();
TypeFKey(2);
TypeEnd();
ExpectString("100%", 0, 0, 0, 1);
TypeHome();
ExpectString("GO", 0, 1, 8, 1);
TypeEscape(); ExpectAppExit(0);
0;
