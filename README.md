# far2l
Linux port of FAR v2 (http://farmanager.com/)
ALPHA VERSION.
**Currently interesting only for entusiasts!!!**

Currently working plugins: colorer, multiarc, farftp, tmppanel, align, autowrap, drawline, editcase, SimpleIndent

[![Travis](https://img.shields.io/travis/elfmz/far2l.svg)](https://travis-ci.org/elfmz/far2l)

#### License: GNU/GPLv2<br>

### Used code from projects

* FAR for Windows
* Wine
* ANSICON
* Portable UnRAR
* 7z ANSI-C Decoder

## Contributing, Hacking
#### Required dependencies

* gawk
* m4
* libglib2.0-dev
* libwxgtk3.0-dev
* cmake ( >= 3.2.2 )
* g++

#### Or simply on Ubuntu:
``` sh
apt-get install gawk m4 libglib2.0-dev libwxgtk3.0-dev cmake g++
```

#### Clone and Build

``` sh
git clone https://github.com/elfmz/far2l
mkdir build
cd build
```
_with make:_
``` sh
cmake -DCMAKE_BUILD_TYPE=Release ../far2l
make -j4
``` 
_or with ninja (you need **ninja-build** package installed)_
``` sh
cmake -DCMAKE_BUILD_TYPE=Release -G Ninja ../far2l
ninja -j4
```

#### Macos build

 * Run brew install glib gawk cmake pkg-config

 * Build latest wxWidgets (brew version is not good one).
Sample configuration: ./configure --disable-shared --disable-debug CC=clang CXX=clang++ CXXFLAGS="-stdlib=libc++ -std=c++11" OBJCXXFLAGS="-stdlib=libc++ -std=c++11" LDFLAGS=-stdlib=libc++ --enable-monolithic --enable-unicode

 * Run cmake -G "Unix Makefiles"

 * Run make

#### IDE Setup
You can import the project into your favourite IDE like QtCreator, CodeLite or any other, which supports cmake or cmake is able to generate projects for

 * **QtCreator**: Select "Open Project" and point QtCreator to the CMakeLists.txt in far2l root directory
 * **CodeLite**: use this guide to setup a project: http://codelite.org/LiteEditor/WorkingWithCMake. Don't create workspace inside far2l directory, so you don't polute your source tree.






## Notes on porting

I implemented/borrowed from Wine some commonly used WinAPI functions. They all declared in WinPort/WinPort.h corresponding defines can be found in WinPort/WinCompat.h. Both included by WinPort/windows.h. Note that this stuff may not be 1-to-1 to corresponding Win32 functionality also doesn't provide full-UNIX functionality, but it simplifies porting and can be considered as temporary scaffold.

However only main executable linked statically to WinPort, it also _exports_ WinPort functionality, so plugins use it without neccessity to bring own copy of its code. So plugin binary should not statically link to WinPort!

While FAR internally is UTF16, so WinPort contains UTF16- related stuff. However native Linux wchar_t size is 4 so potentially Linux FAR may be fully UTF32-capable in future, but while it uses Win32-style UTF16 functions - its not. However programmers must be aware on fact that wchar_t is not 2 bytes long anymore.

Inspect all printf format strings: unlike Windows in Linux both wide and multibyte printf-like functions has same multibyte and wide specifiers. That means %s is always multibyte, while %ls is always wide. So any %s used in wide-printf-s or %ws used in any printf should be replaces by %ls.

Update from 27aug: now its possible by defining WINPORT_DIRECT to avoid renaming used WIndows API and also to avoid changing format strings as swprintf will be intercepted by compatibility wrapper.
