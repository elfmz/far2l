# Colorer library

Colorer is a syntax highlighting library.
[![build](https://github.com/colorer/Colorer-library/actions/workflows/colorer_ci.yml/badge.svg)](https://github.com/colorer/Colorer-library/actions/workflows/colorer_ci.yml)

## How to build from source

### Main

To build library and other utils from source, you will need:

* Visual Studio 2019 or higher / gcc 8 or higher
* git
* cmake 3.10 or higher

### Windows

Download the source of Colorer, for example, in colorer-library:

```bash
git clone https://github.com/colorer/Colorer-library.git --recursive colorer-library 
```

Setup vcpkg

```bash
cd colorer-library
./external/vcpkg/bootstrap-vcpkg.bat
```

Build colorer and dependency, if they are not in the local cache:

```bash
mkdir build
cd build
cmake -S .. -G "Visual Studio 16 2019" -DCMAKE_TOOLCHAIN_FILE=../external/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static -DVCPKG_OVERLAY_PORTS=../external/vcpkg-ports -DVCPKG_FEATURE_FLAGS=manifests,versions
colorer.sln
```

For x86 platform use `--triplet=x86-windows-static`. Once built, the dependencies will be cached in the local cache.

### Linux

You may build library on linux using standard package, without vcpkg.

#### Ubuntu example

```bash
sudo apt install libicu-dev libxerces-c-dev libspdlog-dev libfmt-dev zlib1g-dev libminizip-dev
git clone https://github.com/colorer/Colorer-library.git
cd Colorer-library
mkdir build
cd build
cmake .. -DCOLORER_USE_VCPKG=OFF
cmake --build .
```

#### CentOS Example

```bash
sudo yum install libicu-devel xerces-c-devel spdlog-devel fmt-devel zlib-devel minizip1.2-devel
git clone https://github.com/colorer/Colorer-library.git
cd Colorer-library
mkdir build
cd build
cmake .. -DCOLORER_USE_VCPKG=OFF
cmake --build .
```

### Options for build

This options available for build

* `COLORER_USE_VCPKG` - Use dependencies installed via vcpkg. Default 'ON'.
* `COLORER_BUILD_ARCH` - Build architecture. Default 'x64'.
* `COLORER_BUILD_TOOLS` - Build colorer tools. Default 'ON'.
* `COLORER_BUILD_TEST` - Build tests. Default 'OFF'.
* `COLORER_BUILD_INSTALL` - Make targets for install. Default 'ON'.
* `COLORER_USE_ZIPINPUTSOURCE` - Enable the ability to work with schemes in zip archives. Default 'ON'.
* `COLORER_USE_DUMMY_LOGGER` - Use dummy logging. Default 'OFF'.
* `COLORER_USE_DEEPTRACE` - Use trace logging. Default 'OFF'.
* `COLORER_USE_ICU_STRINGS` - Use ICU library for strings. Default 'ON'.

Links
========================

* Project main page (older): [http://colorer.sourceforge.net/](http://colorer.sourceforge.net/)
* Colorer github discussions: [https://github.com/colorer/Colorer-library/discussions](https://github.com/colorer/Colorer-library/discussions)
* Colorer discussions (in Russian): [http://groups.google.com/group/colorer_ru](http://groups.google.com/group/colorer_ru)
* Colorer discussions (in English): [http://groups.google.com/group/colorer](http://groups.google.com/group/colorer)
