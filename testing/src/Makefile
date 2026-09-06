ALLGOHERE := $(wildcard *.go)

../far2l-smoke: $(ALLGOHERE)
	go get far2l-smoke
	echo PREPARE: downloading modules
	go mod download
	echo PREPARE: building
	go build -o ../far2l-smoke
	echo PREPARE: done
	
clean:
	rm -f ../far2l-smoke
