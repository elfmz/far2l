# Changelog for Colorer base

## [Unreleased]

### Fixed
- [calcset] update hrc for latest file structure.
- [picasm] fix typo  sndwf -> andwf
- [python] numeric fixes, more strict, and fine-grained types/classes
- [python] completely rework string literals, adding bytes and fstring support
    * bytes literals and fstrings are now supported
    * added support for \x \u \U \N{name} escape sequences
    * triple-quoted literals are no longer comments
    * a lot of syntax errors are highlighted
    * fixed bunch of bugs, especially in raw literals
- [csharp] recognise functions with nullable results
- [shell-posix] Comments are not recognized in case statement
- [shell-posix] Commands are not recognized correctly after escaped new-line
- [shell-bash] Redirection symbols < and > inside "magic backticks" block break background highlighting till the end of the file
- [shell-bash] Fix magic backticks in for loop
- [smarty] fixed the work of smarty templates
- [markdown] amend emphasis with underscores
- [markdown] fix trailing spaces in em and strong

### Changed
- Simplified catalog.xml.
- Use new xsd schema for catalog.xml.
- Common.jar rename to common.zip
- reformat proto.hrc and included files; changed namespace.
- Tweak visual.hrd xml CData markup
- remove obsolete from visual-rgb.hrd
- [groovy] add .gradle as groovy
- Use the new shell-posix schema for shell scripts by default

### Added
- New package type of base - all packed. Hrc and hrd files in one archive. Directory 'auto' not in archive.
- [regex] support named capture groups and backreferences, like `(?<name>bar) \k<name>` and `(?'name'bar) \k'name'`
- [fsharp] support Unicode names
- [powershell] add split to regex operators
- [powershell] add missing automatic variables
- [farmenu] Color commands with prefixes lua: using lua scheme, ps: and vps: using powershell scheme.
- [python] missing py3 stuff like operators, keywords, and magic names
- [cpp] all the keywords from the https://en.cppreference.com/w/cpp/keyword
- [qmake] add all identifiers for Qt 5.15
- [csharp] support record, the new type keyword
- [csharp] support C# 11 raw strings
- [cpp] add support for C++11 string literals
- [asm] add new registers, blocks. New masks for filenames and firstline
- [shell-posix] add a new schema for POSIX shell with block structures and error checking
- [shell-bash] add a new schema for bash script based on shell-posix
- [asm] add directives
- [c] add unix standard functions, const and others
- [cpp] add some c++11 types
- [csharp] add new number suffixes u, uy, etc.
- [csharp] add new operators ?:, ??, etc.
- [csharp] retire obsolete cmdlets
- [cobol] many changes
- [cobolfr] many changes
- [jcl] many changes
- [pl1] many changes
- [cmake] updated for cmake version 3.29.2; add gen script for cmake
- [smarty] add tpl extension and first line pattern
- [prolog] add first line pattern
- [shell-posix] add functions to outlined list
- [shell-bash] add functions to outlined list
- [markdown] added the ability to connect the backlight in 'code' by creating your own scheme 'markdown2:markdown2'
- [markdown] highlight special all caps HTML tags

## [1.2.0] - 2021-09-12

### Fixed
- [awk] solve the issue with $( ... ).
- [csharp] Fix csharp.hrc parsing of char literals.

### Changed
- [csharp] Add .csx extension to csharp schema in proto.hrc. CSharp scripts usually have .csx extension.

### Added
- [diff] Add support for git inline diff (aka --word-diff).

## [1.1.0] - 2021-05-07

### Fixed
- [nix] Fixed escape ~ in regexp for home dir.

### Changed
- [GraphQL] add outlined to input, enum, union names.

## [1.0.0] - 2021-03-21

### Added
- First SemVer release after a long time. Previous history look in [old changelog](https://github.com/colorer/Colorer-schemes/blob/0ce9aa4ecf2fda04b959a7a74fd965247d8f65f8/hrc/hrc/CHANGELOG)

