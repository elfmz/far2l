#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Parse --all flag
RUN_ALL=false
if [ "$1" = "--all" ]; then
	RUN_ALL=true
	shift
fi

# Enable fail-fast for setup/build phases: exit on error and pipe failures.
# Note: -u (unset variables) is omitted because $1 and $2 may be unset.
set -eo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

mkdir -p /tmp/far2l-smoke/output
APP="$1"
if [ "$APP" = "" ]; then
	echo 'Please specify path to far2l binary as argument'
	echo 'Note that far2l must be built with -DTESTING=Yes'
	exit 1
fi
BINARY="$SCRIPT_DIR/far2l-smoke"
if [ ! -f "$BINARY" ] || [ "$BINARY" -ot "$SCRIPT_DIR/far2l-smoke.go" ]; then
	echo '--->' Prepare
	cd "$SCRIPT_DIR"
	go get far2l-smoke
	echo PREPARE: downloading modules
	go mod download
	echo PREPARE: building
	go build
	echo PREPARE: done
fi

echo 'Cleaning up...'
for test in "$SCRIPT_DIR"/tests/*; do
	if [ -d "$test" ]; then
		chmod -R u+w "$test"/workdir 2>/dev/null || true
		rm -rf "$test"/workdir
	fi
done

if [ "$2" == "clean" ]; then
	exit 0
fi

# Prepare workdirs for all tests
for test in "$SCRIPT_DIR"/tests/*; do
	if [ -d "$test" ]; then
		mkdir -p "$test"/workdir
		if [ -e "$test"/initdir ]; then
			cp -r -f "$test"/initdir/. "$test"/workdir/
		fi
	fi
done

if [ "$RUN_ALL" = true ]; then
	echo 'Starting all tests...'
	TEST_DIRS=""
	for test in "$SCRIPT_DIR"/tests/*; do
		if [ -d "$test" ]; then
			TEST_DIRS="$TEST_DIRS $test"
		fi
	done
	# Allow tests to continue on failure
	set +e
	"$BINARY" "$APP" $TEST_DIRS
	EXIT_CODE=$?
	echo ""
	echo "Exit code: $EXIT_CODE"
	exit $EXIT_CODE
else
	echo 'Starting tests:' "$SCRIPT_DIR"/tests/"$2"*
	"$BINARY" "$APP" "$SCRIPT_DIR"/tests/"$2"*
fi
