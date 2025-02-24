# DUMPER

## 1. Overview

This tool is designed to assist far2l developers by producing detailed debug prints. All the logging functionality is provided in a single header file (debug.h). There are two main entry points:

 - DUMPV (to_file, ...)
 - DUMP (to_file, ...)

Each of these macros logs variable values along with their names, but they differ in how arguments are provided.

Every log entry is automatically prefixed with a header containing:

- process ID
- thread ID
- formatted local time (with millisecond precision)
- source file and line number information
- the calling function’s name

When outputting string values, any special characters (such as newlines, tabs, quotes, etc.) are automatically escaped to ensure the log remains clear and unambiguous.

Both macros take a first boolean parameter `to_file` that determines where the log entry is sent. If `to_file` is **true**, the log is appended to far2l_debug.log in the user’s home directory (using $HOME or /tmp as fallback). Otherwise, the log is printed to standard error via std::clog.

## 2. DUMPV macro

**Syntax:**

    DUMPV (to_file, var1, var2, ...)

**Purpose:** 
Log simple variables.
Use DUMPV for quickly logging a list of plain variables. It will automatically extract the variable names as written in your source code via stringification. However, be aware that function calls or complex expressions with internal commas are not supported.

**Example Usage:**

    int i = 42;
    double d = 3.1415;
    std::string s = "FAR2L is a Linux port of FAR Manager";
    DUMPV(0, a, b, c);


## 3. DUMP macro

**Syntax:**

    DUMP(to_file, HELPER_MACRO1 (expr1), HELPER_MACRO2 (expr2), ...)


**Purpose:**
Log complex data and/or expressions.

Use DUMP when working with types that need special handling. It requires you to wrap the variable tokens with helper macros (like DVV, DCONT, DSTRBUF, DFLAGS, etc.) so that the logging backend knows how to properly print out the value.

**Example Usage:**

    FARString fs = "The quick brown fox jumps over the lazy dog.";
    std::vector<int> primes {2, 3, 5, 7, 11, 13, 17};
    DUMP(true, DMSG("Hello, far2l world!"), DVV(fs.GetLength()), DCONT(primes,0));


## 4. Additional Helper Macros

### 4.1. DVV macro

**Syntax:**

    DVV (expr)

**Purpose:** 
Wraps a simple variable or expression.

**Example Usage:**

    DUMP(true, DVV(RightAlign && SrcVisualLength > MaxLength));


### 4.2. DMSG macro
**Syntax:**
DMSG (string)

**Purpose:**
Wraps custom text messages.

**Example Usage:**

    DUMP(true, DMSG("Operation completed successfully!"));

### 4.3. DSTRBUF macro

**Syntax:**

     DSTRBUF (ptr, length)

**Purpose:**
Wraps string buffers.
Use DSTRBUF when you are logging string buffers provided as a pointer with an associated length. This is common when dealing with non-null-terminated character arrays or when the length is known separately.

DSTRBUF accepts two arguments:

 -  a pointer to the character data (which may be a `char*`, `unsigned char*`or `wchar_t*`).
 -  the length of the string buffer (in characters).

**Example Usage:**

	WINPORT_DECL(WriteConsole,BOOL,(HANDLE hConsoleOutput, const WCHAR *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved))
	{
	DUMP(true, DSTRBUF(lpBuffer,nNumberOfCharsToWrite));



### 4.4. DBINBUF macro

**Syntax:**

    DBINBUF(ptr, length)

**Purpose:**
Wraps binary buffers.
Use DBINBUF to log the contents of a binary buffer (i.e., an area of memory represented by a pointer and a length in bytes). The macro produces a hex dump output, which is particularly useful when you need to inspect raw binary data.

DBINBUF takes two arguments:
- a pointer to the data you want to dump,
- the length of the data (in bytes).

**Example Usage:**

	struct Header {
		uint32_t magic;
		uint16_t version;
		uint16_t flags;
	};

	Header hdr = { 0xDEADBEEF, 0x0102, 0x000F };
	const wchar_t *wch=L"The quick brown fox jumps over the lazy dog.";

	DUMP(true,
		 DBINBUF(wch, wcslen(wch)*sizeof(wchar_t)),
		 DBINBUF(&hdr, sizeof(hdr)));

### 4.5. DCONT macro

**Syntax:**

    DCONT(container, max_elements)

**Purpose:**
Wraps containers or static arrays.
Use DCONT to log the contents of static arrays and iterable (those providing `begin()` and `end()` methods) containers in a detailed and structured way.

DCONT takes two arguments:
- the container (or static array) you want to inspect,
- an integer indicating the maximum number of elements to log (0 = log all).

**Example Usage:**

    std::vector<std::string> possibilities;
    if (vtc.GetPossibilities(cmd, possibilities) && !possibilities.empty()) {
    	DUMP(true, DCONT(possibilities, 0));


### 4.6. DFLAGS macro

**Syntax:**

    DFLAGS(var, treat_as)

**Purpose:**
Wraps integers representing bit masks or flag sets.
DFLAGS is designed to help with logging values that represent bit masks or flag sets (for example, file attributes or Unix permission bits). It decodes these numeric flag values into human-readable strings.

DFLAGS takes two arguments:

- the flag variable (typically of an unsigned integer type),
- a flag type indicator (an enumeration value, such as `Dumper::FlagsAs::FILE_ATTRIBUTES` or `Dumper::FlagsAs::UNIX_MODE`) to determine how the numeric value should be interpreted.

**Example Usage:**

    int ShellCopy::ShellCopyFile(const wchar_t *SrcName, const FAR_FIND_DATA_EX &SrcData,
     FARString &strDestName, int Append)
     
      DUMP(true,
         DVV(SrcData.strFileName),
         DFLAGS(SrcData.dwFileAttributes, Dumper::FlagsAs::FILE_ATTRIBUTES),
         DFLAGS(SrcData.dwUnixMode, Dumper::FlagsAs::UNIX_MODE));

