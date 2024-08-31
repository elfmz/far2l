#!/bin/bash
# This script used by Viewer to produce F5-toggled 'Processed view' content.
# It gets input file as 1st argument, tries to analyze what is it and should
# write any filetype-specific information into output file, given in 2nd argument.
# Input: $1
# Output: $2

if [ $# != 2 -o ."$1" = ."" -o ."$2" = ."" ]; then
	echo "Two non-empty parameters are expected" >&2
	echo "Usage: view.sh path-to-an-existing-file-for-analysis path-to-the-newly-created-results-file" >&2
	echo >&2
	exit 1
elif [ ! -e "$1" -o -d "$1" ]; then
	echo "File for analysis does not exist - [ "$1" ] " >&2
	echo >&2
	exit 1
elif [ -e "$2" ]; then
	echo "A file for storing results already exists - [ "$2" ]" >&2
	echo >&2
	exit 1
elif [ -z $(type -p file) ]; then
	echo "Install <file> to see information" >&2
	exit 1
fi

FILE="$(echo -n ': ' ; file --brief -- "$1")"

FILEMIME="$(echo -n ': ' ; file --brief --mime -- "$1")"

FILECHARSET="$(echo "$FILEMIME" | sed -n -e 's/^.\{0,100\}: .\{1,100\};[ ]\{0,10\}charset=\([a-zA-Z0-9\-]\{1,20\}\).\{0,30\}$/\1/p')"

# Optional per-user script
if [ -x ~/.config/far2l/view.sh ]; then
	# safely source this script from user config dir if it copied there without changes
	export far2l_view_per_user_script_sourced=$((${far2l_view_per_user_script_sourced:-0}+0))
	if [ ${far2l_view_per_user_script_sourced} -lt 1 ]; then
		export far2l_view_per_user_script_sourced=$((${far2l_view_per_user_script_sourced}+1))
		if [ ${far2l_view_per_user_script_sourced} -eq 1 ]; then
			. ~/.config/far2l/view.sh
		fi
	fi
fi

echo "$1" > "$2"
echo "" >>"$2" 2>&1

echo "$FILE" >> "$2"
echo "$FILEMIME" >> "$2"

FILEMIMEALT=""

if command -v exiftool >/dev/null 2>&1; then
	FILEMIMEALT="$(exiftool -- "$1" | head -n 90 | head -c 2048 | grep -v -e '^File ' | grep -e 'Content Type' | sed -n -e 's/^.\{0,100\}: \(.\{1,100\}\);[ ]\{0,10\}charset=\([a-zA-Z0-9\-]\{1,20\}\).\{0,30\}$/\1; charset=\2/p')"
fi

echo ": $FILEMIMEALT" >> "$2"

echo ": $FILECHARSET" >> "$2"

FILECHARSETALT=""

if command -v exiftool >/dev/null 2>&1; then
	FILECHARSETALT="$(exiftool -- "$1" | head -n 90 | head -c 2048 | grep -v -e '^File ' | grep -e 'charset=' | sed -n -e 's/^.\{0,100\}: .\{1,100\};[ ]\{0,10\}charset=\([a-zA-Z0-9\-]\{1,20\}\).\{0,30\}$/\1/p')"
fi

echo ": $FILECHARSETALT" >> "$2"

if [[ ."$FILECHARSET" == ."" ]] \
	|| [[ ."$FILECHARSET" == ."utf-8" ]] \
	|| [[ ."$FILECHARSET" == ."UTF-8" ]] \
	|| [[ ."$FILECHARSET" == ."binary" ]] \
	|| [[ ."$FILECHARSET" == ."unknown-8bit" ]]; then
	if [[ ! ."$FILECHARSETALT" == ."" ]] \
		&& [[ ! ."$FILECHARSETALT" == ."utf-8" ]] \
		&& [[ ! ."$FILECHARSETALT" == ."UTF-8" ]]; then
		echo "" >>"$2" 2>&1
		echo "Switching charset from [ "$FILECHARSET" ] to [ "$FILECHARSETALT" ]" >>"$2" 2>&1
		FILECHARSET=""$FILECHARSETALT""
	fi
else
	if [[ ! ."$FILECHARSETALT" == ."" ]] \
		&& [[ ! ."$FILECHARSETALT" == ."utf-8" ]] \
		&& [[ ! ."$FILECHARSETALT" == ."UTF-8" ]] \
		&& ! [[ ."$FILECHARSET" == ."$FILECHARSETALT" ]]; then
		echo "" >>"$2" 2>&1
		echo "Switching charset from [ "$FILECHARSET" ] to [ "$FILECHARSETALT" ]" >>"$2" 2>&1
		FILECHARSET=""$FILECHARSETALT""
	fi
fi

echo "" >>"$2" 2>&1
echo "------------" >>"$2" 2>&1

if [[ "$FILE" == *": "*"archive"* ]] \
		|| [[ "$FILE" == *": "*"compressed data"* ]] \
		|| [[ "$FILE" == *": Debian "*" package"* ]] \
		|| [[ "$FILE" == *": RPM"* ]] \
		|| [[ "$FILE" == *": "*"MSI Installer"* ]] \
		|| [[ "$FILE" == *": "*"WIM"* ]] \
		|| [[ "$FILE" == *": "*"ISO 9660"* ]] \
		|| [[ "$FILE" == *": "*"filesystem"* ]] \
		|| [[ "$FILE" == *": "*"extension"* ]] \
		|| [[ "$FILE" == *": "*"gzip"* ]] \
		|| [[ "$FILE" == *": "*"bzip2"* ]] \
		|| [[ "$FILE" == *": "*"XZ"* ]] \
		|| [[ "$FILE" == *": "*"lzma"* ]] \
		|| [[ "$FILE" == *": "*"lzop"* ]] \
		|| [[ "$FILE" == *": "*"zstd"* ]]; then
	if command -v exiftool >/dev/null 2>&1; then
		exiftool -- "$1" | head -n 90 | head -c 2048 >>"$2" 2>&1
		echo "" >>"$2" 2>&1
	else
		echo "Install <exiftool> to see information" >>"$2" 2>&1
	fi
	echo "------------" >>"$2" 2>&1
	echo "Processing file as archive with 7z contents listing" >>"$2" 2>&1
	echo "" >>"$2" 2>&1
	echo "----bof----" >>"$2" 2>&1
	if command -v 7zz >/dev/null 2>&1; then
		7zz l -- "$1" >>"$2" 2>&1
		echo "" >>"$2" 2>&1
	elif command -v 7z >/dev/null 2>&1; then
		7z l -- "$1" >>"$2" 2>&1
		echo "" >>"$2" 2>&1
	else
		echo "Install <7zip> or <p7zip-full> to see information" >>"$2" 2>&1
	fi
	if [[ "$FILE" == *": Debian "*" package"* ]]; then
		echo "------------" >>"$2" 2>&1
		echo "Processing file as DEB package" >>"$2" 2>&1
		echo "" >>"$2" 2>&1
		echo "------------" >>"$2" 2>&1
		if command -v dpkg >/dev/null 2>&1; then
			dpkg --info -- "$1" >>"$2" 2>&1
			echo "------------" >>"$2" 2>&1
			echo "Listing DEB package contents" >>"$2" 2>&1
			echo "" >>"$2" 2>&1
			echo "------------" >>"$2" 2>&1
			dpkg --contents -- "$1" >>"$2" 2>&1
			echo "" >>"$2" 2>&1
		else
			echo "Install <dpkg> to see information" >>"$2" 2>&1
		fi
	fi
	if [[ "$FILE" == *": RPM"* ]]; then
		echo "------------" >>"$2" 2>&1
		echo "Processing file as RPM package" >>"$2" 2>&1
		echo "" >>"$2" 2>&1
		echo "------------" >>"$2" 2>&1
		if command -v rpm >/dev/null 2>&1; then
			rpm -q --info -p -- "$1" >>"$2" 2>&1
			echo "" >>"$2" 2>&1
			echo "------------" >>"$2" 2>&1
			echo "Listing RPM package contents" >>"$2" 2>&1
			echo "" >>"$2" 2>&1
			echo "------------" >>"$2" 2>&1
			rpm -q -v --list -p -- "$1" >>"$2" 2>&1
			echo "------------" >>"$2" 2>&1
			echo "Show RPM package (pre|post)[un]?install scripts" >>"$2" 2>&1
			echo "" >>"$2" 2>&1
			echo "------------" >>"$2" 2>&1
			rpm -q --scripts -p -- "$1" >>"$2" 2>&1
			echo "" >>"$2" 2>&1
		else
			echo "Install <rpm> to see information" >>"$2" 2>&1
		fi
	fi
	if [[ "$FILE" == *": "*"compressed data"* ]]; then
		echo "------------" >>"$2" 2>&1
		echo "Processing file as archive with tar contents listing" >>"$2" 2>&1
		echo "" >>"$2" 2>&1
		TAROPTS=""
		if [[ "$FILE" == *": gzip compressed data"* ]]; then
			TAROPTS=-z
		fi
		if [[ "$FILE" == *": bzip2 compressed data"* ]]; then
			TAROPTS=-j
		fi
		if [[ "$FILE" == *": XZ compressed data"* ]]; then
			TAROPTS=-J
		fi
		if [[ "$FILE" == *": lzma compressed data"* ]]; then
			TAROPTS=--lzma
		fi
		if [[ "$FILE" == *": lzop compressed data"* ]]; then
			TAROPTS=--lzop
		fi
		if [[ "$FILE" == *": zstd compressed data"* ]]; then
			TAROPTS=--zstd
		fi
		if [[ "$(tar --help | grep -e '--full-time' | wc -l)" == "1" ]]; then
			TAROPTS=$TAROPTS" --full-time"
		fi
		echo "TAROPTS=[ "$TAROPTS" ]" >>"$2" 2>&1
		echo "------------" >>"$2" 2>&1
		ELEMENTCOUNT=$( tar -tv $TAROPTS -f "$1" 2>/dev/null | wc -l )
		echo "tar archive elements count = "$ELEMENTCOUNT >>"$2" 2>&1
		if [[ $ELEMENTCOUNT -gt 0 ]]; then
			echo "------------" >>"$2" 2>&1
			tar -tv $TAROPTS -f "$1" >>"$2" 2>&1
			echo "------------" >>"$2" 2>&1
			tar -tv $TAROPTS -f "$1" | \
				tee >/dev/null \
				>( CTOTAL=$( wc -l ) ; ( echo $CTOTAL' total' ; echo "----done----" ; ) >>"$2" 2>&1 ) \
				>( CFOLDERS=$( grep -e '^d' | wc -l ) ; ( echo $CFOLDERS' folders' ) >>"$2" 2>&1 ) \
				>( CFILES=$( grep -v -e '^d' | wc -l ) ; ( echo $CFILES' files' ) >>"$2" 2>&1 )
		fi
	fi
	echo "----eof----" >>"$2" 2>&1
	exit 0
fi

if [[ "$FILE" == *": "*"ELF"*"executable"* ]] \
	|| [[ "$FILE" == *": "*"ELF"*"relocatable"* ]] \
	|| [[ "$FILE" == *": "*"ELF"*"bit"* ]] \
	|| [[ "$FILE" == *": "*"ELF"*"object"* ]]; then
	if command -v exiftool >/dev/null 2>&1; then
		exiftool -- "$1" | head -n 90 | head -c 2048 >>"$2" 2>&1
		echo "" >>"$2" 2>&1
	else
		echo "Install <exiftool> to see information" >>"$2" 2>&1
	fi
	echo "------------" >>"$2" 2>&1
	echo "Processing file as archive with 7z contents listing" >>"$2" 2>&1
	echo "" >>"$2" 2>&1
	echo "----bof----" >>"$2" 2>&1
	if command -v 7zz >/dev/null 2>&1; then
		7zz l -- "$1" >>"$2" 2>&1
		echo "" >>"$2" 2>&1
	elif command -v 7z >/dev/null 2>&1; then
		7z l -- "$1" >>"$2" 2>&1
		echo "" >>"$2" 2>&1
	else
		echo "Install <7zip> or <p7zip-full> to see information" >>"$2" 2>&1
	fi
	echo "------------" >>"$2" 2>&1
	echo "Processing file with ldd" >>"$2" 2>&1
	echo "" >>"$2" 2>&1
	echo "------------" >>"$2" 2>&1
	if command -v ldd >/dev/null 2>&1; then
		ldd -- "$1" >>"$2" 2>&1
	else
		echo "Install <ldd> utility to see more information" >>"$2" 2>&1
	fi
	echo "------------" >>"$2" 2>&1
	echo "Processing file with readelf" >>"$2" 2>&1
	echo "" >>"$2" 2>&1
	echo "------------" >>"$2" 2>&1
	if command -v readelf >/dev/null 2>&1; then
		readelf -n --version-info --dyn-syms -- "$1" >>"$2" 2>&1
	else
		echo "Install <readelf> utility to see more information" >>"$2" 2>&1
	fi
	echo "----eof----" >>"$2" 2>&1
	exit 0
fi

if [[ "$FILE" == *": "*"PE"*"executable"* ]] \
	|| [[ "$FILE" == *": "*"PE"*"object"* ]] \
	|| [[ "$FILE" == *": "*"DOS executable"* ]] \
	|| [[ "$FILE" == *": "*"boot"*"executable"* ]]; then
	if command -v exiftool >/dev/null 2>&1; then
		exiftool -- "$1" | head -n 90 | head -c 2048 >>"$2" 2>&1
		echo "" >>"$2" 2>&1
	else
		echo "Install <exiftool> to see information" >>"$2" 2>&1
	fi
	echo "------------" >>"$2" 2>&1
	echo "Processing file as archive with 7z contents listing" >>"$2" 2>&1
	echo "" >>"$2" 2>&1
	echo "----bof----" >>"$2" 2>&1
	if command -v 7zz >/dev/null 2>&1; then
		7zz l -- "$1" >>"$2" 2>&1
		echo "" >>"$2" 2>&1
		echo "" >>"$2" 2>&1
	elif command -v 7z >/dev/null 2>&1; then
		7z l -- "$1" >>"$2" 2>&1
		echo "" >>"$2" 2>&1
		echo "" >>"$2" 2>&1
	else
		echo "Install <7zip> or <p7zip-full> to see information" >>"$2" 2>&1
	fi
	echo "----eof----" >>"$2" 2>&1
	exit 0
fi

if [[ "$FILE" == *": "*"PGP"*"key public"* ]] \
	|| [[ "$FILE" == *": "*"GPG"*"key public"* ]]; then
	if command -v exiftool >/dev/null 2>&1; then
		exiftool -- "$1" | head -n 90 | head -c 2048 >>"$2" 2>&1
		echo "" >>"$2" 2>&1
	else
		echo "Install <exiftool> to see information" >>"$2" 2>&1
	fi
	echo "------------" >>"$2" 2>&1
	echo "Processing file as archive with gpg" >>"$2" 2>&1
	echo "" >>"$2" 2>&1
	echo "----bof----" >>"$2" 2>&1
	if command -v gpg >/dev/null 2>&1; then
		gpg --show-key -- "$1" >>"$2" 2>&1
	else
		echo "Install <gpg> to see information" >>"$2" 2>&1
	fi
	echo "----eof----" >>"$2" 2>&1
	exit 0
fi

if [[ "$FILE" == *": "*"image data, "* ]] \
	|| [[ "$FILE" == *": "*"SVG"*" image"* ]] \
	|| [[ "$FILE" == *": "*"JPEG"*" image"* ]] \
	|| [[ "$FILEMIME" == *": image/"* ]]; then
	# ??? workaround for bash to get values of variables
	bash -c "echo ${FOO}" >/dev/null 2>&1
	TCOLUMNS=$( bash -c "echo ${COLUMNS}" )
	TLINES=$( bash -c "echo ${LINES}" )
	TCOLUMNS=$(( ${TCOLUMNS:-80} - 0 ))
	TLINES=$(( ${TLINES:-25} - 2 ))
	TCOLORS="$(tput colors)" || TCOLORS=""
	VPRETTY="no"
	if command -v chafa >/dev/null 2>&1; then
		VPRETTY="yes"
		# chafa -c 16 --color-space=din99d --dither=ordered -w 9 --symbols all --fill all !.! && read -n1 -r -p -- "$1" >>"$2" 2>&1
		TCOLUMNS=$(( ${TCOLUMNS:-80} - 1 ))
		TCOLORMODE=""
		if [ ".${TCOLORS}" = ".2" ]; then
			TCOLORMODE="none"
		elif [ ".${TCOLORS}" = ".8" ]; then
			TCOLORMODE="16"
		elif [ ".${TCOLORS}" = ".16" ]; then
			TCOLORMODE="16"
		elif [ ".${TCOLORS}" = ".256" ]; then
			# recommended in chafa manual
			# TCOLORMODE="-c 240"
			# for new far2l terminal
			TCOLORMODE="full"
		fi
		VCHAFACOLOR=""
		if [ ! ".${TCOLORMODE}" = "." ]; then
			VCHAFACOLOR="-c "${TCOLORMODE}
		fi
		chafa -c none --symbols -all+stipple+braille+ascii+space+extra --size ${TCOLUMNS}x${TLINES} -f symbols -- "$1" >>"$2" 2>&1
		echo "Image is viewed by chafa in "${TCOLUMNS}"x"${TLINES}" symbols sized area, no colors" >>"$2" 2>&1
		clear
		chafa ${VCHAFACOLOR} --color-space=din99d -w 9 --symbols all --fill all -f symbols -- "$1" && \
			echo "Image is viewed by chafa in "${TCOLUMNS}"x"${TLINES}" symbols sized area, "${TCOLORMODE}" colors" && \
			read -n1 -r -p "" >>"$2" 2>&1
		clear

	elif command -v timg >/dev/null 2>&1; then
		VPRETTY="yes"
		timg -- "$1" >>"$2" 2>&1
		echo "Image is viewed by timg in "${TCOLUMNS}"x"${TLINES}" symbols sized area, no colors" >>"$2" 2>&1
		clear
		timg -- "$1" && read -n1 -r -p "" >>"$2" 2>&1
		clear

	else
		echo "Install <chafa> or <timg> to see pretty picture" >>"$2" 2>&1
	fi

	VJP2A="no"
	if [[ "$FILE" == *": "*"JPEG image"* ]] \
		&& [[ "$VPRETTY" == "no" ]]; then
		if command -v jp2a >/dev/null 2>&1; then
			VJP2A="yes"
			# jp2a --colors "$1" >>"$2" 2>&1
			# jp2a --size=${TCOLUMNS}x${TLINES} "$1" >>"$2" 2>&1
			# jp2a --height=${TLINES} "$1" >>"$2" 2>&1
			TCOLUMNS=$(( ${TCOLUMNS:-80} - 1 ))
			jp2a --width=${TCOLUMNS} "$1" >>"$2" 2>&1
			echo "Image is viewed by jp2a in "${TCOLUMNS}"x"${TLINES}" symbols sized area" >>"$2" 2>&1
			jp2a --colors --term-fit "$1" && read -n1 -r -p "" >>"$2" 2>&1
			clear
		else
			echo "Install <jp2a> to see colored picture" >>"$2" 2>&1
		fi
	fi
	VASCIIART="no"
	if [[ "$VPRETTY" == "no" ]] \
		&& [[ "$VJP2A" == "no" ]]; then
		if command -v asciiart >/dev/null 2>&1; then
			VASCIIART="yes"
			# asciiart -c -- "$1" >>"$2" 2>&1
			asciiart -- "$1" >>"$2" 2>&1
			echo "Image is viewed by asciiart in "${TCOLUMNS}"x"${TLINES}" symbols sized area" >>"$2" 2>&1
			asciiart --color -- "$1" && read -n1 -r -p "" >>"$2" 2>&1
			clear
		else
			echo "Install <asciiart> to see picture" >>"$2" 2>&1
		fi
	fi
	echo "------------" >>"$2" 2>&1
	if command -v exiftool >/dev/null 2>&1; then
		exiftool -- "$1" | head -n 90 | head -c 2048 >>"$2" 2>&1
		echo "" >>"$2" 2>&1
	else
		echo "Install <exiftool> to see information" >>"$2" 2>&1
	fi
	echo "----eof----" >>"$2" 2>&1
	exit 0
fi

if [[ "$FILE" == *": Audio file"* ]] \
	|| [[ "$FILE" == *": "*"MPEG"* ]] \
	|| [[ "$FILE" == *": Ogg data"* ]] \
	|| [[ "$FILEMIME" == *": audio/"* ]]; then
	if command -v exiftool >/dev/null 2>&1; then
		exiftool -- "$1" >>"$2" 2>&1
	else
		echo "Install <exiftool> to see information" >>"$2" 2>&1
	fi
	echo "----eof----" >>"$2" 2>&1
	exit 0
fi

if [[ "$FILE" == *": RIFF"*" data"* ]] \
	|| [[ "$FILE" == *": "*"ISO Media"* ]] \
	|| [[ "$FILE" == *": "*"Matroska data"* ]] \
	|| [[ "$FILE" == *": "*"MP4"* ]] \
	|| [[ "$FILEMIME" == *": video/"* ]]; then
	if command -v exiftool >/dev/null 2>&1; then
		exiftool -- "$1" >>"$2" 2>&1
	else
		echo "Install <exiftool> to see information" >>"$2" 2>&1
	fi
	echo "----eof----" >>"$2" 2>&1
	exit 0
fi

if [[ "$FILE" == *": BitTorrent file"* ]]; then
	if command -v exiftool >/dev/null 2>&1; then
		exiftool -- "$1" >>"$2" 2>&1
	else
		echo "Install <exiftool> to see information" >>"$2" 2>&1
	fi
	echo "----eof----" >>"$2" 2>&1
	exit 0
fi

if [[ "$FILE" == *": HTML document"* ]] \
	|| [[ "$FILEMIME" == *": text/xml;"*"charset="* ]] \
	|| [[ "$FILEMIMEALT" == *": text/xml;"*"charset="* ]] \
	|| [[ "$FILEMIME" == *": application/xhtml+xml;"*"charset="* ]] \
	|| [[ "$FILEMIMEALT" == *": application/xhtml+xml;"*"charset="* ]]; then
	if command -v exiftool >/dev/null 2>&1; then
		exiftool -- "$1" | head -n 90 | head -c 2048 >>"$2" 2>&1
		echo "" >>"$2" 2>&1
	else
		echo "Install <exiftool> to see information" >>"$2" 2>&1
	fi
	echo "------------" >>"$2" 2>&1
	echo "Processing file as html with pandoc ( formatted as markdown )" >>"$2" 2>&1
	echo "" >>"$2" 2>&1
	if [[ ."$FILECHARSET" == ."" ]] \
		|| [[ ."$FILECHARSET" == ."utf-8" ]] \
		|| [[ ."$FILECHARSET" == ."UTF-8" ]] \
		|| [[ ."$FILECHARSET" == ."binary" ]] \
		|| [[ ."$FILECHARSET" == ."unknown-8bit" ]]; then
		FROMENC=""
	else
		FROMENC="-f "$FILECHARSET" "
	fi
	if [[ ! ."$FROMENC" == ."" ]]; then
		if command -v iconv >/dev/null 2>&1; then
			echo "Using iconv to convert from [ "$FILECHARSET" ] to [ utf-8 ] for pandoc input" >>"$2" 2>&1
		else
			echo "Install <iconv> to convert from [ "$FILECHARSET" ] to [ utf-8 ] for pandoc input" >>"$2" 2>&1
		fi
	else
		echo "Will not convert from [ "$FILECHARSET" ] to [ utf-8 ] for pandoc input" >>"$2" 2>&1
	fi
	echo "" >>"$2" 2>&1
	echo "----bof----" >>"$2" 2>&1
	if command -v pandoc >/dev/null 2>&1; then
		if command -v iconv >/dev/null 2>&1; then
			iconv $FROMENC -t utf-8 -- "$1" | pandoc -f html -t markdown -- >>"$2" 2>&1
		else
			pandoc -f html -t markdown -- "$1" >>"$2" 2>&1
		fi
	else
		echo "Install <pandoc> to see document" >>"$2" 2>&1
	fi
	echo "----eof----" >>"$2" 2>&1
	FROMENC=
	exit 0
fi

if [[ "$FILE" == *": OpenDocument Text"* ]]; then
	if command -v exiftool >/dev/null 2>&1; then
		exiftool -- "$1" | head -n 90 | head -c 2048 >>"$2" 2>&1
		echo "" >>"$2" 2>&1
	else
		echo "Install <exiftool> to see information" >>"$2" 2>&1
	fi
	echo "------------" >>"$2" 2>&1
	echo "Processing file as odt with pandoc ( formatted as markdown )" >>"$2" 2>&1
	echo "" >>"$2" 2>&1
	echo "----bof----" >>"$2" 2>&1
	if command -v pandoc >/dev/null 2>&1; then
		pandoc -f odt -t markdown -- "$1" >>"$2" 2>&1
	else
		echo "Install <pandoc> to see document" >>"$2" 2>&1
	fi
	echo "----eof----" >>"$2" 2>&1
	exit 0
fi

if [[ "$FILE" == *": EPUB document"* ]]; then
	if command -v exiftool >/dev/null 2>&1; then
		exiftool -- "$1" | head -n 90 | head -c 2048 >>"$2" 2>&1
		echo "" >>"$2" 2>&1
	else
		echo "Install <exiftool> to see information" >>"$2" 2>&1
	fi
	echo "------------" >>"$2" 2>&1
	echo "Processing file as epub with pandoc ( formatted as markdown )" >>"$2" 2>&1
	echo "" >>"$2" 2>&1
	echo "----bof----" >>"$2" 2>&1
	if command -v pandoc >/dev/null 2>&1; then
		pandoc -f epub -t markdown -- "$1" >>"$2" 2>&1
	else
		echo "Install <pandoc> to see document" >>"$2" 2>&1
	fi
	echo "----eof----" >>"$2" 2>&1
	exit 0
fi

if [[ "$FILE" == *": DjVu"*"document"* ]]; then
	if command -v exiftool >/dev/null 2>&1; then
		exiftool -- "$1" | head -n 90 | head -c 2048 >>"$2" 2>&1
		echo "" >>"$2" 2>&1
	else
		echo "Install <exiftool> to see information" >>"$2" 2>&1
	fi
	echo "----eof----" >>"$2" 2>&1
	exit 0
fi

if [[ "$FILE" == *": Mobipocket E-book"* ]]; then
	if command -v exiftool >/dev/null 2>&1; then
		exiftool -- "$1" | head -n 90 | head -c 2048 >>"$2" 2>&1
		echo "" >>"$2" 2>&1
	else
		echo "Install <exiftool> to see information" >>"$2" 2>&1
	fi
	echo "----eof----" >>"$2" 2>&1
	exit 0
fi

# if [[ "$FILE" == *": "*"FictionBook2 ebook"* ]]; then
if [[ "$FILE" == *": XML 1.0 document, UTF-8 Unicode "*"text, with very long lines"* ]]; then
	if command -v exiftool >/dev/null 2>&1; then
		exiftool -- "$1" | head -n 90 | head -c 2048 >>"$2" 2>&1
		echo "" >>"$2" 2>&1
	else
		echo "Install <exiftool> to see information" >>"$2" 2>&1
	fi
	echo "------------" >>"$2" 2>&1
	echo "Processing file as fb2 with pandoc ( formatted as markdown )" >>"$2" 2>&1
	echo "" >>"$2" 2>&1
	if [[ ."$FILECHARSET" == ."" ]] \
		|| [[ ."$FILECHARSET" == ."utf-8" ]] \
		|| [[ ."$FILECHARSET" == ."UTF-8" ]] \
		|| [[ ."$FILECHARSET" == ."binary" ]] \
		|| [[ ."$FILECHARSET" == ."unknown-8bit" ]]; then
		FROMENC=""
	else
		FROMENC="-f "$FILECHARSET" "
	fi
	if [[ ! ."$FROMENC" == ."" ]]; then
		if command -v iconv >/dev/null 2>&1; then
			echo "Using iconv to convert from [ "$FILECHARSET" ] to [ utf-8 ] for pandoc input" >>"$2" 2>&1
		else
			echo "Install <iconv> to convert from [ "$FILECHARSET" ] to [ utf-8 ] for pandoc input" >>"$2" 2>&1
		fi
	else
		echo "Will not convert from [ "$FILECHARSET" ] to [ utf-8 ] for pandoc input" >>"$2" 2>&1
	fi
	echo "" >>"$2" 2>&1
	echo "----bof----" >>"$2" 2>&1
	if command -v pandoc >/dev/null 2>&1; then
		if command -v iconv >/dev/null 2>&1; then
			iconv $FROMENC -t utf-8 -- "$1" | pandoc -f fb2 -t markdown -- >>"$2" 2>&1
		else
			pandoc -f fb2 -t markdown -- "$1" >>"$2" 2>&1
		fi
	else
		echo "Install <pandoc> to see document" >>"$2" 2>&1
	fi
	echo "----eof----" >>"$2" 2>&1
	FROMENC=
	exit 0
fi

if [[ "$FILE" == *": Microsoft Word 2007+"* ]]; then
	if command -v exiftool >/dev/null 2>&1; then
		exiftool -- "$1" | head -n 90 | head -c 2048 >>"$2" 2>&1
		echo "" >>"$2" 2>&1
	else
		echo "Install <exiftool> to see information" >>"$2" 2>&1
	fi
	echo "------------" >>"$2" 2>&1
	echo "Processing file as docx with pandoc ( formatted as markdown )" >>"$2" 2>&1
	echo "" >>"$2" 2>&1
	echo "----bof----" >>"$2" 2>&1
	if command -v pandoc >/dev/null 2>&1; then
		# pandoc -- "$1" >>"$2" 2>&1
		pandoc -f docx -t markdown -- "$1" >>"$2" 2>&1
	else
		echo "Install <pandoc> to see document" >>"$2" 2>&1
	fi
	echo "----eof----" >>"$2" 2>&1
	exit 0
fi

if [[ "$FILE" == *": Composite Document File"*"Microsoft Office Word"* ]] \
	|| [[ "$FILE" == *": Composite Document File V2 Document"* ]]; then
	if command -v exiftool >/dev/null 2>&1; then
		exiftool -- "$1" | head -n 90 | head -c 2048 >>"$2" 2>&1
		echo "" >>"$2" 2>&1
	else
		echo "Install <exiftool> to see information" >>"$2" 2>&1
	fi
	echo "------------" >>"$2" 2>&1
	echo "Processing file as doc with catdoc ( text )" >>"$2" 2>&1
	echo "" >>"$2" 2>&1
	echo "----bof----" >>"$2" 2>&1
	if command -v catdoc >/dev/null 2>&1; then
		catdoc -- "$1" >>"$2" 2>&1
	else
		echo "Install <catdoc> to see document" >>"$2" 2>&1
	fi
	echo "----eof----" >>"$2" 2>&1
	exit 0
fi

if [[ "$FILE" == *": Composite Document File"*"Microsoft PowerPoint"* ]]; then
	if command -v exiftool >/dev/null 2>&1; then
		exiftool -- "$1" | head -n 90 | head -c 2048 >>"$2" 2>&1
		echo "" >>"$2" 2>&1
	else
		echo "Install <exiftool> to see information" >>"$2" 2>&1
	fi
	echo "------------" >>"$2" 2>&1
	echo "Processing file as ppt with catppt ( text )" >>"$2" 2>&1
	echo "" >>"$2" 2>&1
	echo "----bof----" >>"$2" 2>&1
	if command -v catppt >/dev/null 2>&1; then
		catppt -- "$1" >>"$2" 2>&1
	else
		echo "Install <catppt> to see document" >>"$2" 2>&1
	fi
	echo "----eof----" >>"$2" 2>&1
	exit 0
fi

if [[ "$FILE" == *": PDF document"* ]]; then
	if command -v exiftool >/dev/null 2>&1; then
		exiftool -- "$1" | head -n 90 | head -c 2048 >>"$2" 2>&1
		echo "" >>"$2" 2>&1
	else
		echo "Install <exiftool> to see information" >>"$2" 2>&1
	fi
	echo "------------" >>"$2" 2>&1
	echo "Processing file as pdf with pdftotext ( text )" >>"$2" 2>&1
	echo "" >>"$2" 2>&1
	echo "----bof----" >>"$2" 2>&1
	if command -v pdftotext >/dev/null 2>&1; then
		pdftotext -enc UTF-8 -- "$1" - >>"$2" 2>>"$2"
	else
		echo "Install <pdftotext> to see document" >>"$2" 2>&1
	fi
	echo "----eof----" >>"$2" 2>&1
	exit 0
fi

if [[ "$FILE" == *": unified diff output"* ]]; then
	if ! (( command -v diff-so-fancy >/dev/null 2>&1 ) || ( command -v diff-highlight >/dev/null 2>&1 )); then
		echo "Install <diff-so-fancy> or <diff-highlight> to see diffs in **human** readable form" >>"$2" 2>&1
		echo "------------" >>"$2" 2>&1
	fi
	if command -v diff-so-fancy >/dev/null 2>&1; then
		echo "Processing file with diff-so-fancy" >>"$2" 2>&1
		echo "------------" >>"$2" 2>&1
		cat -- "$1" | diff-so-fancy >>"$2" 2>&1
	elif command -v colordiff >/dev/null 2>&1; then
		if command -v diff-highlight >/dev/null 2>&1; then
			echo "Processing file with colordiff and diff-highlight" >>"$2" 2>&1
			echo "------------" >>"$2" 2>&1
			cat -- "$1" | colordiff --color=yes | diff-highlight >>"$2" 2>&1
		else
			echo "Processing file with colordiff" >>"$2" 2>&1
			echo "------------" >>"$2" 2>&1
			cat -- "$1" | colordiff --color=yes >>"$2" 2>&1
		fi
	elif command -v source-highlight >/dev/null 2>&1; then
		if command -v diff-highlight >/dev/null 2>&1; then
			echo "Processing file with source-highlight and diff-highlight" >>"$2" 2>&1
			echo "------------" >>"$2" 2>&1
			cat -- "$1" | ( source-highlight --failsafe --src-lang=diff --out-format=esc -o STDOUT 2> /dev/null || cat ) | diff-highlight >>"$2" 2>&1
		else
			echo "Processing file with source-highlight" >>"$2" 2>&1
			echo "------------" >>"$2" 2>&1
			cat -- "$1" | ( source-highlight --failsafe --src-lang=diff --out-format=esc -o STDOUT 2> /dev/null || cat ) >>"$2" 2>&1
		fi
	elif command -v diff-highlight >/dev/null 2>&1; then
		echo "Processing file with diff-highlight ( two colors only ? )" >>"$2" 2>&1
		echo "------------" >>"$2" 2>&1
		cat -- "$1" | diff-highlight >>"$2" 2>&1
	else
		echo "Install ( <diff-so-fancy> or <diff-highlight> ) and ( <colordiff> or <source-highlight> ) to see colored diff" >>"$2" 2>&1
		echo "------------" >>"$2" 2>&1
		cat -- "$1" >>"$2" 2>&1
	fi
	echo "----eof----" >>"$2" 2>&1
	exit 0
fi

if [[ "$FILE" == *": "*"ASCII text, with very long lines"* ]] \
		|| [[ "$FILE" == *": "*"UTF-8 Unicode "*"text, with very long lines"* ]] \
		|| [[ "$FILE" == *": "*"ISO"*" text, with very long lines"* ]] \
		|| [[ "$FILE" == *": "*"Non-ISO extended-ASCII text, with very long lines"* ]] \
		|| [[ "$FILE" == *": "*"ASCII text, with "*" line terminators"* ]] \
		|| [[ "$FILE" == *": "*"UTF-8 Unicode "*"text, with "*" line terminators"* ]] \
		|| [[ "$FILE" == *": "*"ISO"*" text, with "*" line terminators"* ]] \
		|| [[ "$FILE" == *": "*"Non-ISO extended-ASCII text, with "*" line terminators"* ]] \
		|| [[ "$FILE" == *": "*"ASCII text"* ]] \
		|| [[ "$FILE" == *": "*"UTF-8 Unicode "*"text"* ]] \
		|| [[ "$FILE" == *": "*"ISO"*" text"* ]] \
		|| [[ "$FILE" == *": "*"Non-ISO extended-ASCII text"* ]] \
		|| [[ "$FILE" == *": XML 1.0 document, "*" text"* ]] \
		|| [[ "$FILE" == *": JSON data"* ]] \
		|| [[ "$FILE" == *": PEM certificate"* ]] \
		|| [[ "$FILE" == *": "*" program"*" text"* ]] \
		|| [[ "$FILE" == *": "*" source"*" text"* ]] \
		|| [[ "$FILE" == *": "*" shell"*" text"* ]] \
		|| [[ "$FILE" == *": "*" script"*" text"* ]]; then
	if command -v exiftool >/dev/null 2>&1; then
		exiftool -- "$1" | head -n 90 | head -c 2048 >>"$2" 2>&1
		echo "" >>"$2" 2>&1
	else
		echo "Install <exiftool> to see information" >>"$2" 2>&1
	fi
	echo "------------" >>"$2" 2>&1
	# ??? workaround for bash to get values of variables
	bash -c "echo ${FOO}" >/dev/null 2>&1
	FILESIZE=$( wc -c -- "$1" | sed -n -e 's:^\([0-9]\{1,\}\)\ .\{0,\}$:\1:p' )
	FILESIZE=${FILESIZE:-0}
	echo "File size is "$FILESIZE" bytes" >>"$2" 2>&1
	VIEWLIMIT=1024
	DOTCOUNT=12
	## example ( 1024 + 12 ) = 1036
	SIZELIMIT=$(( VIEWLIMIT + DOTCOUNT ))
	SIZELIMIT=${SIZELIMIT:-64}
	## example ( 1024 / 2 ) = 512
	HALFLIMIT=$(( VIEWLIMIT / 2 ))
	HALFLIMIT=${HALFLIMIT:-32}
	echo "Half of view size limit is ( "$VIEWLIMIT" / 2 ) = "$HALFLIMIT" bytes" >>"$2" 2>&1
	if [[ $FILESIZE -gt $SIZELIMIT ]]; then
		## example with file size ( 1024 + 12 + 1 ) = 1037 bytes
		## ( 1037 - 2 * 512 - 12 ) = 1
		RESTLIMIT=$(( FILESIZE - 2 * HALFLIMIT - DOTCOUNT ))
	fi
	RESTLIMIT=${RESTLIMIT:-0}
	echo "Size of file data that will not be shown is ( "$FILESIZE" - 2 * "$HALFLIMIT" - "$DOTCOUNT" ) = "$RESTLIMIT" bytes" >>"$2" 2>&1
	echo "Processing file as is with cat, head and tail ( raw )" >>"$2" 2>&1
	echo "" >>"$2" 2>&1
	echo "----bof----" >>"$2" 2>&1
	if [[ $FILESIZE -le $SIZELIMIT ]]; then
		cat -- "$1" >>"$2" 2>&1
		echo "" >>"$2" 2>&1
	else
		head -c $HALFLIMIT -- "$1" >>"$2" 2>&1
		echo "" >>"$2" 2>&1
		echo "" >>"$2" 2>&1
		if [[ $RESTLIMIT -gt 0 ]]; then
			# count of dots is DOTCOUNT
			echo ">8::::::::8<" >>"$2" 2>&1
			echo "" >>"$2" 2>&1
			tail -c $HALFLIMIT -- "$1" >>"$2" 2>&1
			echo "" >>"$2" 2>&1
		fi
	fi
	echo "----eof----" >>"$2" 2>&1
	if [[ "$FILE" == *": "*" source, "*" text"* ]]; then
		echo "------------" >>"$2" 2>&1
		echo "Processing file with ctags" >>"$2" 2>&1
		echo "" >>"$2" 2>&1
		echo "------------" >>"$2" 2>&1
		if command -v ctags >/dev/null 2>&1; then
			ctags --totals -x -u "$1" >>"$2" 2>&1
			if [ $? -ne 0 ] ; then
				echo "------------" >>"$2" 2>&1
				ctags -x -u "$1" >>"$2" 2>&1
			fi
		else
			echo "Install <ctags> to see source overview" >>"$2" 2>&1
		fi
		echo "------------" >>"$2" 2>&1
	fi
	exit 0
fi

if [[ "$FILE" == *": symbolic link"* ]]; then
	if command -v exiftool >/dev/null 2>&1; then
		exiftool -- "$1" | head -n 90 | head -c 2048 >>"$2" 2>&1
		echo "" >>"$2" 2>&1
	else
		echo "Install <exiftool> to see information" >>"$2" 2>&1
	fi
	echo "------------" >>"$2" 2>&1
	echo "" >>"$2" 2>&1
	if command -v ls >/dev/null 2>&1; then
		ls -la -- "$1" >>"$2" 2>&1
		echo "" >>"$2" 2>&1
	else
		echo "Install <ls> to see listing" >>"$2" 2>&1
	fi
	echo "----eof----" >>"$2" 2>&1
	exit 0
fi

if [[ "$FILE" == *": "* ]]; then
	if command -v exiftool >/dev/null 2>&1; then
		exiftool -- "$1" | head -n 90 | head -c 2048 >>"$2" 2>&1
		echo "" >>"$2" 2>&1
	else
		echo "Install <exiftool> to see information" >>"$2" 2>&1
	fi
	echo "------------" >>"$2" 2>&1
	echo "Processing file with hexdump ( hexadecimal view )" >>"$2" 2>&1
	echo "" >>"$2" 2>&1
	echo "------------" >>"$2" 2>&1
	# ??? workaround for bash to get values of variables
	bash -c "echo ${FOO}" >/dev/null 2>&1
	FILESIZE=$( wc -c -- "$1" | sed -n -e 's:^\([0-9]\{1,\}\)\ .\{0,\}$:\1:p' )
	FILESIZE=${FILESIZE:-0}
	echo "File size is "$FILESIZE" bytes" >>"$2" 2>&1
	VIEWLIMIT=1024
	DOTCOUNT=12
	## example ( 1024 + 12 ) = 1036
	SIZELIMIT=$(( VIEWLIMIT + DOTCOUNT ))
	SIZELIMIT=${SIZELIMIT:-64}
	## example ( 1024 / 2 ) = 512
	HALFLIMIT=$(( VIEWLIMIT / 2 ))
	HALFLIMIT=${HALFLIMIT:-32}
	echo "Half of view size limit is ( "$VIEWLIMIT" / 2 ) = "$HALFLIMIT" bytes" >>"$2" 2>&1
	if [[ $FILESIZE -gt $SIZELIMIT ]]; then
		## example with file size ( 1024 + 12 + 1 ) = 1037 bytes
		## ( 1037 - 2 * 512 - 12 ) = 1
		RESTLIMIT=$(( FILESIZE - 2 * HALFLIMIT - DOTCOUNT ))
	fi
	RESTLIMIT=${RESTLIMIT:-0}
	echo "Size of file data that will not be shown is ( "$FILESIZE" - 2 * "$HALFLIMIT" - "$DOTCOUNT" ) = "$RESTLIMIT" bytes" >>"$2" 2>&1
	echo "Processing file as is with cat, head, tail and hexdump ( raw )" >>"$2" 2>&1
	echo "" >>"$2" 2>&1
	echo "----bof----" >>"$2" 2>&1
	if command -v hexdump >/dev/null 2>&1; then
		if [[ $FILESIZE -le $SIZELIMIT ]]; then
			cat -- "$1" | hexdump -C >>"$2" 2>&1
			echo "" >>"$2" 2>&1
		else
			head -c $HALFLIMIT -- "$1" | hexdump -C >>"$2" 2>&1
			echo "" >>"$2" 2>&1
			if [[ $RESTLIMIT -gt 0 ]]; then
				# count of dots is DOTCOUNT
				echo ">8::::::::8<" >>"$2" 2>&1
				echo "" >>"$2" 2>&1
				tail -c $HALFLIMIT -- "$1" | hexdump -C >>"$2" 2>&1
				echo "" >>"$2" 2>&1
			fi
		fi
	else
		echo "Install <hexdump> to partially see file contents" >>"$2" 2>&1
	fi
	echo "----eof----" >>"$2" 2>&1
	# exit 0
fi

if [[ "$FILE" == *": "*"boot sector"* ]] \
	|| [[ "$FILE" == *": "*"block size"* ]] \
	|| [[ "$FILE" == *": data"* ]]; then
	echo "" >>"$2" 2>&1
	echo "------------" >>"$2" 2>&1
	echo "Processing file as boot sector with fdisk" >>"$2" 2>&1
	echo "" >>"$2" 2>&1
	echo "------------" >>"$2" 2>&1
	if command -v fdisk >/dev/null 2>&1; then
		fdisk -l "$1" >>"$2" 2>&1
		echo "" >>"$2" 2>&1
	else
		echo "Install <fdisk> to see information" >>"$2" 2>&1
	fi
	echo "------------" >>"$2" 2>&1
	echo "Processing file as boot sector with gdisk" >>"$2" 2>&1
	echo "" >>"$2" 2>&1
	echo "------------" >>"$2" 2>&1
	if command -v fdisk >/dev/null 2>&1; then
		echo "2" | gdisk -l "$1" >>"$2" 2>&1
		echo "" >>"$2" 2>&1
	else
		echo "Install <gdisk> to see information" >>"$2" 2>&1
	fi
	echo "------------" >>"$2" 2>&1
	echo "Processing file as archive with 7z contents listing" >>"$2" 2>&1
	echo "" >>"$2" 2>&1
	echo "----bof----" >>"$2" 2>&1
	if command -v 7zz >/dev/null 2>&1; then
		7zz l -- "$1" >>"$2" 2>&1
		echo "" >>"$2" 2>&1
	elif command -v 7z >/dev/null 2>&1; then
		7z l -- "$1" >>"$2" 2>&1
		echo "" >>"$2" 2>&1
	else
		echo "Install <7zip> or <p7zip-full> to see information" >>"$2" 2>&1
	fi
	echo "----eof----" >>"$2" 2>&1
	exit 0
fi

echo "" >>"$2" 2>&1
echo "Hint: use <F5> to switch back to raw file viewer" >>"$2" 2>&1

##########################################################
# safely source this script from user config dir if it copied there without changes
exit 0
