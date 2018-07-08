# far2l
Linux port of FAR Manager v2 (http://farmanager.com/)
ALPHA VERSION.
**Currently interesting only for enthusiasts!!!**

Plug-ins that are currently working: colorer, multiarc, farftp, tmppanel, align, autowrap, drawline, editcase, SimpleIndent

[![Travis](https://img.shields.io/travis/elfmz/far2l.svg)](https://travis-ci.org/elfmz/far2l)

#### License: GNU/GPLv2<br>

### Used code from projects

* FAR for Windows
* WINE
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
* git (needed for downloading source code)

#### Or simply on Ubuntu:
``` sh
apt-get install gawk m4 libglib2.0-dev libwxgtk3.0-dev cmake g++ git
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

#### macOS build

 * Supported compiler: ```AppleClang 8.0.0.x``` or newer. Check your version, and install/update XCode if necessary.
 ```sh
 clang++ -v
 ```

 * Install Homebrew:
```sh
ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
```

 * Install required packages:
```sh
brew install glib gawk cmake pkg-config wget
```

 * Download and build latest source from https://wxWidgets.org (brew version is too old?). Example: 
```sh
wget https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.1/wxWidgets-3.1.1.tar.bz2
bunzip2 wxWidgets-3.1.1.tar.bz2
tar xvf wxWidgets-3.1.1.tar
cd wxWidgets-3.1.1
./configure --disable-shared --disable-debug CC=clang CXX=clang++ CXXFLAGS="-stdlib=libc++ -std=c++11" OBJCXXFLAGS="-stdlib=libc++ -std=c++11" LDFLAGS=-stdlib=libc++ --enable-monolithic --enable-unicode
make -j4
make install
```

 * Download, generate makefiles and build far2l:
```sh
git clone https://github.com/elfmz/far2l
cd far2l
cmake -G "Unix Makefiles"
make -j4
```



#### IDE Setup
You can import the project into your favourite IDE like QtCreator, CodeLite, or any other, which supports cmake or which cmake is able to generate projects for.

 * **QtCreator**: select "Open Project" and point QtCreator to the CMakeLists.txt in far2l root directory
 * **CodeLite**: use this guide to setup a project: http://codelite.org/LiteEditor/WorkingWithCMake (to avoid polluting your source tree, don't create your workspace inside of the far2l directory)

#### Useful add-ons

 * A collection of macros: https://github.com/corporateshark/far2l-macros






## Notes on porting

I implemented/borrowed from WINE some commonly used WinAPI functions. They are all declared in WinPort/WinPort.h and corresponding defines can be found in WinPort/WinCompat.h (both are included by WinPort/windows.h). Note that this stuff may not be 1-to-1 to corresponding Win32 functionality also doesn't provide full-UNIX functionality, but it simplifies porting and can be considered as temporary scaffold.

However, only the main executable is linked statically to WinPort, although it also _exports_ WinPort functionality, so plugins use it without the neccessity to bring their own copies of this code. This is the reason that each plugin's binary should not statically link to WinPort.

While FAR internally is UTF16 (because WinPort contains UTF16-related stuff), native Linux wchar_t size is 4 bytes (rather than 2 bytes) so potentially Linux FAR may be fully UTF32-capable console interaction in the future, but while it uses Win32-style UTF16 functions it does not. However, programmers need to be aware that wchar_t is not 2 bytes long anymore.

Inspect all printf format strings: unlike Windows, in Linux both wide and multibyte printf-like functions have the same multibyte and wide specifiers. This means that %s is always multibyte while %ls is always wide. So, any %s used in wide-printf-s or %ws used in any printf should be replaced with %ls.

Update from 27aug: now it's possible by defining WINPORT_DIRECT to avoid renaming used Windows API and also to avoid changing format strings as swprintf will be intercepted by a compatibility wrapper.
