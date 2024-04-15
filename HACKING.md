## Coding style
See: [CODESTYLE.md](CODESTYLE.md)

## Lyric
I implemented/borrowed from WINE some commonly used WinAPI functions. They are all declared in WinPort/WinPort.h and corresponding defines can be found in WinPort/WinCompat.h (both are included by WinPort/windows.h). Note that this stuff may not be 1-to-1 to corresponding Win32 functionality also doesn't provide full-UNIX functionality, but it simplifies porting and can be considered as temporary scaffold.

However, only the main executable is linked statically to WinPort, although it also _exports_ WinPort functionality, so plugins use it without the neccessity to bring their own copies of this code. This is the reason that each plugin's binary should not statically link to WinPort.

While FAR internally is UTF16 (because WinPort contains UTF16-related stuff), native Linux wchar_t size is 4 bytes (rather than 2 bytes) so potentially Linux FAR may be fully UTF32-capable console interaction in the future, but while it uses Win32-style UTF16 functions it does not. However, programmers need to be aware that wchar_t is not 2 bytes long anymore.

Inspect all printf format strings: unlike Windows, in Linux both wide and multibyte printf-like functions have the same multibyte and wide specifiers. This means that %s is always multibyte while %ls is always wide. So, any %s used in wide-printf-s or %ws used in any printf should be replaced with %ls.

Update from 27aug: now it's possible by defining WINPORT_DIRECT to avoid renaming used Windows API and also to avoid changing format strings as swprintf will be intercepted by a compatibility wrapper.
Update from 03/11/22: far2l's console emulator capable to correctly render full-width and combining characters as well as 24 bit colors. This caused following deviation of console-simulation functions behavior comparing to original Win32 API counterparts:
 * CHAR_INFO's Char::UnicodeChar field extended to 64 bit length to be able to associate sequence of multiple WCHARs with single cell.
 * Writing to console full-width character causes two cells to be used: first will get given character code in UnicodeChar field but next one will have UnicodeChar set to zero.
 * Writing combined characters - normal character followed by set of diactrical marks - will make UnicodeChar field to contain so-called 'composite' character code that represents sequence of character codes registered with WINPORT(CompositeCharRegister). Actual sequence of WCHARs can be obtained by WINPORT(CompositeCharLookup). There is macro CI_USING_COMPOSITE_CHAR that allows to detect if given CHAR_INFO contains composite character code or normal WCHAR.
 * Both above transformations happen automatically _only_ if using WriteConsole API. If one uses WriteConsoleOutput - then its up to caller to perform that transformations. Failing to do so will cause incorrect rendering of full-width or diactrical characters.
 * CHAR_INFO's and CONSOLE_SCREEN_BUFFER_INFO's Attributes fields extended to 64 bit to be able to hold 24 bit RGB colors in higher bytes. Use macroses GET_RGB_FORE/GET_RGB_BACK/SET_RGB_FORE/SET_RGB_BACK/SET_RGB_BOTH to access that colors. Note that such colors will be used only if FOREGROUND_TRUECOLOR/BACKGROUND_TRUECOLOR attribute is set. Old attributes define colors from usual 16-elements palette used to render if ..._TRUECOLOR is not set or if backend's target doesn't support more than 16 colors.

## Plugin API

Plugins API based on FAR Manager v2 plus following changes:

### Added following entries to FarStandardFunctions:

* `int Execute(const wchar_t *CmdStr, unsigned int ExecFlags);`
...where ExecFlags - combination of values of EXECUTEFLAGS.
Executes given command line, if EF_HIDEOUT and EF_NOWAIT are not specified then command will be executed on far2l virtual terminal.

* `int ExecuteLibrary(const wchar_t *Library, const wchar_t *Symbol, const wchar_t *CmdStr, unsigned int ExecFlags)`
Executes given shared library symbol in separate process (process creation behaviour is the same as for Execute).
symbol function must be defined as: `int 'Symbol'(int argc, char *argv[])`

* `void DisplayNotification(const wchar_t *action, const wchar_t *object);`
Shows (depending on settings - always or if far2l in background) system shell-wide notification with given title and text.

* `int DispatchInterThreadCalls();`
far2l supports calling APIs from different threads by marshalling API calls from non-main threads into main one and dispatching them on main thread at certain known-safe points inside of dialog processing loops. DispatchInterThreadCalls() allows plugin to explicitly dispatch such calls and plugin must use it periodically in case it blocks main thread with some non-UI activity that may wait for other threads.

* `void BackgroundTask(const wchar_t *Info, BOOL Started);`
If plugin implements tasks running in background it may invoke this function to indicate about pending task in left-top corner.
 * Info is a short description of task or just its owner and must be same string when invoked with Started TRUE or FALSE.

* `size_t StrCellsCount(const wchar_t *Str, size_t CharsCount);`
Returns count of console cells which will be used to display given string of CharsCount characters.

* `size_t StrSizeOfCells(const wchar_t *Str, size_t CharsCount, size_t *CellsCount, BOOL RoundUp);`
Returns count of characters which will be used to fill up to CellsCount cells from given string of CharsCount characters.
RoundUp argument tells what to do with full-width characters that crossed by CellsCount.
On return CellsCount contains cells count that will be filled by returned characters count, that:
 * Can be smaller than initial value if string has too few characters to fill all CellsCount cells or if RoundUp was set to FALSE and last character would then overflow wanted amount.
 * Can be larger by one than initial value if RoundUp was set to TRUE and last full-width character crossed initial value specified in *CellsCount.

* `TruncStr and TruncPathStr`
This two functions not added but changed to use console cells count as string limiting factor.


### Added following commands into FILE_CONTROL_COMMANDS:
* `FCTL_GETPANELPLUGINHANDLE`
Can be used to interact with plugin that renders other panel.
`hPlugin` can be set to `PANEL_ACTIVE` or `PANEL_PASSIVE`.
`Param1` ignored.
`Param2` points to value of type `HANDLE`, call sets that value to handle of plugin that renders specified panel or `INVALID_HANDLE_VALUE`.

### Added following plugin-exported functions:
* `int MayExitFARW();`
far2l asks plugin if it can exit now. If plugin has some background tasks pending it may block exiting of far2l, however it highly recommended to give user choice using UI prompt.

* `int GetLinkTargetW(HANDLE hPlugin, struct PluginPanelItem *PanelItem, wchar_t *Target, size_t TargetSize, int OpMode);`
far2l uses this to resolve symlink destination when user selects plugin's item that has FILE_ATTRIBUTE_REPARSE_POINT. Target is displayed in status field as for local symlinks.

### Added following dialog messages:
* `DM_SETREADONLY` - changes readonly-ness of selected dialog edit control item
* `DM_GETCOLOR` - retrieves current color attributes of selected dialog item
* `DM_SETCOLOR` - changes current color attributes of selected dialog item
* `DM_SETTRUECOLOR` - sets 24-bit RGB colors to selected dialog item, can be used within DN_CTLCOLORDLGITEM handler to provide extra coloring.
* `DM_GETTRUECOLOR` - retrieves 24-bit RGB colors of selected dialog item, if they were set before by DM_SETTRUECOLOR.
* `ECTL_ADDTRUECOLOR` - applies coloring to editor like ECTL_ADDCOLOR does but allows to specify 24 RGB color using EditorTrueColor structure.
* `ECTL_GETTRUECOLOR` - retrieves coloring of editor like ECTL_GETCOLOR does but gets 24 RGB color using EditorTrueColor structure.

Note that all true-color capable messages extend but don't replace 'base' 16 palette colors. This is done intentionally as far2l may run in terminal that doesn't support true color palette, and in such case 24bit colors will be ignored and base palette attributes will be used instead.
