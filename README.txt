This tool is intended for post-processing book pages scanned to color JPEG
format utilizing the fact that book pages are usually black-on-white or
white, black and one or two other colors.

The utility is designed with GCC in MinGW environment (to mimic Linux)
so hopfully it could be easily ported under any other UNIX system.

To build the thing both gcc compiler and commands like rm should be
available through PATH environment variable.

The utility is based on the open source libraries:
JPEG and PNG (consequently ZLIB) - their sources should be downloaded
separately and unpacked to their relevant directories specified
in the Makefile (or Makefile changed).
