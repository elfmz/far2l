[![Cirrus Build Status](https://api.cirrus-ci.com/github/elfmz/far2l.svg)](https://cirrus-ci.com/github/elfmz/far2l) [![Coverage Status](https://codecov.io/gh/elfmz/far2l/coverage.svg?branch=master)](https://codecov.io/gh/elfmz/far2l?branch=master) [![Coverity Scan](https://scan.coverity.com/projects/27038/badge.svg)](https://scan.coverity.com/projects/elfmz-far2l) [![Language Grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/elfmz/far2l.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/elfmz/far2l/context:cpp) [![Packages](https://repology.org/badge/tiny-repos/far2l.svg)](https://repology.org/project/far2l)

# far2l [![tag](https://img.shields.io/github/tag/elfmz/far2l.svg)](https://github.com/elfmz/far2l/tags)
Linux fork of FAR Manager v2 (http://farmanager.com/)   
Works also on OSX/MacOS and BSD (but latter not tested on regular manner)   
BETA VERSION.   
**Use on your own risk!**

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

* libwxgtk3.0-gtk3-dev (in new distributives - libwxgtk3.2-dev, in old distributives - libwxgtk3.0-dev)  (needed for GUI backend, not needed with -DUSEWX=no)
* libx11-dev (optional - needed for X11 extension that provides better UX for TTY backend wherever X11 is available)
* libxi-dev (optional - needed for X11/Xi extension that provides best UX for TTY backend wherever X11 Xi extension is available)
* libxerces-c-dev
* libuchardet-dev
* libssh-dev (needed for NetRocks/SFTP)
* libssl-dev (needed for NetRocks/FTPS)
* libsmbclient-dev (needed for NetRocks/SMB)
* libnfs-dev (needed for NetRocks/NFS)
* libneon27-dev (or later, needed for NetRocks/WebDAV)
* libarchive-dev (needed for better archives support in multiarc)
* libunrar-dev (optionally needed for RAR archives support in multiarc - see UNRAR command line option)
* libpcre3-dev (or in older distributives - libpcre2-dev) (needed for custom archives support in multiarc)
* cmake ( >= 3.2.2 )
* g++
* git (needed for downloading source code)

#### Or simply on Debian/Ubuntu:
``` sh
apt-get install libwxgtk3.0-gtk3-dev libx11-dev libxi-dev libpcre3-dev libxerces-c-dev libuchardet-dev libssh-dev libssl-dev libsmbclient-dev libnfs-dev libneon27-dev libarchive-dev cmake g++ git
```

On Debian unstable/sid:

`apt-get install far2l`

A simple sid back port should be as easy as (build your own binary deb from the official source deb package):

```
# you will find the latest dsc link at http://packages.debian.org/sid/far2l
dget http://deb.debian.org/debian/pool/main/f/far2l/far2l_2.5.0~beta+git20230223+ds-2.dsc
dpkg-source -x *.dsc
cd far2l-*/
debuild
# cd .. and install your self built far2l*.deb
```

In older distributives: use libpcre2-dev and libwxgtk3.0-dev instead of libpcre3-dev and libwxgtk3.0-gtk3-dev

#### Clone and Build
 * Clone current master `git clone https://github.com/elfmz/far2l`
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
cmake --build . -j$(nproc --all)
``` 
_or with ninja (you need **ninja-build** package installed)_
``` sh
cmake -DUSEWX=yes -DCMAKE_BUILD_TYPE=Release -G Ninja ..
cmake --build .
```

 * If above commands finished without errors - you may also install far2l, `sudo cmake --install .`

 * Also its possible to create far2l_2.X.X_ARCH.deb or ...tar.gz packages in `_build` directory by running `cmake --build . --target package` command.

##### Additional build configuration options:

To build without WX backend (console version only): change `-DUSEWX=yes` to `-DUSEWX=no` also in this case dont need to install libwxgtk\*-dev package

To force-disable TTY|X and TTY|Xi backends: add argument `-DTTYX=no`; to disable only TTY|Xi - add argument `-DTTYXI=no`

To eliminate libuchardet requirement to reduce far2l dependencies by cost of losing automatic charset detection functionality: add -DUSEUCD=no

To build with Python plugin: add argument `-DPYTHON=yes`

To control how RAR archives will be handled in multiarc:
 `-DUNRAR=bundled` (default) use bundled sources found in multiarc/src/formats/rar/unrar
 `-DUNRAR=lib` use libunrar and unrar utility, also build requires libunrar-dev to be installed
 `-DUNRAR=NO` dont use special unrar code, rar archives will be handled by libarchive unless its also disabled

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
 * Additionally you can enable python support by adding `--with-python@3.10` to the one of two above commands.

##### Full OSX/MacOS build from sources (harder):
Some issues can be caused by conflicting dependencies, like having two versions of wxWidgets, so avoid such situation when installing dependecies.

 * Clone:
```sh
git clone https://github.com/elfmz/far2l
cd far2l
```
 * Install needed dependencies with MacPorts:
``` sh
sudo port install cmake pkgconfig wxWidgets-3.2 libssh openssl xercesc3 libfmt uchardet neon
export PKG_CONFIG_PATH=/opt/local/lib/pkgconfig
```
 * OR if you prefer to use brew packages, then:
```sh
brew bundle -v
export PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$(brew --prefix)/opt/openssl/lib/pkgconfig:$(brew --prefix)/opt/libarchive/lib/pkgconfig"
```
 * After dependencies installed - you can build far2l:
_with make:_
```sh
mkdir _build
cd _build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local -DUSEWX=yes -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(sysctl -n hw.logicalcpu)
``` 
_or with ninja:_
```sh
mkdir _build
cd _build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local -DUSEWX=yes -DCMAKE_BUILD_TYPE=Release -G Ninja ..
cmake --build .
```
 * Then you may create .dmg package by running: `cpack` command.
Note that this step sometimes fails and may succeed from not very first attempt.
Its recommended not to do anything on machine while cpack is in progress.
After .dmg successfully created, you may install it by running `open ...path/to/created/far2l-*.dmg`

#### Building on Gentoo (and derivatives)
For absolute minimum you need:
```
emerge -avn dev-libs/xerces-c app-i18n/uchardet dev-util/cmake
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
 * **CLion**: the same as **QtCreator**.
 * **CodeLite**: use this guide to setup a project: https://wiki.codelite.org/pmwiki.php/Main/TheCMakePlugin (to avoid polluting your source tree, don't create your workspace inside of the far2l directory)

### Useful 3rd-party extras

 * A collection of macros for far2l: https://github.com/corporateshark/far2l-macros
 * Fork of Putty (Windows SSH client) with added far2l TTY extensions support (fluent keypresses, clipboard sharing etc): https://github.com/unxed/putty4far2l
 * Kitty (another fork of Putty) also have far2l TTY extensions support: https://github.com/cyd01/KiTTY
 * putty-nd, one more putty fork with extensions support: https://sourceforge.net/p/putty-nd/
 * Turbo Vision, TUI framework supporting far2l terminal extensions: https://github.com/magiblot/tvision
 * turbo, text editor supporting far2l terminal extensions: https://github.com/magiblot/turbo
 * Tool to import color schemes from windows FAR manager 2 .reg format: https://github.com/unxed/far2l-deb/blob/master/far2l_import.pl

## Notes on porting and FAR Plugin API changes
 * See HACKING.md

## Known issues:
* Only valid translations are English, Russian, Ukrainian and Belarussian (interface only), all other languages require deep correction.
