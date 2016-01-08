# dmrlib
C library for Digital Mobile Radio

# Compiling

The following software is required to compile dmrlib:

  * [Python 2.7](http://www.python.org/)
  * [SConstruct](http://scons.org/)
  * A suitable compiler, such as [clang](http://clang.llvm.org) or gcc
  * [libpcap](http://www.tcpdump.org)

To compile dmrlib, you just run scons:

  scons

This should result with libdmr built in the *build* directory.

## Compiling on Linux

On Debian (compatible) systems, you need the following packages:

  sudo apt-get install gcc scons libpcap-dev

## Compiling on OS X

Using [Homebrew](http://brew.sh), you need the following packages:

  brew install scons pcap

## Compiling on Windows

Tested to work with the
[Minimalist GNU for Windows (MinGW)](http://www.mingw.org) compiler.

# Contributing

Check out https://github.com/pd0mz/dmrlib

# Acknowledgements

The development of dmrlib wasn't possible without the help of the following
people:

  * [Rudy Hardeman PD0ZRY](https://github.com/zarya)
  * Artem Prilutskiy R3ABM for support with Homebrew protocol
  * [Guus van Dooren PE1PLM](http://dvmega.auria.nl) for providing hardware to integrate MMDVM
  * [Jonathan Naylor G4KLX](https://twitter.com/G4KLX)

# Bugs

You can use the [issue tracker](https://github.com/pd0mz/dmrlib/issues) to file
bug reports and feature requests.
