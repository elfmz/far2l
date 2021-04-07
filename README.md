# far2l
Linux port of FAR Manager v2 (http://farmanager.com/)   
Works also on OSX/MacOS and BSD (but later not tested on regular manner)   
ALPHA VERSION.   
**Currently interesting only for enthusiasts!!!**

Plug-ins that are currently working: NetRocks (SFTP/SCP/FTP/FTPS/SMB/NFS/WebDAV), colorer, multiarc, tmppanel, align, autowrap, drawline, editcase, SimpleIndent, Calculator, Python (optional scripting support)

Linux/MacOS (Travis CI): [![Travis](https://img.shields.io/travis/elfmz/far2l.svg)](https://travis-ci.org/elfmz/far2l)

FreeBSD (Cirrus CI): [![Cirrus](https://api.cirrus-ci.com/github/elfmz/far2l.svg)](https://cirrus-ci.com/github/elfmz/far2l)


#### License: GNU/GPLv2

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
* libwxgtk3.0-dev (or in Ubuntu 19.04+ - libwxgtk3.0-gtk3-dev)  (needed for GUI backend, not needed with -DUSEWX=no)
* libxerces-c-dev
* libspdlog-dev
* libuchardet-dev
* libssh-dev (needed for NetRocks/SFTP)
* libssl-dev (needed for NetRocks/FTPS)
* libsmbclient-dev (needed for NetRocks/SMB)
* libnfs-dev (needed for NetRocks/NFS)
* libneon27-dev (or later, needed for NetRocks/WebDAV)
* libarchive-dev (needed for better archives support in multiarc)
* libpcre2-dev (or in recent distributives - libpcre3-dev) (needed for custom archives support in multiarc)
* cmake ( >= 3.2.2 )
* g++
* git (needed for downloading source code)

#### Or simply on Debian/Ubuntu:
``` sh
apt-get install gawk m4 libwxgtk3.0-dev libpcre2-dev libxerces-c-dev libspdlog-dev libuchardet-dev libssh-dev libssl-dev libsmbclient-dev libnfs-dev libneon27-dev libarchive-dev cmake g++ git

```
In recent distributives: use libpcre3-dev and libwxgtk3.0-gtk3-dev instead of libpcre2-dev and libwxgtk3.0-dev

#### Clone and Build

``` sh
git clone https://github.com/elfmz/far2l
cd far2l
mkdir build
cd build
```
_with make:_
``` sh
cmake -DUSEWX=yes -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc --all)
``` 
_or with ninja (you need **ninja-build** package installed)_
``` sh
cmake -DUSEWX=yes -DCMAKE_BUILD_TYPE=Release -G Ninja ..
ninja -j$(nproc --all)
```

#### OSX/MacOS install

 * Supported compiler: ```AppleClang 8.0.0.x``` or newer. Check your version, and install/update XCode if necessary.
 ```sh
 clang++ -v
 ```

 * If you don't have Homebrew stop by <https://brew.sh/> for installation instructions.

##### One line OSX/MacOS install latest far2l git master via unofficial brew tap

 * With GUI/TTY backends:
```sh
brew install --HEAD yurikoles/yurikoles/far2l --with-wxmac
```
 * With TTY backend only:
```sh
brew install --HEAD yurikoles/yurikoles/far2l
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
sudo port install cmake gawk pkgconfig wxWidgets-3.2 libssh openssl xercesc3 libfmt spdlog
```
 * OR if you prefer to use brew packages, then:
```sh
brew bundle
```
 * After dependencies installed - you can build far2l:
_with make:_
```sh
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local -DUSEWX=yes -DCMAKE_BUILD_TYPE=Release ..
make -j$(sysctl -n hw.logicalcpu)
``` 
_or with ninja_
```sh
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local -DUSEWX=yes -DCMAKE_BUILD_TYPE=Release -G Ninja ..
ninja -j$(sysctl -n hw.logicalcpu)
```

To build without WX backend (console version only): change -DUSEWX=yes to -DUSEWX=no also in this case dont need to install libwxgtk\*-dev package

To save space by exluding support of East Asian codepages set: add -DEACP=no

To build with Python plugin: add argument -DPYTHON=yes

#### Building on Gentoo (and derivatives)
For absolute minimum you need:
```
emerge -avn dev-libs/xerces-c app-i18n/uchardet dev-util/cmake dev-libs/spdlog
```
If you want to build far2l with wxGTK support also install it:
```
emerge -avn x11-libs/wxGTK
```
And if you also want Python plugin, consider installing virtualenv:
```
emerge -avn dev-python/virtualenv
```
Additionally, for NetRocks you will need:
```
emerge -avn net-libs/neon net-libs/libssh net-fs/libnfs net-fs/samba
```
After installing, follow Clone and Build section above.

#### Building DMG, DEB and TGZ packages

Please follow CMake related instructions from previous step to build the project, then run:
``` sh
cpack
```

to generate the following packages (depending on architecture):
- far2l-2.X.X.dmg
- far2l_2.X.X_ARCH.deb 
- far2l-2.X.X.tar.gz

All packages can be found in `build` folder.

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

* int Execute(const wchar_t *CmdStr, unsigned int ExecFlags);
...where ExecFlags - combination of values of EXECUTEFLAGS.
Executes given command line, if EF_HIDEOUT and EF_NOWAIT are not specified then command will be executed on far2l virtual terminal.

* int ExecuteLibrary(const wchar_t *Library, const wchar_t *Symbol, const wchar_t *CmdStr, unsigned int ExecFlags)
Executes given shared library symbol in separate process (process creation behaviour is the same as for Execute).
symbol function must be defined as: int 'Symbol'(int argc, char *argv[])

* void DisplayNotification(const wchar_t *action, const wchar_t *object);
Shows (depending on settings - always or if far2l in background) system shell-wide notification with given title and text.

* int DispatchInterThreadCalls();
far2l supports calling APIs from different threads by marshalling API calls from non-main threads into main one and dispatching them on main thread at certain known-safe points inside of dialog processing loops. DispatchInterThreadCalls() allows plugin to explicitely dispatch such calls and plugin must use it periodically in case it blocks main thread with some non-UI activity that may wait for other threads.

### Added following commands into FILE_CONTROL_COMMANDS:
* FCTL_GETPANELPLUGINHANDLE
Can be used to interract with plugin that renders other panel.
hPlugin can be set to PANEL_ACTIVE or PANEL_PASSIVE.
Param1 ignored.
Param2 points to value of type HANDLE, call sets that value to handle of plugin that renders specified panel or INVALID_HANDLE_VALUE.

### Added following plugin-exported functions:
* int MayExitFARW();
far2l asks plugin if it can exit now. If plugin has some background tasks pending it may block exiting of far2l, however it highly recommended to give user choice using UI prompt.

### Added following dialog messages:
* DM_GETCOLOR - retrieves get current color attributes of selected dialog item
* DM_SETCOLOR - changes current color attributes of selected dialog item
