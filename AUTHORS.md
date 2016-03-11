# AUTHORS

## `dmrlib`

Wijnand Modderman-Lenstra wrote most parts of the library, some parts are
borrowed from the dmrshark project by [Norbert Varga](http://www.nonoo.hu/)
HA2NON, see the [COPYING.dmrshark](COPYING.dmrshark.md) document.

Rudy Hardeman did most of the testing and initial bug reporting.

## `common/*`

Major parts are borrowed from libowfat, a library of general purpose APIs
extracted from Dan Bernstein's software, reimplemented and covered by the
GNU General Public License Version 2 (no later versions), see the
[COPYING.libowfat](COPYING.libowfat.md) document.

The author, Felix von Leitner, can be reached under
[felix-libowfat@fefe.de](mailto:felix-libowfat@fefe.de).

## `common/serial*`

Martin Ling conceived the idea for the library, designed the API and wrote much
of the implementation and documentation.
 
The initial codebase was adapted from serial code in libsigrok, written by Bert
Vermeulen and Uwe Hermann, who gave permission for it to be relicensed under
the LGPL3+ for inclusion in libserialport, see the
[COPYING.libserialport](COPYING.libserialport.md) document.

The package is maintained by Uwe Hermann, with input from Martin Ling.

Aurelien Jacobs made several contributions, including the USB metadata API,
improvements to the port enumeration code, and eliminating the previous
dependence on libudev for enumeration on Linux.
 
Uffe Jakobsen contributed the FreeBSD support.
 
Matthias Heidbrink contributed support for non-standard baudrates on Mac OS X.
 
Bug fixes have been contributed by Bert Vermeulen, silverbuddy, Marcus
Comstedt, Antti Nykanen, Michael B. Trausch and Janne Huttunen.
