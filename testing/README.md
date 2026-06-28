## How to run tests
To run tests execute ./far2l-smoke-run.sh with argument - path to far2l which was built by cmake configured with -DTESTING=Yes  
Example: `./far2l-smoke-run.sh ../../far2l.build/install/far2l`  
Note: if provided far2l is built without testing support this will stuck for a while and fail then.

## How to write tests
Actual tests written in JS and located under tests directory. They can use predefined functions described below to perform some actions.  
Add your test as .js file with numbered name prefix, that number defines execution order as tests executed in alphabetical order.  
In general test must:
 * optionally do some preparations, like create some files needed for tests if any etc
 * start far2l using `StartApp()` function. Its recommended to start far2l with unique profile derived from WorkDir() using -u parameter
 * wait for far2l startup by expecting output of left panel title and help page (as on clean profile far2l always shows help on 1st start)
 * perform some interactive actions with far2l by sending key presses and checking presence of some expected strings
 * validate results if need
 * send close command to far2l, e.g. by pressing F10 using TypeFKey(10) and then wait for its shutdown by `ExpectAppExit()`
 * test may also start far2l again to do some other actions withing same test-case, buts this needed rarely

Note that by default many functions that perform validations, like `ExpectString()`, `ExpectAppExit()` etc - abort execution in case of unexpected results. This can be changed by BeCalm() function (see below) if need. But typically its behavior you exactly want.

## Functions list goes below

---------------------------------------------------------

`BePanic()`  
In case of logical problem in subsequent functions - abort tests execution.  
This is default mode.

`BeCalm()`  
In case of logical problem in subsequent functions - continue execution.  
Note that low-level problems, like communication issues or failure to start far2l - still will abort execution.

`Inspect() string`  
This function useful in calm mode.  
If no problem happened so far - it returns empty string.  
Otherwise - it returns error description and empties it for next invokations of `Inspect()`.

---------------------------------------------------------

`StartApp(["arg1", "arg2" ...]) far2l_Status`  
`StartAppWithSize(["arg1", "arg2" ...], cols int, rows int) far2l_Status`  
Starts far2l with given arguments, note that path to far2l implicitly inserted as very first argument  
StartAppWithSize allows also to specify initial terminal size  
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

`Sync(tmout uint32) bool`  
Waits (with given msec timeout) for all input events being processed.  
Fails/Returns false if wait timed out.

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
'Inverted' versions of ExpectString/ExpectStrings that wait until string(s) will NO NOT appear in provided rectangular area

---------------------------------------------------------

`ExpectAppExit(code, timeout_ms) string`  
Expects that far2l will exit with specified exit code within given milliseconds of timeout.  
Aborts execution if app not exited during timeout or exited with wrong code (unless in calm mode).

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
Return count of existing files from specified pathes list

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

---------------------------------------------------------

`SaveTextFile(fpath string, lines []string)`  
Saves given array of strings as LF-separated text file.

---------------------------------------------------------

`BoundedLinesSaveAsTextFile(left uint32, top uint32, width uint32, height uint32, fpath string)`
Saves specified screen area strings as LF-separated text file.

---------------------------------------------------------

`LoadTextFile(fpath string) []string`  
Loads given text file into array of strings

---------------------------------------------------------

`BoundedLinesMatchTextFile(left uint32, top uint32, width uint32, height uint32, fpath string) bool`  
Loads given text file into array of strings and checks that given screen region contains exactly same strings

---------------------------------------------------------
