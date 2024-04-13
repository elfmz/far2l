To run tests run far2l-smoke-run.sh specifying path to far2l which was built by cmake configured with -DTESTING=Yes

Example: `./far2l-smoke-run.sh ../../far2l.build/install/far2l`

Note: if provided far2l built without testing support this will just stuck.. for now.

Actual tests written in JS and located under test directory. They can use following predefined functions to perform actions:

---------------------------------------------------------

`StartApp(["arg1", "arg2" ...])`

Starts far2l with given arguments, note that some arguments are implicitly inserted - path to far2l as very first argument and --test=

Returns status of started far2l as structure of following fields:
 * Width uint32
 * Height uint32
 * Title string

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
 * BackTrue uint32       - 24 bit foreground color if its used
 * ForeTrue uint32       - 24 bit background color if its used
 * BackRGBI uint8        - 4 bit base foreground color
 * ForeRGBI uint8        - 4 bit base background color
 * IsBackTrue bool       - true if 24 bit foreground color is used
 * IsForeTrue bool       - true if 24 bit background color is used
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

`ExpectStrings(["string 1", "string 2" ...], x, y, w, h, timeout_ms)`

Waits given amount of milliseconds for any of given strings will appear in provided rectangular area.

Returns result as structure of following fields, that defines index of found string and its coordinates:
 * I uint32
 * X uint32
 * Y uint32
In case no string found before timeout reached - all fields set to 0xffffffff (-1)

---------------------------------------------------------

`ExpectString("string", x, y, w, h, timeout_ms)`

Simplified version of ExpectStrings that waits for one string only.

Returns same structure as ExpectStrings.

---------------------------------------------------------

`ExpectAppExit(code, timeout_ms)`

Expects that far2l will exit with specified exit code withing given milliseconds of timeout.

Returns empty string if everything happen as expected, otherwise will return string with problem description.

---------------------------------------------------------

`LogInfo(string)`

Writes given string to test output.

---------------------------------------------------------

`LogFatal(string)`

Writes given string to test output and aborts tests.

---------------------------------------------------------

`WriteTTY(string)`

Writes given string to stdin of pseudoterminal where tested far2l is running.

---------------------------------------------------------

`CtrlC()`

Generates Ctrl+C event on pseudoterminal where tested far2l is running.

---------------------------------------------------------

`RunCmd(["prog", "arg1", "arg2" ...])`

Runs given command, returns empty string if start succeeded and command returned zero code, otherwise returns error description.

Note that command is run NOT in pseudoterminal where tested far2l is running.
