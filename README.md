# far2l
Linux fork of FAR Manager v2 (http://farmanager.com/)   
Works also on OSX/MacOS and BSD (but later not tested on regular manner)   
BETA VERSION.   
**Use on your own risk!!!**

Plug-ins that are currently working: NetRocks (SFTP/SCP/FTP/FTPS/SMB/NFS/WebDAV), colorer, multiarc, tmppanel, align, autowrap, drawline, editcase, SimpleIndent, Calculator, Python (optional scripting support)

FreeBSD/MacOS (Cirrus CI): [![Cirrus](https://api.cirrus-ci.com/github/elfmz/far2l.svg)](https://cirrus-ci.com/github/elfmz/far2l)


#### License: GNU/GPLv2

### Used code from projects

* FAR for Windows and some of its plugins
* WINE
* ANSICON
* Portable UnRAR
* 7z ANSI-C Decoder
* utf-cpp by ww898

## Contributing, Hacking
#### Required dependencies

* gawk
* m4
* libwxgtk3.0-gtk3-dev (or in older distributives - libwxgtk3.0-dev)  (needed for GUI backend, not needed with -DUSEWX=no)
* libx11-dev (optional - needed for X11 extension that provides better UX for TTY backend wherever X11 is available)
* libxi-dev (optional - needed for X11/Xi extension that provides best UX for TTY backend wherever X11 Xi extension is available)
* libxerces-c-dev
* libspdlog-dev
* libuchardet-dev
* libssh-dev (needed for NetRocks/SFTP)
* libssl-dev (needed for NetRocks/FTPS)
* libsmbclient-dev (needed for NetRocks/SMB)
* libnfs-dev (needed for NetRocks/NFS)
* libneon27-dev (or later, needed for NetRocks/WebDAV)
* libarchive-dev (needed for better archives support in multiarc)
* libpcre3-dev (or in older distributives - libpcre2-dev) (needed for custom archives support in multiarc)
* cmake ( >= 3.2.2 )
* g++
* git (needed for downloading source code)

#### Or simply on Debian/Ubuntu:
``` sh
apt-get install gawk m4 libwxgtk3.0-gtk3-dev libx11-dev libxi-dev libpcre3-dev libxerces-c-dev libspdlog-dev libuchardet-dev libssh-dev libssl-dev libsmbclient-dev libnfs-dev libneon27-dev libarchive-dev cmake g++ git

```
In older distributives: use libpcre2-dev and libwxgtk3.0-dev instead of libpcre3-dev and libwxgtk3.0-gtk3-dev

#### Clone and Build
 * Clone current master `git: clone https://github.com/elfmz/far2l`
 * Checkout some stable release tag (master considered unstable): `git checkout v_2.#.#`
 * Prepare build directory:
``` sh
mkdir -p far2l/_build
cd far2l/_build
```

 * Build:
_with make:_
``` sh
cmake -DUSEWX=yes -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc --all)
``` 
_or with ninja (you need **ninja-build** package installed)_
``` sh
cmake -DUSEWX=yes -DCMAKE_BUILD_TYPE=Release -G Ninja ..
ninja
```

 * If above commands finished without errors - you may also install far2l, _with make:_ `sudo make install` _or with ninja:_ `sudo ninja install`

 * Also its possible to create far2l_2.X.X_ARCH.deb or ...tar.gz packages in `_build` directory by running `cpack` command

##### Additional build configuration options:

To build without WX backend (console version only): change -DUSEWX=yes to -DUSEWX=no also in this case dont need to install libwxgtk\*-dev package

To force-disable TTY|X and TTY|Xi backends: add argument -DTTYX=no; to disable only TTY|Xi - add argument -DTTYXI=no

To eliminate libuchardet requirement to reduce far2l dependencies by cost of losing automatic charset detection functionality: add -DUSEUCD=no

To build with Python plugin: add argument -DPYTHON=yes

There're also options to toggle other plugins build in same way: ALIGN AUTOWRAP CALC COLORER COMPARE DRAWLINE EDITCASE EDITORCOMP FARFTP FILECASE INCSRCH INSIDE MULTIARC NETROCKS SIMPLEINDENT TMPPANEL

#### OSX/MacOS install

 * You can install prebuilt package for x86_64 platform via Homebrew Cask, by command:
 ```sh
 brew install --cask far2l
 ```
 * You can also manually download and install prebuilt package for x86_64 platform from Releases page: <https://github.com/elfmz/far2l/releases>
 * But if you need custom/recent build, you can proceed to build steps below: using brew or MacPorts.
 * Supported compiler: `AppleClang 8.0.0.x` or newer. Check your version, and install/update Xcode if necessary.
 ```sh
 clang++ -v
 ```
 * If you want to build using Homebrew - first visit <https://brew.sh/> for installation instructions. Note that there're reported problems with Homebrew-based build under MacOS Big Sur.
 * If you want to build using MacPorts - first visit <https://www.macports.org/install.php> for installation instructions.

##### One line OSX/MacOS install latest far2l git master via unofficial brew tap

 * With GUI/TTY backends:
```sh
brew install --HEAD yurikoles/yurikoles/far2l
```
 * With TTY backend only:
```sh
brew install --HEAD yurikoles/yurikoles/far2l --without-wxwidgets
```
 * Additionally you can enable python support by adding `--with-python@3.9`

##### Full OSX/MacOS build from sources (harder):
Some issues can be caused by conflicting dependencies, like having two versions of wxWidgets, so avoid such situation when installing dependecies.

 * Clone:
```sh
git clone https://github.com/elfmz/far2l
cd far2l
```
 * Install needed dependencies with MacPorts:
``` sh
sudo port install cmake gawk pkgconfig wxWidgets-3.2 libssh openssl xercesc3 libfmt spdlog uchardet neon
```
 * OR if you prefer to use brew packages, then:
```sh
brew bundle -v
```
 * After dependencies installed - you can build far2l:
_with make:_
```sh
mkdir _build
cd _build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local -DUSEWX=yes -DCMAKE_BUILD_TYPE=Release ..
make -j$(sysctl -n hw.logicalcpu)
``` 
_or with ninja:_
```sh
mkdir _build
cd _build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local -DUSEWX=yes -DCMAKE_BUILD_TYPE=Release -G Ninja ..
ninja
```
 * Then you may create .dmg package by running: `cpack` command.
Note that this step sometimes fails and may succeed from not very first attempt.
Its recommended not to do anything on machine while cpack is in progress.
After .dmg successfully created, you may install it by running `open ...path/to/created/far2l-*.dmg`

#### Building on Gentoo (and derivatives)
For absolute minimum you need:
```
emerge -avn dev-libs/xerces-c app-i18n/uchardet dev-util/cmake dev-libs/spdlog
```
If you want to build far2l with wxGTK support also install it:
```
emerge -avn x11-libs/wxGTK
```
Additionally, for NetRocks you will need:
```
emerge -avn net-libs/neon net-libs/libssh net-fs/libnfs net-fs/samba
```
After installing, follow Clone and Build section above.

#### Installing and Building on [NixOS](https://nixos.org/)

To install system-wide, add the `far2l` package to your [`configuration.nix`](https://nixos.org/manual/nixos/stable/index.html#sec-changing-config) `environment.systemPackages` list. To run the application on-demand without affecting the system state, execute `nix-shell -p far2l --command far2l`. These use a package version from your current [channel](https://nixos.wiki/wiki/Nix_channels).

The Far2l adaptation for _nix_ is [a small file on GitHub](https://github.com/NixOS/nixpkgs/blob/master/pkgs/applications/misc/far2l/default.nix), it tells which Git revision from Far2l repo to fetch, with what dependencies to build it, and how to patch its references to other software to make it run in isolated fashion independently from other versions available in the system.

You can build and run `far2l` package for any revision:
* Directly from GitHub (`NixOS/nixpkgs` repo, or your own fork and branch):
 ``` sh
nix-shell -I nixpkgs=https://github.com/<fork>/nixpkgs/archive/<revision-or-branch>.tar.gz -p far2l --command far2l
 ```
* From a locally cloned working directory of the repo:
``` sh
nix-shell -I nixpkgs=/path/to/nixpkgs -p far2l --command far2l
```

To advance the package to a new Far2l revision, edit the `fetchFromGitHub` set attributes `rev` (revision hash) and `sha256` (revision content hash). **Important!** If you leave the old content hash, the old cached content for that hash might be used without attempting to download the new revision. If you're not expecting the build to break, the easiest would be to make a fork, push the change, and build straight from github. 

#### IDE Setup
You can import the project into your favourite IDE like QtCreator, CodeLite, or any other, which supports cmake or which cmake is able to generate projects for.

 * **QtCreator**: select "Open Project" and point QtCreator to the CMakeLists.txt in far2l root directory
 * **CodeLite**: use this guide to setup a project: https://wiki.codelite.org/pmwiki.php/Main/TheCMakePlugin (to avoid polluting your source tree, don't create your workspace inside of the far2l directory)

### Useful 3rd-party extras

 * A collection of macros for far2l: https://github.com/corporateshark/far2l-macros
 * Fork of Putty (Windows SSH client) with added far2l TTY extensions support (fluent keypresses, clipboard sharing etc): https://github.com/unxed/putty4far2l
 * Similar fork of Kitty: https://github.com/mihmig/KiTTY
 * Tool to import color schemes from windows FAR manager 2 .reg format: https://github.com/unxed/far2l-deb/blob/master/far2l_import.pl

## Notes on porting

I implemented/borrowed from WINE some commonly used WinAPI functions. They are all declared in WinPort/WinPort.h and corresponding defines can be found in WinPort/WinCompat.h (both are included by WinPort/windows.h). Note that this stuff may not be 1-to-1 to corresponding Win32 functionality also doesn't provide full-UNIX functionality, but it simplifies porting and can be considered as temporary scaffold.

However, only the main executable is linked statically to WinPort, although it also _exports_ WinPort functionality, so plugins use it without the neccessity to bring their own copies of this code. This is the reason that each plugin's binary should not statically link to WinPort.

While FAR internally is UTF16 (because WinPort contains UTF16-related stuff), native Linux wchar_t size is 4 bytes (rather than 2 bytes) so potentially Linux FAR may be fully UTF32-capable console interaction in the future, but while it uses Win32-style UTF16 functions it does not. However, programmers need to be aware that wchar_t is not 2 bytes long anymore.

Inspect all printf format strings: unlike Windows, in Linux both wide and multibyte printf-like functions have the same multibyte and wide specifiers. This means that %s is always multibyte while %ls is always wide. So, any %s used in wide-printf-s or %ws used in any printf should be replaced with %ls.

Update from 27aug: now it's possible by defining WINPORT_DIRECT to avoid renaming used Windows API and also to avoid changing format strings as swprintf will be intercepted by a compatibility wrapper.

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
far2l supports calling APIs from different threads by marshalling API calls from non-main threads into main one and dispatching them on main thread at certain known-safe points inside of dialog processing loops. DispatchInterThreadCalls() allows plugin to explicitely dispatch such calls and plugin must use it periodically in case it blocks main thread with some non-UI activity that may wait for other threads.

* `int BackgroundTask(const wchar_t *Info, BOOL Started);`
If plugin implements tasks running in background it may invoke this function to indicate about pending task in left-top corner.
Info is a short description of task or just its owner and must be same string when invoked with Started TRUE or FALSE.

### Added following commands into FILE_CONTROL_COMMANDS:
* `FCTL_GETPANELPLUGINHANDLE`
Can be used to interract with plugin that renders other panel.
`hPlugin` can be set to `PANEL_ACTIVE` or `PANEL_PASSIVE`.
`Param1` ignored.
`Param2` points to value of type `HANDLE`, call sets that value to handle of plugin that renders specified panel or `INVALID_HANDLE_VALUE`.

### Added following plugin-exported functions:
* `int MayExitFARW();`
far2l asks plugin if it can exit now. If plugin has some background tasks pending it may block exiting of far2l, however it highly recommended to give user choice using UI prompt.

### Added following dialog messages:
* `DM_GETCOLOR` - retrieves get current color attributes of selected dialog item
* `DM_SETCOLOR` - changes current color attributes of selected dialog item

### Known issues:
* Only valid translations are Russian and English, all other languages require deep correction.
* Characters that occupies more than single 'placeholder' or diacritic-like characters are rendered buggy, that means Chinees and Japaneese texts are hardly readable.
