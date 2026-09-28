# Changelog for Colorer base

## [Unreleased]

## [1.3.0] - 2026-09-05

### Added
- [julia] Julia (`.jl`)
- [gitattributes] Git attributes (`.gitattributes`)
- [typst] Typst (`.typ`)
- [gleam] Gleam (`.gleam`)
- [solidity] Solidity (`.sol`)
- [luau] Luau (`.luau`) on top of Lua
- [blade] Laravel Blade (`.blade.php`) on top of PHP
- [gdscript] GDScript (`.gd`)
- [elixir] Elixir (`.ex`, `.exs`)
- [zig] Zig language (`.zig`)
- [csv] CSV / TSV (`.csv`, `.tsv`)
- [mdx] MDX (`.mdx`): Markdown with ESM import/export
- [astro] Astro components (`.astro`) with JS front matter; weight 3 so diff does not steal `---`
- [protobuf] Protocol Buffers schema (`.proto`)
- [dotenv] dotenv / env files (`.env`, `.env.*`)
- [jsx] JSX/TSX highlighting (`.jsx`, `.tsx`) on top of JavaScript
- [scss] SCSS highlighting (`.scss`); indented Sass stays on `.sass`
- [ipynb] Jupyter Notebook (`.ipynb`) as JSON, so Pascal firstline does not steal it
- [rmd] R Markdown / Quarto (`.Rmd`, `.qmd`) with YAML front matter and R/Python chunks
- [python] recognise `.pyi` type stubs
- [kotlin] recognise `.kts` / `.gradle.kts` scripts
- [prisma] Prisma schema (`.prisma`), so C firstline does not steal it
- [meson] Meson build (`meson.build`, `meson.options`)
- [vue] Vue single-file components (`.vue`)
- [svelte] Svelte components (`.svelte`)
- [razor] Razor / Blazor (`.razor`, `.cshtml`) with `@code` C# islands
- [cpp] recognise C++20 modules and extra headers (`.ixx`, `.cppm`, `.hh`, `.hxx`, `.ipp`, `.tpp`, `.inl`)
- [cython] Cython highlighting (`.pyx`, `.pxd`, `.pxi`) on top of Python
- [rproj] RStudio project files (`.Rproj`)
- [typescript] TypeScript highlighting (`.ts`, `.mts`, `.cts`) on top of JavaScript
- [toml] Add TOML syntax highlighting support
- [dart] Add Dart syntax highlighting support
- [swift] Add Swift syntax highlighting support
- [go.mod] Add go.mod file syntax highlighting support
- [go.sum] Add go.sum file syntax highlighting support
- [gitignore] Add .gitignore file syntax highlighting support
- [moonscript] Add moonscript file syntax highlighting support

### Changed
- [markdown] highlight common languages in fenced code blocks (indented, no blank line; same coloring in lists); markdown2 stays for extra languages
- [terraform] recognise `.tofu`
- [graphql] recognise `.graphqls` schema files
- [typescript] `accessor` auto-accessor fields
- [css] color functions `oklch()` / `oklab()` / `color-mix()`
- [config] pair `[]` around INI/EditorConfig section names
- [json] recognise `jsconfig.json` / `tsconfig*.json`
- [gitignore] recognise `.gitignore_global`
- [sql] PostgreSQL dollar-quotes (`$$` / `$tag$`), `$1` parameters, and `COPY FROM stdin` dumps
- [yaml] quoted keys (`"on":`)
- [python] PEP 750 t-strings (`t"…"`, `tr"…"`)
- [xml] recognise `.slnx` Visual Studio solutions
- [go] type-set operator `~` in interface constraints
- [csharp] collection-expression spread `..` before member `.`
- [java] unnamed `_` and `module-info` keywords (`module`/`requires`/`exports`/…)
- [markdown] GFM tables (`|`) and task lists (`[ ]` / `[x]`)
- [dockerfile] parser directives (`# syntax=`, `# escape=`) before comments
- [vue] `v-*` / `@` / `:` template directives and `<script lang="ts">`
- [rust] raw strings `r#"…"#` / `r##"…"##`, `#![` crate attributes, and lifetimes
- [csv] colour TSV tab separators (tabs are skipped unless attached to a field)
- [go] `//go:` compiler directives before line comments
- [python] PEP 695 `type` alias as a keyword
- [yaml] GitHub Actions `${{ }}` expressions
- [css] `@layer`/`@container`/`@scope`, `:has()`/`:is()`/`:where()`, `&` nesting, and nested rules inside declarations
- [php] PHP 8 `match`/`enum`/`readonly`/`fn`/`never`, `=>`, and `#[...]` attributes before `#` comments
- [jScript] ES2015–2024 operators (`?.`, `??`, `=>`, `...`, `**`, logical assignment), regexp flags `s/u/y/d/v`, `#private`, and `using`
- [json] recognise `.jsonc`; list JSON, YAML, TOML and Jupyter under scripts instead of rare
- [gitignore] recognise `.dockerignore`, `.prettierignore`, `.eslintignore`, `.npmignore`
- [dockerfile] recognise `Containerfile` and `.dockerfile`
- [python] add `match`/`case`; treat `print`/`exec` as builtins, not keywords
- [rust] add `async`/`await`/`dyn`; drop obsolete reserved words (`alignof`, `proc`, …)
- [java] add `record`/`sealed`/`permits`/`var`/`yield`/`when` and text blocks
- [csharp] add `required`/`init`/`file`/`scoped`/`nint`/`nuint`
- [jScript] add `async`/`await`/`of`; do not treat `boolean` as an ES3 reserved word
- [vbnet] add `Async`/`Await`/`Iterator`/`Yield`/`NameOf`
- [toml] recognise `Cargo.lock`, `poetry.lock`, `uv.lock`
- [r] native pipe `|>` and `\(x)` lambda
- [mysql] bind `.mysql` and mysqldump firstline; `.sql` stays on sql
- [jsx] TypeScript keywords in TSX via shared `tsKeywords`
- [json] speed up number and string-escape matching
- [qml] speed up import and property matching
- [gitignore] speed up comment, negation and escape matching
- [swift] speed up directives; colour Swift number literals (underscores, `0o`, hex-float) without inheriting `def:Number`
- [vbasic] speed up strings; recognise `""` inside a string
- [foxpro] speed up strings and `{|...|}` inserts; recognise doubled quotes
- [ole] speed up strings and `{|...|}` inserts; recognise doubled quotes
- [css] add @media rules and media query highlighting
- License has been changed to LGPL 2.1 for the resulting hrc files with the specified license 'MPL 1.1/GPL 2.0/LGPL 2.1'.
- Simplified catalog.xml.
- Use new xsd schema for catalog.xml.
- Common.jar rename to common.zip
- reformat proto.hrc and included files; changed namespace.
- Tweak visual.hrd xml CData markup
- remove obsolete from visual-rgb.hrd
- [groovy] add .gradle as groovy
- Use the new shell-posix schema for shell scripts by default
- [json] highlight JSON object keys
- rename lib/default.hrc to lib/def.hrc
- move 'default' type implementation from proto.hrc to base/default.hrc
- [shell-posix] allow to include shell-posix scheme as subscheme enclosed in quotes
- Add *.xaml to xml prototype
- Add *.lpr to pascal prototype
- Add *.lfm to delphiform prototype

### Fixed
- [farmenu] do not treat `!` inside parentheses as a `!?...!` dialog field terminator
- [rust] highlight matching `()`, `[]` and `{}` when the cursor is on a bracket
- [batch] cmd does not treat backslash as a quote escape; mark double quotes as pairs
- [makefile] recognise GNU Make `else ifeq`/`else ifdef` conditionals and `export`/`unexport` without assignment
- [perl] recognise regexp after range operator (`/'/ .. /"/`)
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
- [shell-bash] Fix array extension in for loop
- [smarty] fixed the work of smarty templates
- [markdown] amend emphasis with underscores
- [markdown] fix trailing spaces in em and strong
- [smarty] fixed working with nested brackets
- [smarty] literal block - text only
- [shell-posix] fix variable assignments with line continuations
- [shell-bash] fix indirect expansion+transformation
- [shell-posix] fix unhighlighted areas after updating colorer from 1.3.3 to 1.4.0
- [shell-bash] fix unhighlighted areas after updating colorer from 1.3.3 to 1.4.0
- [dockerfile] fix unhighlighted areas after updating colorer from 1.3.3 to 1.4.0
- [blue.hrd] fix colors for cross
- [json] fix comments in json
- [shell-posix] recognize line continuation after "while; do...done" / "for; do...done" blocks
- [shell-bash] recornize bash-specific syntax in for loops, recornize an append operator
- [black.hrd] fix colors for cross
- [yml] fix error in generated scheme; update scheme to 2.4.0
- [verilog] add define, include, timescale support
- [json] object keys use string edges/escapes and Keyword coloring (visible in default.hrd)
- [verilog] highlight function-like compiler macros (`name(...)) and pair module/endmodule when the name is a macro; recognise .vh headers
- [cmake] fix escaping strings
- [cpp] functions outliner list cleanup
- [markdown] improve HTML handling (inline tags and CommonMark blocks)
- [markdown] highlight URL and title in link reference definitions
- [shell-posix] fix heredoc with '<<-' operator

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
- [hcl] add a new schema for HashiCorp HCL
- [terraform] add a new schema for HashiCorp Terraform
- [dockerfile] add a new schema for Dockerfile
- [jenkinsfile] add a new schema for Jenkins configuration (Jenkinsfile)
- [markdown] add Obsidian Templater blocks
- [pascal] add 'object' to highlight
- [pascal] add some types
- [pascal] highlight noAscii symbols
- [pascal] add abstract and sealed for object; type NativeInt, NativeUint
- [c] add format string length modifiers since C99
- [navy-mirror.hrd] add colors for cross
- [farmenu] Add optional @ before Lua and PowerShellFar prefixes
- [farmenu] Support luas: in addition to lua:
- [powershell] Add to outlined: #requires and top level process|begin|end|dynamicparam|clean blocks.
- [AviSynth] Add block comments support
- [go-template] add a new schema for Go template
- [go-template-sprig] add a new schema for Go template with Sprig functions
- [helm-tpl] add a new schema for Helm templates
- [helm-text] add a new schema for text files used in Helm Charts
- [hrc] add new attribute global for packages
- [hrd] add attributes for hrd element
- [far2l.hrd] add rgb style from far2l
- [hrd] add new rgb style "Violet by Kate tempergate"

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

