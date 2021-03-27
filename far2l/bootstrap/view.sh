#!/bin/bash
# This script used by Viewer to produce F5-toggled 'Processed view' content.
# It gets input file as 1st argument, tries to analyze what is it and should
# write any filetype-specific information into output file, given in 2nd argument.
# Input: $1
# Output: $2

FILE="$(file "$1")"

# Optional per-user script
if [ -x ~/.config/far2l/view.sh ]; then
. ~/.config/far2l/view.sh
fi

echo "$FILE" > "$2"
echo >> "$2"

if [[ "$FILE" == *" archive data, "* ]] \
		|| [[ "$FILE" == *" compressed data"* ]]; then
	if command -v 7z >/dev/null 2>&1; then
		7z l "$1" >>"$2" 2>&1
	else
		echo "Install <p7zip-full> to see information" >>"$2" 2>&1
	fi
	exit 0
fi

if [[ "$FILE" == *ELF*executable* ]] || [[ "$FILE" == *ELF*object* ]]; then
	if command -v readelf >/dev/null 2>&1; then
		readelf -n --version-info --dyn-syms "$1" >>"$2" 2>&1
	else
		echo "Install <readelf> utility to see more information" >>"$2" 2>&1
	fi
	exit 0
fi

if [[ "$FILE" == *"JPEG image"* ]]; then
	if command -v chafa >/dev/null 2>&1; then
		chafa -c 16 --color-space=din99d -w 9 --symbols all --fill all "$1" && read -n1 -r -p "" >>"$2" 2>&1
		exit 0
	else
		echo "Install <chafa> to see picture" >>"$2" 2>&1
	fi
	if command -v jp2a >/dev/null 2>&1; then
		jp2a --colors "$1" >>"$2" 2>&1
		exit 0
	else
		echo "Install <jp2a> to see colored picture" >>"$2" 2>&1
	fi
fi

if [[ "$FILE" == *" image data, "* ]]; then
	if command -v chafa >/dev/null 2>&1; then
		chafa -c 16 --color-space=din99d -w 9 --symbols all --fill all "$1" && read -n1 -r -p "" >>"$2" 2>&1
		exit 0
	else
		echo "Install <chafa> to see picture" >>"$2" 2>&1
	fi
	if command -v asciiart >/dev/null 2>&1; then
		asciiart -c "$1" >>"$2" 2>&1
	else
		echo "Install <asciiart> to see picture" >>"$2" 2>&1
	fi
	exit 0
fi

if [[ "$FILE" == *": Audio file"* ]]; then
	if command -v exiftool >/dev/null 2>&1; then
		exiftool "$1" >>"$2" 2>&1
	else
		echo "Install <exiftool> to see picture" >>"$2" 2>&1
	fi
	exit 0
fi

if [[ "$FILE" == *": HTML document"* ]]; then
	if command -v pandoc >/dev/null 2>&1; then
		pandoc -f html -t markdown "$1" >>"$2" 2>&1
	else
		echo "Install <pandoc> to see document" >>"$2" 2>&1
	fi
	exit 0
fi

if [[ "$FILE" == *": OpenDocument Text"* ]]; then
	if command -v pandoc >/dev/null 2>&1; then
		pandoc -f odt -t markdown "$1" >>"$2" 2>&1
	else
		echo "Install <pandoc> to see document" >>"$2" 2>&1
	fi
	exit 0
fi

if [[ "$FILE" == *": EPUB document"* ]]; then
	if command -v pandoc >/dev/null 2>&1; then
		pandoc -f epub -t markdown "$1" >>"$2" 2>&1
	else
		echo "Install <pandoc> to see document" >>"$2" 2>&1
	fi
	exit 0
fi

# if [[ "$FILE" == *" FictionBook2 ebook"* ]]; then
if [[ "$FILE" == *": XML 1.0 document, UTF-8 Unicode text, with very long lines"* ]]; then
	if command -v pandoc >/dev/null 2>&1; then
		pandoc -f fb2 -t markdown "$1" >>"$2" 2>&1
	else
		echo "Install <pandoc> to see document" >>"$2" 2>&1
	fi
	exit 0
fi

if [[ "$FILE" == *": Microsoft Word 2007+"* ]]; then
	if command -v pandoc >/dev/null 2>&1; then
		pandoc "$1" >>"$2" 2>&1
	else
		echo "Install <pandoc> to see picture" >>"$2" 2>&1
	fi
	exit 0
fi

if [[ "$FILE" == *": Composite Document File"*"Microsoft Office Word"* ]]; then
	if command -v catdoc >/dev/null 2>&1; then
		catdoc "$1" >>"$2" 2>&1
	else
		echo "Install <catdoc> to see picture" >>"$2" 2>&1
	fi
	exit 0
fi

if [[ "$FILE" == *": Composite Document File"*"Microsoft PowerPoint"* ]]; then
	if command -v catppt >/dev/null 2>&1; then
		catppt "$1" >>"$2" 2>&1
	else
		echo "Install <catppt> to see picture" >>"$2" 2>&1
	fi
	exit 0
fi

if [[ "$FILE" == *": PDF document"* ]]; then
	if command -v pdftotext >/dev/null 2>&1; then
		pdftotext -enc UTF-8 "$1" "$2" 2>>"$2"
	else
		echo "Install <pdftotext> to see picture" >>"$2" 2>&1
	fi
	exit 0
fi

if [[ "$FILE" == *": unified diff output"* ]]; then
	if command -v colordiff >/dev/null 2>&1; then
		cat "$1" | colordiff --color=yes >>"$2" 2>&1
	else
		echo "Install <colordiff> to see colored diff" >>"$2" 2>&1
	fi
	exit 0
fi

if [[ "$FILE" == *": "*" source, "*" text"* ]]; then
	if command -v ctags >/dev/null 2>&1; then
		ctags --totals -x -u "$1" >>"$2" 2>&1
	else
		echo "Install <ctags> to see source overview" >>"$2" 2>&1
	fi
	exit 0
fi

if [[ "$FILE" == *": ASCII text"* ]] \
		|| [[ "$FILE" == *": UTF-8 Unicode"* ]]; then
	head "$1" >>"$2" 2>&1
	echo ............  >>"$2" 2>&1
	tail "$1" >>"$2" 2>&1
	exit 0
fi

echo "Hint: use <F5> to switch back to raw file viewer" >>"$2" 2>&1
#exit 1
