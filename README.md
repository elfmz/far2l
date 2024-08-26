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

### UI Backends
  FAR2L has base UI Backends (see details in build-in help section **UI backends**):

- **GUI** (**WX**): uses wxWidgets, works in graphics mode, ideal UX, requires a lot of X11 dependencies;

- **TTY|Xi**: works in terminal mode, requires a dependency on pair X11 libraries (to access clipboard and to get state of
all keyboard modifiers), almost perfect UX;

- **TTY|X**: works in terminal mode, uses X11 to access clipboard, all keyboard works via terminal;

- **TTY**: plain terminal mode, no X11 dependencies, UX with some restrictions (works fully when running in the [terminal
emulators](#terminals), which provide clipboard access and has their advanced keyboard-protocols).



| Mode<br>(UI Backends) | TTY<br>(plain far2l) | TTY\|X | TTY\|Xi | GUI |
| ---: | --- | --- | --- | --- |
| **Works:** | in terminal | in terminal | in terminal | in Desktop<br>environment<br><sub>(X11<br>or Wayland<br>or macOS)</sub> |
| **Binaries:** | far2l | far2l<br>far2l_ttyx.broker | far2l<br>far2l_ttyx.broker | far2l<br>far2l_gui.so |
| **[Dependencies](#required-dependencies):** | minimal | + libx11 | + libx11, libxi | + wxWidgets, GTK |
| **Keyboard:** | <sub>_Typical terminals_:<br>**only essential<br>key combinations**<br><br>_KiTTY_ (putty fork),<br>_kitty_ (*nix one),<br>_iTerm2_,<br>_Windows Terminal_,<br>far2l’s VT: **full support**</sub> | <sub>_Typical terminals_:<br>**only essential<br>key combinations**<br><br>_KiTTY_ (putty fork),<br>_kitty_ (*nix one),<br>_iTerm2_,<br>_Windows Terminal_,<br>far2l’s VT: **full support**</sub> | <sub>_Typical terminals_:<br>**most of key<br>combinations under x11**;<br>**only essential key<br>combinations<br>under Wayland**<br><br>_KiTTY_ (putty fork),<br>_kitty_ (*nix one),<br>_iTerm2_,<br>_Windows Terminal_,<br>far2l’s VT: **full support**</sub> | **All key<br>combinations** |
| **Clipboard<br>access:** | <sub>_Typical terminals_:<br>via command line<br>tools like xclip<br><br>_kitty_ (*nix one),<br>_iTerm2_:<br>via **OSC52**<br><br>_Windows Terminal_:<br>via **OSC52**<br>or via **command line<br>tools under WSL**<br><br>_KiTTY_ (putty fork),<br>far2l’s VT:<br>via **far2l extensions**</sub> | <sub>_Typical terminals_,<br>_kitty_ (*nix one):<br>via **x11 interaction**<br><br>_iTerm2_:<br>via **OSC52**<br><br>_Windows Terminal_:<br>via **OSC52**<br>or via **command line<br>tools under WSL**<br><br>_KiTTY_ (putty fork),<br>far2l’s VT:<br>via **far2l extensions**</sub> | <sub>_Typical terminals_,<br>_kitty_ (*nix one):<br>via **x11 interaction**<br><br>_iTerm2_:<br>via **OSC52**<br><br>_Windows Terminal_:<br>via **OSC52**<br>or via **command line<br>tools under WSL**<br><br>_KiTTY_ (putty fork),<br>far2l’s VT:<br>via **far2l extensions**</sub> | via<br>**wxWidgets API**<br><br><sub>via command line<br>tools under WSL</sub> |
| **Typical<br>use case:** | **Servers**,<br>embedded | <sub>Run far2l in<br>favorite terminal<br>but with<br>**better UX**</sub> | <sub>Run far2l in<br>favorite terminal<br>but with<br>**best UX**</sub> | **Desktop** |
| **Mandatory:** | yes | no | no | no |

<sub>_Note about use OSC 52 in TTY/TTY|X_:
to interact with the system clipboard you must **not forget to enable OSC 52**
in both the **FAR2L settings** (`Options`->`Interface settings`->`Use OSC52 to set clipboard data`,
which shown in the dialog only if far2l run in TTY/TTY|X mode and all other options for clipboard access are unavailable;
you can run `far2l --tty --nodetect` to force not use others clipboard options),
and in **terminal settings** option OSC 52 must be allowed (by default, OSC 52 is disabled in some terminals for security reasons;
OSC 52 in many terminals is implemented only for the copy mode, and paste from the terminal goes by bracketed paste mode).</sub>


## Installing, Running
#### Debian/Ubuntu 23.10+ binaries (with TTY X/Xi backends only)

```sh
apt-get install far2l
```

Only under Ubuntu Desktop 23.10 with Wayland run as
`far2l --nodetect=xi --ee`


<sub>**Debian** has far2 in **sid-unstable** / **13 trixie-testing** / **12 bookworm-backports**; **Ubuntu** from **23.10**.
Details about versions in the official repositories see in
https://packages.debian.org/search?keywords=far2l or https://packages.ubuntu.com/search?keywords=far2l </sub>


<sub>Note: now far2l in official repositories Debian/Ubuntu is only TTY|Xi version with extra dependencies of pair X11-libs.
It may be not convenient for some servers.
For servers without X and only terminal/ssh access the plain far2l-TTY version is more suitable
(binaries or portable see in [Community packages & binaries](#community_bins)).</sub>

#### OSX/MacOS binaries

You can install prebuilt package for x86_64 platform via Homebrew Cask, by command:
```sh
brew install --cask far2l
```

You can also manually download and install prebuilt package for x86_64 platform from Releases page: <https://github.com/elfmz/far2l/releases>

#### Docker

You can use containers to try `far2l` without installing anything.
```sh
docker build . -l far2l
docker run -it far2l
```

See also [Community packages & binaries](#community_bins)


## Building, Contributing, Hacking
#### Required dependencies

* `libwxgtk3.0-gtk3-dev` (or `libwxgtk3.2-dev` in newer distributions, or `libwxgtk3.0-dev` in older ones, _optional_ - needed for **GUI backend**, not needed with `-DUSEWX=no`)
* `libx11-dev` (_optional_ - needed for **X11 extension** that provides better UX for TTY backend wherever X11 is available)
* `libxi-dev` (_optional_ - needed for **X11/Xi extension** that provides best UX for TTY backend wherever X11 Xi extension is available)
* `libxml2-dev` (_optional_ - needed for **Colorer plugin**, not needed with `-DCOLORER=no`)
* `libuchardet-dev` (_optional_ - needed for **auto charset detection**, not needed with `-DUSEUCD=no`)
* `libssh-dev` (_optional_ - needed for **NetRocks/SFTP**)
* `libssl-dev` (_optional_ - needed for **NetRocks/FTPS**)
* `libsmbclient-dev` (_optional_ - needed for **NetRocks/SMB**)
* `libnfs-dev` (_optional_ - needed for **NetRocks/NFS**)
* `libneon27-dev` (or later, _optional_ - needed for **NetRocks/WebDAV**)
* `libarchive-dev` (_optional_ - needed for better archives support in **multiarc**)
* `libunrar-dev` (_optional_ - needed for RAR archives support in **multiarc**, see UNRAR command line option)
* `libpcre3-dev` (or `libpcre2-dev` in older distributions, _optional_ - needed for advanced custom archive formats support in **multiarc**)
* `cmake` ( >= 3.2.2 )
* `pkg-config`
* `g++`
* `git` (needed for downloading source code)

#### Or simply on Debian/Ubuntu:
``` sh
apt-get install libwxgtk3.0-gtk3-dev libx11-dev libxi-dev libpcre3-dev libxml2-dev libuchardet-dev libssh-dev libssl-dev libsmbclient-dev libnfs-dev libneon27-dev libarchive-dev cmake pkg-config g++ git
```

A simple sid back port should be as easy as (build your own binary deb from the official source deb package):
```sh
# you will find the latest dsc link at http://packages.debian.org/sid/far2l
dget http://deb.debian.org/debian/pool/main/f/far2l/far2l_2.5.0~beta+git20230223+ds-2.dsc
dpkg-source -x *.dsc
cd far2l-*/
debuild
# cd .. and install your self built far2l*.deb
```

In older distributions: use libpcre2-dev and libwxgtk3.0-dev instead of libpcre3-dev and libwxgtk3.0-gtk3-dev

#### Clone and Build
 * Clone current master
```sh
git clone https://github.com/elfmz/far2l
cd far2l
```
 * Checkout some stable release tag (master considered unstable): `git checkout v_2.#.#`
 * Prepare build directory:
```sh
mkdir -p _build
cd _build
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

 * Also it's possible to create far2l_2.X.X_ARCH.deb or ...tar.gz packages in `_build` directory by running `cmake --build . --target package` command.

##### Additional build configuration options:

To build without WX backend (console version only): change `-DUSEWX=yes` to `-DUSEWX=no` also in this case dont need to install libwxgtk\*-dev package

To force-disable TTY|X and TTY|Xi backends: add argument `-DTTYX=no`; to disable only TTY|Xi - add argument `-DTTYXI=no`

To eliminate libuchardet requirement to reduce far2l dependencies by cost of losing automatic charset detection functionality: add `-DUSEUCD=no`

To build with Python plugin: add argument `-DPYTHON=yes`

To control how RAR archives will be handled in multiarc:
 `-DUNRAR=bundled` (default) use bundled sources found in multiarc/src/formats/rar/unrar
 `-DUNRAR=lib` use libunrar and unrar utility, also build requires libunrar-dev to be installed
 `-DUNRAR=NO` dont use special unrar code, rar archives will be handled by libarchive unless its also disabled

There're also options to toggle other plugins build in same way: ALIGN AUTOWRAP CALC COLORER COMPARE DRAWLINE EDITCASE EDITORCOMP FARFTP FILECASE INCSRCH INSIDE MULTIARC NETROCKS SIMPLEINDENT TMPPANEL

#### OSX/MacOS build

To make custom/recent build use brew or MacPorts.

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
Some issues can be caused by conflicting dependencies, like having two versions of wxWidgets, so avoid such situation when installing dependencies.

 * Clone:
```sh
git clone https://github.com/elfmz/far2l
cd far2l
```
 * Install needed dependencies with MacPorts:
``` sh
sudo port install cmake pkgconfig wxWidgets-3.2 libssh openssl libxml2 libfmt uchardet neon
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

##### macOS workaround if far2l in macOS regularly asks permission to folders
After command
```
 sudo codesign --force --deep --sign - /Applications/far2l.app
```
it is enough to confirm permission only once.

Details see in [`issue`](https://github.com/elfmz/far2l/issues/1754).

#### Building on Gentoo (and derivatives)
For absolute minimum you need:
```
emerge -avn dev-libs/libxml2 app-i18n/uchardet dev-util/cmake
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

To install system-wide, add the `far2l` package to your [`configuration.nix`](https://nixos.org/manual/nixos/stable/index.html#sec-changing-config) `environment.systemPackages` list. To run the application on-demand without affecting the system state, execute `nix-shell -p far2l --command far2l`. These use a package version from your current [channel](https://wiki.nixos.org/wiki/Nix_channels).

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
 * **Visual Studio Code** (required _CMake Tools extension_): open far2l root directory (by default building in subdirectory `_build`; you can change in `.vscode/settings.json`)

<a name="terminals"></a>
### Terminals and SSH clients
Supporting extended far2l keyboard shortcuts and clipboard access

 * **kovidgoyal's kitty** (Linux, macOS, *BSD): https://github.com/kovidgoyal/kitty & https://sw.kovidgoyal.net/kitty (TTY|k backend: keys by kovidgoyal's kitty keyboard protocol; turn on OSC 52 in far2l and kitty for clipboard support)
 * **Wez's Terminal Emulator** (Linux, FreeBSD, Windows): https://github.com/wez/wezterm & https://wezfurlong.org/wezterm (TTY|k backend: keys in Linux, FreeBSD by kovidgoyal's kitty keyboard protocol; TTY|w backend: keys in Windows by win32-input-mode, enabled by default; turn on OSC 52 for clipboard support) [kitty keyboard protocol not supported in macOS & Windows]
 * **iTerm2** (macOS): https://gitlab.com/gnachman/iterm2 & https://iterm2.com (TTY|a backend: keys by iTerm2 "raw keyboard" protocol; turn on OSC 52 for clipboard support)
 * **Windows Terminal** (TTY|w backend: keys by win32-input-mode; turn on OSC 52 for clipboard support; has mouse bug: https://github.com/microsoft/terminal/issues/15083 )

 * _Original Putty_ does _not correctly send some keyboard shortcuts_. Please use putty forks with _special far2l TTY extensions support (fluent keypresses, clipboard sharing etc)_:
   * **putty4far2l** (Windows ssh-client): https://github.com/ivanshatsky/putty4far2l/releases & https://github.com/unxed/putty4far2l (TTY|F backend: keys and clipboard by FAR2L TTY extensions support)
   * **cyd01's KiTTY** (Windows ssh-client): https://github.com/cyd01/KiTTY & https://www.9bis.net/kitty (TTY|F backend: keys and clipboard by FAR2L TTY extensions support)
   * **putty-nd** (Windows ssh-client): https://sourceforge.net/projects/putty-nd & https://github.com/noodle1983/putty-nd (TTY|F backend: keys and clipboard by FAR2L TTY extensions support)

_Note_: to full transfer extended keyboard shortcuts and the clipboard to/from the **remote far2l**
one of the best way to initiate the connection **inside local far2l-GUI**
(see details in build-in help section **UI backends**).

### Useful 3rd-party extras

 * A collection of macros for far2l: https://github.com/corporateshark/far2l-macros
 * Turbo Vision, TUI framework supporting far2l terminal extensions: https://github.com/magiblot/tvision
 * turbo, text editor supporting far2l terminal extensions: https://github.com/magiblot/turbo
 * Tool to import color schemes from windows FAR manager 2 .reg format: https://github.com/unxed/far2ltricks/blob/main/misc/far2l_import.pl

 * **Community wiki & tips** (in Russian; unofficial): https://github.com/akruphi/far2l/wiki

<a name="community_bins"></a>
### Community packages & binaries

 _They are mainteined by enthusiasts and may be not exact with master: sometimes has extra plugins, sometimes has tweak, etc._

 * **Portable** (_with TTY X/Xi backend_) | **AppImage** (_with wx-GUI and some extra plugins_): https://github.com/spvkgn/far2l-portable/releases
 * **Ubuntu** and **Mint** from PPA with fresh far2l: https://launchpad.net/~far2l-team/+archive/ubuntu/ppa
 * **Fedora** and **CentOS**: https://copr.fedorainfracloud.org/coprs/polter/far2l
 * **OpenSUSE**, **Fedora**, **Ubuntu**: https://download.opensuse.org/repositories/home:/viklequick/
 * **OpenWrt**: https://github.com/spvkgn/far2l-openwrt
 * **Termux**: https://github.com/spvkgn/far2l-termux
 * **Flatpak**: https://github.com/spvkgn/far2l-flatpak (access only to part of real filesystem via sandbox)

 See also in https://github.com/elfmz/far2l/issues/647

## Notes on porting and FAR Plugin API changes
 * See [HACKING.md](HACKING.md)

## Testing
 * See [testing/README.md](testing/README.md)

## Known issues:
* Only valid translations are English, Russian, Ukrainian and Belarussian (interface only), all other languages require deep correction.
