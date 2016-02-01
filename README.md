# dmrlib
C library for Digital Mobile Radio

[![Build Status](https://travis-ci.org/pd0mz/dmrlib.svg?branch=master)](https://travis-ci.org/pd0mz/dmrlib)

# About

`dmrlib` is a C library for building Digital Mobile Radio applications,
included are a couple of utilities that build on `dmrlib`.

**These applications are for educational purposes and should be used by
licensed HAM radio operators, commercial use is strictly prohibited.**

# Compiling

The following software is required to compile dmrlib:

  * [Python 2.7](http://www.python.org/) with [Jinja2](http://jinja.pocoo.org/)
  * A suitable compiler, such as [clang](http://clang.llvm.org) or gcc
  * [git](https://git-scm.com)
  * [talloc](https://talloc.samba.org/)
  * [libpcap](http://www.tcpdump.org)

If you enable the mbelib proto (`--with-mbelib`):

  * [PortAudio](http://portaudio.com/)

To compile dmrlib, you run:

    $ ./configure && make

To compile dmrlib for debugging:

    $ ./configure --with-debug && make

This should result with libdmr built in the *build* directory.

## Compiling on Linux

On Debian (compatible) systems, you need the following packages:

    ~$ sudo apt-get install gcc git python-jinja2 libtalloc-dev

For the *mbe proto*:

    ~$ sudo apt-get install portaudio-dev

Now clone the repository and build the software:

    ~$ git clone --recursive https://github.com/pd0mz/dmrlib
    ...
    ~$ cd dmrlib
    dmrlib$ ./configure
    ...

## Compiling on OS X

Using [Homebrew](http://brew.sh), you need the following packages:

    ~$ brew install talloc python libpcap
    ...
    ~$ pip install Jinja2
    ...

For the *mbe proto*:

    ~$ brew install portaudio

See [compiling on Linux](#compiling-on-linux) on how to build dmrlib.

## Compiling on Windows

Tested to work with the
[Minimalist GNU for Windows (MinGW)](http://www.mingw.org) compiler. You also
need to get these packages:

  * [Git Windows installer](https://git-scm.com/download/win)
  * [Python Windows installer](https://www.python.org/downloads/windows/) version 2.7.x
  * [Jinja2](http://jinja.pocoo.org/)

Make sure you install the Git Bash shell when prompted in the installer.

Start the Git Bash shell, then enter:

    ~$ git clone --recursive https://github.com/pd0mz/dmrlib
    ...
    ~$ cd dmrlib
    dmrlib$ ./configure
    ...

If configure can't find Python, you can run Wright (our configuration utility) manually:

    ~$ export PYTHONPATH=$(pwd)/site_wright
    ~$ /c/Python27/pythonw.exe -m wright.main [.. configure flags ..]

# Contributing

Check out https://github.com/pd0mz/dmrlib

# Supported features

## Cyclic Redundancy Checks

| **Algorithm**                      | **Encoding**   | **Decoding**   |
|:-----------------------------------|:---------------|:---------------|
| CRC-5                              | :x:            | :x:            |
| CRC-9                              | :thumbsup:     | :thumbsup:     |
| CRC-16 (*CCITT*)                   | :thumbsup:     | :thumbsup:     |
| CRC-32                             | :thumbsup:     | :thumbsup:     |

## (Forward) Error Correction

| **Algorithm**                        | **Encoding**   | **Decoding**   |
|:-------------------------------------|:---------------|:---------------|
| Block Product Turbo Code (196, 96)   | :thumbsup:     | :thumbsup:     |
| Hamming (7,4,3)                      | :thumbsup:     | :thumbsup:     |
| Hamming (13,9,3)                     | :thumbsup:     | :thumbsup:     |
| Hamming (15,11,3)                    | :thumbsup:     | :thumbsup:     |
| Hamming (16,11,4)                    | :thumbsup:     | :thumbsup:     |
| Hamming (17,12,3)                    | :thumbsup:     | :thumbsup:     |
| Golay (20, 8)                        | :thumbsup:     | :thumbsup:     |
| Quadratic Residue (16, 7, 6)         | :thumbsup:     | :thumbsup:     |
| Reed-Solomon (12, 9, 4) <sup>2</sup> | :thumbsup:     | :thumbsup:     |
| Rate ¾ Trellis                       | :x:            | :x:            |
| Variable length BPTC                 | :thumbsup:     | :thumbsup:     |

1.  Full Hamming error correction not yet available
2.  Simplified form, shortened syndrome not implemented

## DMR Data Types

| **Data Type**                      | **Encoding**   | **Decoding**   |
|:-----------------------------------|:---------------|:---------------|
| Privacy Indicator                  | :x:            | :x:            |
| Voice Link Control                 | :thumbsup:     | :thumbsup:     |
| Terminator with Link Control       | :x:            | :x:            |
| Control Signaling Block            | :x:            | :x:            |
| Multiple Block Control             | :x:            | :x:            |
| Data Header                        | :x:            | :x:            |
| Rate                               | :x:            | :x:            |
| Rate ¾ Data                        | :x:            | :x:            |
| Idle                               | :x:            | :x:            |
| Voice <sup>1</sup>                 | :thumbsup:     | :thumbsup:     |

1.  Depends on the availability of compatible hardware and/or software

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
