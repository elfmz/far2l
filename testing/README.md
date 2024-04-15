To run tests run far2l-smoke-run.sh specifying path to far2l which was built by cmake configured with -DTESTING=Yes  
Example: `./far2l-smoke-run.sh ../../far2l.build/install/far2l`  
Note: if provided far2l built without testing support this will just stuck.. for now.  
Actual tests written in JS and located under test directory. They can use following predefined functions to perform actions:

---------------------------------------------------------

`BePanic`  
In case of logical problem in subsequent functions - abort exectution.  
This is default mode.

`BeCalm`  
In case of logical problem in subsequent functions - continue execution.  
Note that low-level problems, like communication timeout or failure to start far2l - still will abort execution.

`Inspect`  
This function useful for calm mode.  
If no problem happened so far - it returns empty string.  
Otherwise - it returns error description and empties it for next invokations of CheckFailure.

---------------------------------------------------------

`StartApp(["arg1", "arg2" ...])`  
Starts far2l with given arguments, note that path to far2l implicitly inserted as very first argument
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

`CheckBoundedLine("expected string", left, top, width, " \t") string`  
Check that single line at specified rectangle matches to specified value.  
Optionally trims edges of line from trim_chars characters if its not empty.  
If string doesnt match - aborts execution unless in calm mode.  
Returns actual line.

---------------------------------------------------------


`SurroundedLines(x, y, "║═│─", " \t")`  
Returns array of lines bounded by any of specified in boundary_chars characters.  
x, y represends coordinates of any cell inside of required area  
Optionally trims edges of each line from trim_chars characters if its not empty.

---------------------------------------------------------

`CellCharMatches(x, y, "abcdef...") bool`  
`CheckCellChar(x, y, "abcdef...") string`  
Checks if cell under specified coordinates contains any of characters contained in specified string.  
CellCharMatches returns true if cell character matched otherwise it returns false.  
CheckCellChar returns cell character. But if no character matched then aborts execution unless in calm mode.

---------------------------------------------------------

`ExpectString("string", x, y, w, h, timeout_ms)`  
`ExpectStrings(["string 1", "string 2" ...], x, y, w, h, timeout_ms)`  
Waits given amount of milliseconds for given string/any of given strings will appear in provided rectangular area.  
Aborts execution in case no string found before timeout reached unless in calm mode, otherwise:  
Returns result as structure of following fields, that defines index of found string and its coordinates or -1 if no string found:
 * I uint32
 * X uint32
 * Y uint32

---------------------------------------------------------

`ExpectNoString("string", x, y, w, h, timeout_ms)`  
`ExpectNoStrings(["string 1", "string 2" ...], x, y, w, h, timeout_ms)`  
'Inverted' versions of ExpectString/ExpectNoStrings that wait for when string will NO NOT appear in provided rectangular area

---------------------------------------------------------

`ExpectAppExit(code, timeout_ms) string`  
Expects that far2l will exit with specified exit code withing given milliseconds of timeout.  
Aborts execution if app not exited due given timeout or exited with wrong code (unless in calm mode).

---------------------------------------------------------

`Log("string")`  
Writes given string to test output.

---------------------------------------------------------

`Panic("string")`  
Writes given string to test output and aborts tests.

---------------------------------------------------------

`TTYWrite("string")`  
Writes given string to stdin of pseudoterminal where tested far2l is running.

---------------------------------------------------------

`TTYCtrlC()`  
Generates Ctrl+C for pseudoterminal where tested far2l is running.

---------------------------------------------------------

`RunCmd(["prog", "arg1", "arg2" ...])`  
Runs given command, returns empty string if start succeeded and command returned zero code, otherwise aborts unless in calm mode.  
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

`Exists(path string) bool`  
Return true if something exists under specified path

---------------------------------------------------------

`CountExisting(pathes []string) int`  
Return count of existing files from specified pathes list list

---------------------------------------------------------


`Chmod(name string, mode os.FileMode) bool`  
`Chown(name string, uid, gid int) bool`  
`Chtimes(name string, atime time.Time, mtime time.Time) bool`  
`Mkdir(name string, perm os.FileMode) bool`  
`MkdirTemp(dir, pattern string) string`  
`MkdirAll(path string, perm os.FileMode) bool`  
`MkdirsAll(pathes []string, perm os.FileMode)` - custom wrapper around MkdirAll  
`Remove(name string) bool`  
`RemoveAll(name string) bool`  
`Rename(oldpath, newpath string) bool`  
`ReadFile(name string) []byte`  
`WriteFile(name string, data []byte, perm os.FileMode) bool`  
`Truncate(name string, size int64) bool`  
`ReadDir(name string) []os.DirEntry`  
`Symlink(oldname, newname string) bool`  
`Readlink(name string) string`  
This functions are almost matched to ones provided by os package so you can find their description right there: https://pkg.go.dev/os  
Main difference that in case of failure they will abort execution when in panic mode, or will return false or empty result in case of calm mode.

---------------------------------------------------------

`Mkfile(path string, mode os.FileMode, min_size uint64, max_size uint64) bool`  
`Mkfiles(pathes []string, mode os.FileMode, min_size uint64, max_size uint64) bool`  
Creates single file/set of files with given mode of random size within given range filled with random data.  
Aborts execution in case of failure unless in calm mode.

---------------------------------------------------------

`HashPath(path string, hash_data bool, hash_name bool, hash_link bool, hash_mode bool, hash_times bool) string`
`HashPathes(pathes []string, hash_data bool, hash_name bool, hash_link bool, hash_mode bool, hash_times bool) string`

Hashes FS objects at given pathes or single path, returning hash that unique identifies contained objects. If there're directories - they're recursively traversed.  
Hash optionally affected by files data, names, mode and times.  
Thus this function allows to easily check if there're changes in file(s)
In case of any IO error - error text included into hashing result.
