#!/bin/bash
# input: $1
# output: $2

if [ -x ~/.config/far2l/view.sh ]; then
. ~/.config/far2l/view.sh
fi

FILE="$(file "$1")"

echo "$FILE" > "$2"

if [[ "$FILE" == *ELF*executable* ]] || [[ "$FILE" == *ELF*object* ]]; then
	if command -v readelf >/dev/null 2>&1; then
		readelf -n --version-info --dyn-syms "$1" >>"$2" 2>&1
	else
		echo "Install <readelf> utility to see more information" >>"$2" 2>&1
	fi
	exit 0
fi

if [[ "$FILE" == *"JPEG image"* ]]; then
	if command -v jp2a >/dev/null 2>&1; then
		jp2a --colors "$1" >>"$2" 2>&1
		exit 0
	else
		echo "Install <jp2a> to see colored picture" >>"$2" 2>&1
	fi
fi

if [[ "$FILE" == *" image data, "* ]]; then
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

echo "Hint: use <F9> to switch to raw file viewer" >>"$2" 2>&1
#exit 1
