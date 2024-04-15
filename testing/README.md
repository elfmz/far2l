To run tests run far2l-smoke-run.sh specifying path to far2l which was built by cmake configured with -DTESTING=Yes  
Example: `./far2l-smoke-run.sh ../../far2l.build/install/far2l`  
Note: if provided far2l built without testing support this will just stuck.. for now.  
Actual tests written in JS and located under test directory. They can use following predefined functions to perform actions:

---------------------------------------------------------

`StartApp(["arg1", "arg2" ...])`  
Starts far2l with given arguments, note that some arguments are implicitly inserted - path to far2l as very first argument and --test=  
Returns status of started far2l as structure of following fields:
 * Title string      - application title
 * Width uint32      - application TUI columns
 * Height uint32     - application TUI lines
 * CurX uint32       - X cursor position (column index)
 * CurY uint32       - Y cursor position (line index)
 * CurH uint8        - cursor height from 0 to 100
 * CurV bool         - true if cursor is visible

---------------------------------------------------------

`AppStatus()`  
Returns actual status of far2l as structure described above.

---------------------------------------------------------

`ReadCellRaw(x, y)`  
Reads screen cell at specified coordinates.  
Returns structure which has following fields:
 * Text string           - string representing contained character
 * Attributes uint64     - value from corresponding CHAR_INFO::Attributes

---------------------------------------------------------

`ReadCell(x, y)`  
Reads screen cell at specified coordinates.  
Returns more 'decomposed' (comparing to Raw version) structure wich has following fields:
 * Text string           - string representing contained character
 * BackTC uint32         - 24 bit foreground color if its used
 * ForeTC uint32         - 24 bit background color if its used
 * Back uint8            - 4 bit base foreground color
 * Fore uint8            - 4 bit base background color
 * IsBackTC bool         - true if 24 bit foreground color is used
 * IsForeTC bool         - true if 24 bit background color is used
 * ForeBlue bool         - true if base foreground color has blue component
 * ForeGreen bool        - true if base foreground color has green component
 * ForeRed bool          - true if base foreground color has red component
 * ForeIntense bool      - true if base foreground color has intensified brightness
 * BackBlue bool         - true if base background color has blue component
 * BackGreen bool        - true if base background color has green component
 * BackRed bool          - true if base background color has red component
 * BackIntense bool      - true if base background color has intensified brightness
 * ReverseVideo bool     - true if foreground/background colors are swapped at rendering
 * Underscore bool       - true if character is underscored (not implemented yet)
 * Strikeout bool        - true if character is striked out (not implemented yet)

---------------------------------------------------------

`BoundedLines(left, top, width, height, " \t")`  
Returns array of lines bounded by specified rectangle.  
Optionally trims edges of each line from trim_chars characters if its not empty.

---------------------------------------------------------

`BoundedLine(left, top, width, " \t")`  
Returns single line bounded by specified rectangle on unity height.  
Optionally trims edges of line from trim_chars characters if its not empty.

---------------------------------------------------------

`CheckBoundedLineOrDie("expected string", left, top, width, " \t")`  
Check that single line at specified rectangle matches to specified value.  
Optionally trims edges of line from trim_chars characters if its not empty.  
If string doesnt match - aborts execution.

---------------------------------------------------------


`SurroundedLines(x, y, "║═│─", " \t")`  
Returns array of lines bounded by any of specified in boundary_chars characters.  
x, y represends coordinates of any cell inside of required area  
Optionally trims edges of each line from trim_chars characters if its not empty.

---------------------------------------------------------

`CheckCellChar(x, y, "abcdef...")`  
`CheckCellCharOrDie(x, y, "abcdef...")`  
Checks if cell under specified coordinates contains any of characters contained in specified string.  
Returns matched character. But if no character matched then:
 * CheckCellChar returns empty string
 * CheckCellCharOrDie aborts execution


---------------------------------------------------------

`ExpectStrings(["string 1", "string 2" ...], x, y, w, h, timeout_ms)`  
`ExpectStringsOrDie(["string 1", "string 2" ...], x, y, w, h, timeout_ms)`  
Waits given amount of milliseconds for any of given strings will appear in provided rectangular area.  
Returns result as structure of following fields, that defines index of found string and its coordinates:
 * I uint32
 * X uint32
 * Y uint32

In case no string found before timeout reached:
 * ExpectStrings returns all fields set to 0xffffffff (-1)
 * ExpectStringsOrDie aborts execution

---------------------------------------------------------

`ExpectString("string", x, y, w, h, timeout_ms)`  
`ExpectStringOrDie("string", x, y, w, h, timeout_ms)`  
Simplified version of ExpectStrings that waits for one string only.  
Returns same as ExpectStrings.

---------------------------------------------------------

`ExpectAppExit(code, timeout_ms)`  
`ExpectAppExitOrDie(code, timeout_ms)`  
Expects that far2l will exit with specified exit code withing given milliseconds of timeout.  
Returns empty string if everything happen as expected, otherwise:
 * ExpectAppExit returns string with problem description
 * ExpectAppExitOrDie aborts execution

---------------------------------------------------------

`LogInfo("string")`  
Writes given string to test output.

---------------------------------------------------------

`LogFatal("string")`  
Writes given string to test output and aborts tests.

---------------------------------------------------------

`TTYWrite("string")`  
Writes given string to stdin of pseudoterminal where tested far2l is running.

---------------------------------------------------------

`TTYCtrlC()`  
Generates Ctrl+C for pseudoterminal where tested far2l is running.

---------------------------------------------------------

`RunCmd(["prog", "arg1", "arg2" ...])`  
`RunCmdOrDie(["prog", "arg1", "arg2" ...])`  
Runs given command, returns empty string if start succeeded and command returned zero code, otherwise returns error description or aborts if its RunCmdOrDie.  
Note that command is run NOT in pseudoterminal where tested far2l is running.

---------------------------------------------------------

`Sleep(msec)`  
Pauses execution for specified amount of milliseconds

---------------------------------------------------------

`ToggleShift(pressed bool)`  
`ToggleLCtrl(pressed bool)`  
`ToggleRCtrl(pressed bool)`  
`ToggleLAlt(pressed bool)`  
`ToggleRAlt(pressed bool)`  
Simulate changing state of specific named control key. Changed state affects all following keypresses.

---------------------------------------------------------

`TypeBack()`  
`TypeEnter()`  
`TypeEscape()`  
`TypePageUp()`  
`TypePageDown()`  
`TypeEnd()`  
`TypeHome()`  
`TypeLeft()`  
`TypeUp()`  
`TypeRight()`  
`TypeDown()`  
`TypeIns()`  
`TypeDel()`  
Simulate typing of specific named key

---------------------------------------------------------

`TypeAdd()`  
`TypeSub()`  
`TypeMul()`  
`TypeDiv()`  
`TypeSeparator()`  
`TypeDecimal()`  
Simulate typing of specific named NumPad key

---------------------------------------------------------

`TypeDigit(n)`  
Simulates typing of specified NumPad digit, where n=0 means 0, n=1 means 1 and so on

---------------------------------------------------------

`TypeFKey(n)`  
Simulates typing of specified F-key, where n=1 means F1, n=2 means F2 and so on

---------------------------------------------------------

`TypeText("someText to Type")`  
Simulates char-by-char typing of specified text.

---------------------------------------------------------

`WorkDir()`  
Returns path to tests working directory - where temporary files and logs are resided

---------------------------------------------------------

`Chmod(name string, mode FileMode) error`  
`Chown(name string, uid, gid int) error`  
`Chtimes(name string, atime time.Time, mtime time.Time) error`  
`Mkdir(name string, perm FileMode) error`  
`MkdirAll(path string, perm FileMode) error`  
`MkdirTemp(dir, pattern string) (string, error)`  
`Remove(name string) error`  
`RemoveAll(name string) error`  
`Rename(oldpath, newpath string) error`  
`ReadFile(name string) ([]byte, error)`  
`WriteFile(name string, data []byte, perm FileMode) error`  
`ReadDir(name string) ([]DirEntry, error)`  
`Truncate(name string, size int64) error`  
`Symlink(oldname, newname string) error`  
`Readlink(name string) (string, error)`  
This functions are directly exposed from go os so you can find their description right there: https://pkg.go.dev/os

`MkdirsAll(pathes []string, perm os.FileMode) error`  
This like MkdirAll but creates multiple directories trees at once
