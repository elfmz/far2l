# DUMPER

## Overview

This tool is designed to assist far2l developers by producing detailed debug prints. All logging functionality is provided in the single header file `debug.h`.

There are two main entry points:

- `DUMPV(...)`
- `DUMP(...)`

Each of these macros logs the values of variables along with their names, but they differ in how arguments are provided.

Every log entry is automatically prefixed with a customizable header (see 
[section 4](#4-configuration-options) for configuration details) which by default contains:


- process ID,
- thread ID,
- formatted local time (with millisecond precision),
- source file and line number,
- the calling function’s name.

The logger supports various data types, including primitives, strings, wide strings, pointers, containers, pairs, and custom buffers. For containers, nested structures are displayed with tree-like indentation.

When dumping containers, elements are enumerated using sequential indices (e.g., container[0], container[1]), even for non-indexable types like std::list or std::set. These indices are fictional and serve only for display purposes and navigation in the log structure. The actual traversal is performed using iterators (begin() to end()), ensuring compatibility with all iterable containers.

When logging string values, special characters (such as newlines, tabs, quotes) are automatically escaped to ensure clarity and unambiguity. Note that character escaping via hexadecimal codes uses the C++23 style syntax: `\x{NN}`.

The output destination can be either a file or standard error stream, and is configured by options `WRITE_LOG_TO_FILE` and `LOG_FILENAME`. See [section 4](#4-configuration-options) for more details.

For example, inserting these debug statements at appropriate points in the source code:

```cpp
DUMPV(strStr, possibilities);
DUMP(DVV(SrcName), DFLAGS(SrcData.dwFileAttributes,Dumper::FlagsAs::FILE_ATTRIBUTES), DVV(strDestName));
DUMP(DBINBUF(Data, Size));
```

will produce log output like this:

```
/-----[PID:38000, TID:1]-----[2025-05-08 00:44:18,764]-----
|[/home/testuser/far2l/far2l/src/cmdline.cpp:191] in ProcessTabCompletion()
|=> strStr = sha
|=> possibilities
|   |=> possibilities[0] = sha1sum
|   |=> possibilities[1] = sha224sum
|   |=> possibilities[2] = sha256sum
|   |=> possibilities[3] = sha384sum
|   |=> possibilities[4] = sha512sum
|   \=> possibilities[5] = shasum


/-----[PID:38000, TID:1]-----[2025-05-08 00:46:28,450]-----
|[/home/testuser/far2l/far2l/src/copy.cpp:2803] in ShellCopyFile()
|=> SrcName = /home/testuser/far2l/far2l/bootstrap/unmount.sh
|=> SrcData.dwFileAttributes = ARCHIVE, EXECUTABLE
|=> strDestName = /home/testuser/foobar/unmount.sh


/-----[PID:38000, TID:1]-----[2025-05-08 00:46:28,450]-----
|[/home/testuser/far2l/far2l/src/copy.cpp:2781] in PieceWrite()
|=> Data =
|             00 01 02 03 04 05 06 07 | 08 09 0a 0b 0c 0d 0e 0f  | ASCII
|   -------------------------------------------------------------+-----------------
|   00000000  23 21 2f 62 69 6e 2f 73 | 68 0a 69 66 20 5b 20 22  | #!/bin/sh.if [ "
|   00000010  24 32 22 20 3d 20 27 66 | 6f 72 63 65 27 20 5d 3b  | $2" = 'force' ];
|   00000020  20 74 68 65 6e 0a 09 73 | 75 64 6f 20 75 6d 6f 75  |  then..sudo umou
|   00000030  6e 74 20 2d 66 20 22 24 | 31 22 0a 65 6c 73 65 0a  | nt -f "$1".else.
|   00000040  09 75 6d 6f 75 6e 74 20 | 22 24 31 22 0a 66 69 0a  | .umount "$1".fi.
```


## 1. DUMPV macro

Logs simple variables.

Use it for quickly logging a list of plain variables. It automatically extracts the variable names as written in your source code via stringification.

> **Warning**
> 
> Function calls or complex expressions with internal commas are not supported. If parsing fails, an error message is logged instead.

**Syntax:**

```cpp
DUMPV(var1, var2, ...);
```

**Usage:**

```cpp
int answer = 42;
double pi = 3.14;
std::string name = "FAR2L";

DUMPV(answer, pi, name);
```

## 2. DUMP macro

Logs complex data and/or expressions.

Use when working with types that need special handling. It requires you to wrap the variable tokens with helper macros (like `DVV`, `DCONT`, `DSTRBUF`, `DFLAGS`, etc.), so that the logging backend knows how to properly print out the value.

**Syntax:**

```cpp
DUMP(HELPER_MACRO1(expr1), HELPER_MACRO2(expr2), ...);
```

**Usage:**

```cpp
FARString fs = "The quick brown fox jumps over the lazy dog.";
std::vector<int> primes {2, 3, 5, 7, 11, 13, 17};

DUMP(DMSG("Hello, far2l world!"), DVV(fs.GetLength()), DCONT(primes, 4));
```


## 3. Additional Helper Macros

### 3.1. DVV

Wraps a simple variable or expression.

**Syntax:**

```cpp
DVV(expr)
```

**Usage:**

```cpp
std::string str = "Pack my box with five dozen liquor jugs.";
int x = 2, y = 3, z = 4;

DUMP(DVV(str), DVV(str.find('o', 10)), DVV(x + y < z));
```


### 3.2. DMSG

Wraps a custom text message.

**Syntax:**

```cpp
DMSG(msg)
```

**Usage:**

```cpp
DUMP(DMSG("Operation completed successfully!"));
```

### 3.3. DSTRBUF

Wraps string buffers.

Use when logging string buffers provided as a pointer with an associated length. This is typically needed for non-null-terminated character arrays or strings that may contain embedded null characters.

**Syntax:**

```cpp
DSTRBUF(ptr, length)
```

Accepts two arguments:

- pointer to character data (`char *`, `unsigned char *` or `wchar_t *`),
- length of the string buffer (in characters).

**Usage:**

```cpp
WINPORT_DECL(WriteConsole, BOOL, (HANDLE hConsoleOutput, const WCHAR *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved))
{
	DUMP(DSTRBUF(lpBuffer, nNumberOfCharsToWrite));
	// ...
}
```

### 3.4. DBINBUF

Wraps binary buffers.

Use to log the contents of a binary buffer — a memory region defined by a pointer and its length in bytes. The macro produces a hexadecimal dump, which is especially useful for inspecting raw binary data.

The output consists of byte offsets, hexadecimal representation, and ASCII interpretation ('.' for non-printable).

**Syntax:**

```cpp
DBINBUF(ptr, length)
```

Accepts two arguments:

- pointer to data you want to dump,
- data length (in bytes).

**Usage:**

```cpp
struct Header {
	uint32_t magic;
	uint16_t version;
	uint16_t flags;
};

Header hdr = { 0xDEADBEEF, 0x0102, 0x000F };
const wchar_t *wch = L"The quick brown fox jumps over the lazy dog.";

DUMP(DBINBUF(wch, wcslen(wch)*sizeof(wchar_t)),
	DBINBUF(&hdr, sizeof(hdr)));
```

### 3.5. DCONT

Wraps containers or static arrays.

Use to log the contents of static arrays and iterable containers (i.e. containers that provide both `begin()` and `end()` methods) when you need to limit the number of displayed elements. For a complete container dump, prefer the simpler DVV macro.

**Syntax:**

```cpp
DCONT(container, max_elements)
```

Accepts two arguments:

- container or static array you want to inspect,
- integer indicating the maximum number of elements to log (pass `0` to log all available data).

**Usage:**

```cpp
std::list<std::string> FAR2L = {"Linux", "fork", "of", "FAR Manager"};
DUMP(DCONT(FAR2L, 2));
```

### 3.6. DFLAGS

Wraps integers representing bit masks or flag sets. 

Use for decoding numeric values (such as file attributes or Unix permission bits) into human-readable strings.

**Syntax:**

```cpp
DFLAGS(var, treat_as)
```

Accepts two arguments:

- flag variable (typically of an unsigned integer type),
- flag type indicator (an enumeration value, such as `Dumper::FlagsAs::FILE_ATTRIBUTES`) that determines how the numeric value should be interpreted.

**Usage:**

```cpp
mode_t mode = S_IFDIR | 0755;
DUMP(DFLAGS(mode, Dumper::FlagsAs::UNIX_MODE));
```

### 3.7. DSTACKTRACE

Captures and logs a stack trace at the point of invocation.

Use when you need to understand the call flow leading to a particular code location. The macro captures the current execution context and displays it as a sequence of function calls with associated memory addresses and module information. Stack trace resolution quality depends on platform capabilities and available debug symbols.

**Syntax:**

```cpp
DSTACKTRACE()
```


**Usage:**

```cpp
DUMP(DSTACKTRACE());
```

The stack trace output includes:

- Frames section. Each frame shows module short name, function name (demangled if available), and associated memory addresses.
- Command-line tool section. Ready-to-use shell commands for external debugging tools like addr2line, grouped by module.

This functionality is highly configurable through various compile-time options described in [section 4](#4-configuration-options).

> **Warning: Experimental Feature**
> 
> Stack trace support is experimental, has limited platform availability (guaranteed on Linux), and is fully functional only in debug builds — release/stripped binaries may lack symbol information.


## 4. Configuration Options

The dumper behavior can be customized through the `DumperConfig` structure located at the beginning of `debug.h`.

> **Note**
> 
> All configuration options are compile-time constants and cannot be changed at runtime. Any modifications require recompilation of the affected code.

Here is the full list:

* `WRITE_LOG_TO_FILE`

	Specifies the output destination for log entries. If set to `true`, logs are appended to a file determined by the `LOG_FILENAME` option. If set to `false`, logs are directed to `std::cerr`.

* `LOG_FILENAME`

	Specifies the filename for log output when `WRITE_LOG_TO_FILE` is enabled. The file is created with default permissions (subject to the process's umask) in the user's home directory (determined by the `$HOME` environment variable), or, if unavailable, `/tmp` as a fallback. Default value: `far2l_debug.log`.

* `ENABLE_PID_TID`

	Allows process and thread IDs in log entry headers, formatted as `[PID:12345, TID:1]`. Thread IDs are sequential numbers assigned by the debugging system starting from 1. Each thread receives its unique number upon the first logging call made from that thread.

* `ENABLE_TIMESTAMP`

	Allows timestamps in log entry headers, formatted as: `[YYYY-MM-DD HH:MM:SS,mmm]` with local time and millisecond precision.

* `ENABLE_LOCATION`

	Allows source code location in log entry headers. When enabled, the file path, line number, and calling function name are recorded in the format `[/path/to/file.cpp:123] in FunctionName()`.

* `HEXDUMP_BYTES_PER_LINE`

	Specifies how many bytes are displayed per line in hexadecimal dumps (via the `DBINBUF` macro). The default value is 16.

* `HEXDUMP_MAX_LENGTH`

	Sets the maximum number of bytes to display in a single hexadecimal dump (via the `DBINBUF` macro). This prevents extremely large buffers from overwhelming the log output. When a buffer exceeds this limit, only the first `HEXDUMP_MAX_LENGTH` bytes are displayed, followed by a truncation notice showing both the displayed length and the original buffer size. Set to `0` to disable length limiting entirely. Default: 1MB.

* `CONTAINERS_MAX_INDENT_LEVEL`

	Sets the maximum available nesting depth for displaying hierarchical container structures.

* `STACKTRACE_SHOW_ADDRESSES`

	Controls whether memory addresses are displayed in stack trace output. When enabled, each frame shows the absolute address, module base address, symbol address, and calculated offsets.

* `STACKTRACE_DEMANGLE_NAMES`

	Enables demangling of C++ function names in stack traces. When enabled, mangled symbol names (like `_ZN7Dumper9DumpValueE`) are converted to human-readable function signatures. Requires compiler support for `abi::__cxa_demangle`. When disabled or unavailable, raw mangled names are displayed.

* `STACKTRACE_RETADDR_ADJUSTMENT`

	Specifies the strategy for adjusting return addresses to improve symbol resolution accuracy. Return addresses typically point to the instruction after a function call, but for debugging purposes, the call instruction itself is often more meaningful.

	Available strategies:

	- Off: Use original addresses without modification.
	- PreferAdjusted: Try adjusted addresses first, fall back to original if dladdr() fails.
	- PreferOriginal: Try original addresses first, fall back to adjusted if dladdr() fails.

* `STACKTRACE_SYMBOL_RESOLUTION`

    Specifies the strategy for resolving symbol names from instruction addresses when producing stack traces. Resolution may use the dynamic symbol table (as provided by dladdr()) or the full symbol table (by directly parsing the ELF .symtab/.strtab sections from the binary).

    Available strategies:

	- OnlyDynsym: Resolve only from .dynsym. Fast and consistent with runtime-visible symbols; will not find local or non-exported symbols.
	- OnlySymtab: Resolve only from .symtab. Returns the most complete symbol names (including local/non-exported) but requires .symtab to be present; will fail in stripped/release binaries.
	- PreferDynsym: Try .dynsym first; if lookup fails, fall back to .symtab.
	- PreferSymtab: Try .symtab first; if lookup fails or .symtab is absent, fall back to .dynsym.


* `SHOW_SYMBOL_SOURCE`

	When true, append a source tag indicating whether symbols were resolved via the dynamic symbol table (dynsym) or the full symbol table (symtab); when false, omit the tag.

* `STACKTRACE_SHOW_CMDLINE_TOOL_COMMANDS`

	Generates command-line invocations for external debugging tools (like addr2line) that can be used to obtain detailed source location information. When enabled, the stack trace output includes ready-to-use shell commands for each module, grouped by binary file. These commands can be copied and executed to resolve addresses to source file names and line numbers.

* `STACKTRACE_MAX_FRAMES`

	Sets the maximum number of stack frames to capture using `backtrace()`. This limits memory usage and prevents extremely deep call stacks from overwhelming the output. The actual number of frames displayed may be less due to the `STACKTRACE_SKIP_FRAMES` setting.

* `STACKTRACE_SKIP_FRAMES`

	Specifies the number of initial stack frames to skip when generating a stack trace. This is useful for excluding internal logging functions from the trace output. Default value: 2.
