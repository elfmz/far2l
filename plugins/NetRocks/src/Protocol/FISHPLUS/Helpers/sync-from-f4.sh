#!/bin/sh
# Refreshes helper.sh and helper.ps1 from an f4 checkout. See UPSTREAM.md.
#
# Both helpers are kept byte-for-byte identical to upstream on purpose: f4 is
# where the protocol is developed, and a local edit would have to be redone on
# every refresh.
set -e

F4="$1"
if [ -z "$F4" ]; then
	echo "usage: $0 /path/to/f4" >&2
	exit 2
fi

DIR="$(dirname "$0")"

for f in helper.sh helper.ps1; do
	SRC="$F4/plugins/netfox/fishplus/$f"
	DST="$DIR/$f"

	[ -f "$SRC" ] || { echo "not found: $SRC" >&2; exit 1; }

	# A helper whose token placeholder went missing would make every reply
	# look like payload, so refuse it here rather than at connect time.
	grep -q '__F4_TOKEN__' "$SRC" || { echo "$SRC carries no __F4_TOKEN__ placeholder" >&2; exit 1; }

	if cmp -s "$SRC" "$DST"; then
		echo "$f is already up to date"
	else
		cp "$SRC" "$DST"
		echo "$f updated"
	fi
done

echo
echo "upstream protocol version (sh):  $(sed -n 's/^F4PROTO=\([0-9]*\).*/\1/p' "$DIR/helper.sh" | head -1)"
echo "upstream protocol version (ps1): $(sed -n 's/^\$F4PROTO *= *\([0-9]*\).*/\1/p' "$DIR/helper.ps1" | head -1)"
echo "client protocol version:         $(sed -n 's/.*PROTOCOL_VERSION = \([0-9]*\).*/\1/p' "$DIR/../FishPlusScript.h" | head -1)"
if [ -d "$F4/.git" ]; then
	echo "upstream commit:                 $(git -C "$F4" rev-parse HEAD)"
fi
echo
echo "If any of the protocol versions differ, read f4's FISH+.md before shipping,"
echo "and re-check the coupling points listed in UPSTREAM.md."
