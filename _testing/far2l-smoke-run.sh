mkdir -p /tmp/far2l-smoke/output
APP="$1"
if [ "$APP" = "" ]; then
	echo 'Please specify path to far2l binary as argument'
	echo 'Note that far2l must be built with -DTESTING=Yes'
	exit 1
fi
if [ ! -f ./far2l-smoke ]; then
	echo PREPARE: downloading modules
	go mod download
	echo PREPARE: building
	go build src
	echo PREPARE: done
fi
./far2l-smoke "$APP" /tmp/far2l-smoke/output tests/test*.js 
