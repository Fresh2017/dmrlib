all: build

build: .PHONY
	scons

clean:
	scons -c

.PHONY:
