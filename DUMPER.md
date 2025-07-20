# DUMPER

## Overview

This tool is designed to assist far2l developers by producing detailed debug prints. All logging functionality is provided in the single header file `debug.h`.

There are two main entry points:

- `DUMPV(to_file, ...)`
- `DUMP(to_file, ...)`

Each of these macros logs the values of variables along with their names, but they differ in how arguments are provided.

Every log entry is automatically prefixed with a header containing:

- process ID,
- thread ID,
- formatted local time (with millisecond precision),
- source file and line number,
- the calling function’s name.

When logging string values, special characters (such as newlines, tabs, quotes) are automatically escaped to ensure clarity and unambiguity in the log.

Both macros take a boolean parameter `to_file` as their first argument, which determines the output destination of the log entry. If `to_file` is `true`, the log is appended to `far2l_debug.log` in the user’s home directory (using `$HOME` or, if unavailable, `/tmp` as a fallback). Otherwise, the log is printed to standard error via `std::clog`.

Output example:

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
|=> SrcName = .editorconfig
|=> SrcData.dwUnixMode = 100644 (rw-r--r--)
|=> strDestName = /home/testuser/foobar/.editorconfig


/-----[PID:38000, TID:1]-----[2025-05-08 00:46:28,450]-----
|[/home/testuser/far2l/far2l/src/copy.cpp:2781] in PieceWrite()
|=> Data =
|             00 01 02 03 04 05 06 07 | 08 09 0a 0b 0c 0d 0e 0f  | ASCII
|   -------------------------------------------------------------+-----------------
|   00000000  72 6f 6f 74 20 3d 20 74 | 72 75 65 0a 0a 5b 2a 5d  | root = true..[*]
|   00000010  0a 63 68 61 72 73 65 74 | 20 3d 20 75 74 66 2d 38  | .charset = utf-8
|   00000020  0a 65 6e 64 5f 6f 66 5f | 6c 69 6e 65 20 3d 20 6c  | .end_of_line = l
|   00000030  66 0a 69 6e 64 65 6e 74 | 5f 73 74 79 6c 65 20 3d  | f.indent_style =
|   00000040  20 74 61 62 0a 69 6e 64 | 65 6e 74 5f 73 69 7a 65  |  tab.indent_size
|   00000050  20 3d 20 34 0a 0a 5b 2a | 2e 79 6d 6c 5d 0a 69 6e  |  = 4..[*.yml].in
|   00000060  64 65 6e 74 5f 73 74 79 | 6c 65 20 3d 20 73 70 61  | dent_style = spa
|   00000070  63 65 0a 69 6e 64 65 6e | 74 5f 73 69 7a 65 20 3d  | ce.indent_size =
|   00000080  20 32 0a 0a 5b 43 4d 61 | 6b 65 4c 69 73 74 73 2e  |  2..[CMakeLists.
|   00000090  74 78 74 5d 0a 69 6e 64 | 65 6e 74 5f 73 74 79 6c  | txt].indent_styl
|   000000a0  65 20 3d 20 73 70 61 63 | 65 0a                    | e = space.      
```


## 1. DUMPV macro

Logs simple variables.

Use it for quickly logging a list of plain variables. It automatically extracts the variable names as written in your source code via stringification.

> **Warning**
> 
> Function calls or complex expressions with internal commas are not supported.

**Syntax:**

```cpp
DUMPV(to_file, var1, var2, ...);
```

**Usage:**

```cpp
int i = 42;
double d = 3.1415;
std::string s = "FAR2L is a Linux port of FAR Manager";

DUMPV(true, i, d, s);
```

## 2. DUMP macro

Logs complex data and/or expressions.

Use when working with types that need special handling. It requires you to wrap the variable tokens with helper macros (like `DVV`, `DCONT`, `DSTRBUF`, `DFLAGS`, etc.), so that the logging backend knows how to properly print out the value.

**Syntax:**

```cpp
DUMP(to_file, HELPER_MACRO1(expr1), HELPER_MACRO2(expr2), ...);
```

**Usage:**

```cpp
FARString fs = "The quick brown fox jumps over the lazy dog.";
std::vector<int> primes {2, 3, 5, 7, 11, 13, 17};

DUMP(true, DMSG("Hello, far2l world!"), DVV(fs.GetLength()), DCONT(primes, 0));
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
DUMP(true, DVV(RightAlign && SrcVisualLength > MaxLength));
```


### 3.2. DMSG

Wraps custom text messages.

**Syntax:**

```cpp
DMSG(msg)
```

**Usage:**

```cpp
DUMP(true, DMSG("Operation completed successfully!"));
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
	DUMP(true, DSTRBUF(lpBuffer, nNumberOfCharsToWrite));
	// ...
}
```

### 3.4. DBINBUF

Wraps binary buffers.

Use to log the contents of a binary buffer — a memory region defined by a pointer and its length in bytes. The macro produces a hexadecimal dump, which is especially useful for inspecting raw binary data.

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

DUMP(true,
	DBINBUF(wch, wcslen(wch)*sizeof(wchar_t)),
	DBINBUF(&hdr, sizeof(hdr)));
```

### 3.5. DCONT

Wraps containers or static arrays.

Use to log the contents of static arrays and iterable containers (i.e. containers that provide both `begin()` and `end()` methods) when you need to limit the number of displayed elements; for a complete container dump, prefer the simpler DVV macro.

**Syntax:**

```cpp
DCONT(container, max_elements)
```

Accepts two arguments:

- container or static array you want to inspect,
- integer indicating the maximum number of elements to log (pass `0` to log all available data).

**Usage:**

```cpp
std::vector<std::string> possibilities;

if (vtc.GetPossibilities(cmd, possibilities) && !possibilities.empty()) {
	DUMP(true, DCONT(possibilities, 0));
}
```

### 3.6. DFLAGS

Wraps integers representing bit masks or flag sets.

This macro is designed to help log values that represent bit masks or flag sets (for example, file attributes or Unix permission bits) by decoding these numeric flag values into human-readable strings.

**Syntax:**

```cpp
DFLAGS(var, treat_as)
```

Accepts two arguments:

- flag variable (typically of an unsigned integer type),
- flag type indicator (an enumeration value, such as `Dumper::FlagsAs::FILE_ATTRIBUTES`) that determines how the numeric value should be interpreted.

**Usage:**

```cpp
int ShellCopy::ShellCopyFile(const wchar_t *SrcName, const FAR_FIND_DATA_EX &SrcData,
	FARString &strDestName, int Append)

// ...
     
DUMP(true,
	 DVV(SrcData.strFileName),
	 DFLAGS(SrcData.dwFileAttributes, Dumper::FlagsAs::FILE_ATTRIBUTES),
	 DFLAGS(SrcData.dwUnixMode, Dumper::FlagsAs::UNIX_MODE));
```
