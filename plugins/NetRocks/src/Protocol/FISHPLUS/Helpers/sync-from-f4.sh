#!/bin/sh
# Refreshes helper.sh from an f4 checkout. See UPSTREAM.md.
#
# The helper is kept byte-for-byte identical to upstream on purpose: f4 is
# where the protocol is developed, and a local edit would have to be redone on
# every refresh.
set -e

F4="$1"
if [ -z "$F4" ]; then
	echo "usage: $0 /path/to/f4" >&2
	exit 2
fi

SRC="$F4/plugins/netfox/fishplus/helper.sh"
DST="$(dirname "$0")/helper.sh"

[ -f "$SRC" ] || { echo "not found: $SRC" >&2; exit 1; }

# A helper whose token placeholder went missing would make every reply look
# like payload, so refuse it here rather than at connect time.
grep -q '__F4_TOKEN__' "$SRC" || { echo "$SRC carries no __F4_TOKEN__ placeholder" >&2; exit 1; }

if cmp -s "$SRC" "$DST"; then
	echo "helper.sh is already up to date"
else
	cp "$SRC" "$DST"
	echo "helper.sh updated"
fi

echo
echo "upstream protocol version: $(sed -n 's/^F4PROTO=\([0-9]*\).*/\1/p' "$DST" | head -1)"
echo "client protocol version:   $(sed -n 's/.*PROTOCOL_VERSION = \([0-9]*\).*/\1/p' "$(dirname "$0")/../FishPlusScript.h" | head -1)"
if [ -d "$F4/.git" ]; then
	echo "upstream commit:           $(git -C "$F4" rev-parse HEAD)"
fi
echo
echo "If the two protocol versions differ, read f4's FISH+.md before shipping,"
echo "and re-check the coupling points listed in UPSTREAM.md."
