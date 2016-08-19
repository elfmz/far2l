# far2l
Linux port of FAR v2<br>
PRE-ALPHA VERSION - currently interesting only for programmers!!!

Better use CodeLite to open and compile this<br>

License: GNU/GPLv2<br>

Used code from projects:<br>
FAR for Windows<br>
Wine<br>
ANSICON<br>

Before building:<br>
apt-get install gawk m4 libglib2.0-dev libwxbase3.0-dev wx3.0-headers libwxgtk3.0-dev<br>
..or similar according to your system <br>


Notes on porting<br>
I implemented/borrowed from Wine some commonly used WinAPI functions. They all declared in WinPort/WinPort.h corresponding defines can be found in WinPort/WinCompat.h. Both included by WinPort/windows.h. Note that this stuff may not be 1-to-1 to corresponding Win32 functionality also doesn't provide full-UNIX functionality, but it simplifies porting and can be considered as temporary scaffold.<br>
However only main executable linked statically to WinPort, it also _exports_ WinPort functionality, so plugins use it without neccessity to bring own copy of its code. So plugin binary should not statically link to WinPort!<br>
Another important note: while FAR internally is UTF16, so WinPort contains UTF16- related stuff. However native Linux wchar_t size is 4 so potentially Linux FAR may be fully UTF32-capable in future, but while it uses Win32-style UTF16 functions - its not. However programmers must be aware on fact that wchar_t is not 2 bytes long anymore.<br>
