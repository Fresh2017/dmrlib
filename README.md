# dmrlib
C library for Digital Mobile Radio

# About

`dmrlib` is a C library for building Digital Mobile Radio applications,
included are a couple of utilities that build on `dmrlib`.

**These applications are for educational purposes and should be used by 
licensed HAM radio operators, commercial use is strictly prohibited.**

# Compiling

The following software is required to compile dmrlib:

  * [Python 2.7](http://www.python.org/)
  * [SConstruct](http://scons.org/)
  * A suitable compiler, such as [clang](http://clang.llvm.org) or gcc
  * [git](https://git-scm.com)
  * [talloc](https://talloc.samba.org/)

If you enable the mbelib proto (`--enable-mbe-proto`):

  * [PortAudio](http://portaudio.com/)

If you enable the dmrdump utility (`--enable-dmrdump`):

  * [libpcap](http://www.tcpdump.org)

To compile dmrlib, you just run scons:

    $ scons

To compile dmrlib for debugging:

    $ scons --with-debug

This should result with libdmr built in the *build* directory.

## Compiling on Linux

On Debian (compatible) systems, you need the following packages:

    ~$ sudo apt-get install gcc scons git libtalloc-dev

For the *mbe proto*:

    ~$ sudo apt-get install libpcap-dev

For the *dmrdump* utility:

    ~$ sudo apt-get install portaudio-dev

Now clone the repository and build the software:

    ~$ git clone https://github.com/pd0mz/dmrlib
    ...
    ~$ cd dmrlib
    dmrlib$ scons
    ...

## Compiling on OS X

Using [Homebrew](http://brew.sh), you need the following packages:

    ~$ brew install scons talloc

For the *mbe proto*:

    ~$ brew install portaudio

For the *dmrdump* utility:

    ~$ brew install pcap

See [compiling on Linux](#compiling-on-linux) on how to build dmrlib.

## Compiling on Windows

Tested to work with the
[Minimalist GNU for Windows (MinGW)](http://www.mingw.org) compiler. You also
need to get these packages:

  * [Scons Windows installer](http://www.scons.org/download.php)
  * [Git Windows installer](https://git-scm.com/download/win)

Make sure you install the Git Bash shell when prompted in the installer.

Start the Git Bash shell, then enter:

    ~$ git clone https://github.com/pd0mz/dmrlib
    ...
    ~$ cd dmrlib
    dmrlib$ /c/Python27/Scripts/scons.bat
    ...

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
