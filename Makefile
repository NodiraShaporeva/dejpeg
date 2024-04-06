ZLIBDIR=zlib
PLIBDIR=lpng
JLIBDIR=jpeg

OBJEXT=.o
LIBEXT=.a
EXE=.exe

LIBJPEG=$(JLIBDIR)/libjpeg$(LIBEXT)
LIBPNG=$(PLIBDIR)/libpng$(LIBEXT)
LIBZ=$(ZLIBDIR)/libz$(LIBEXT)
LIBRARIES=$(LIBJPEG) $(LIBPNG) $(LIBZ)

JMAKEFILE=makefile.wingcc
PMAKEFILE=scripts/makefile.gcc
ZMAKEFILE=win32/makefile.gcc

CC=gcc
INCLUDES=-I$(JLIBDIR) -I$(PLIBDIR) -I$(ZLIBDIR)
CFLAGS=$(INCLUDES) -O -Wall -pedantic

all: dejpeg$(EXE)

dejpeg$(EXE): dejpeg.c $(LIBRARIES)
	gcc $(CFLAGS) -o $@ dejpeg.c $(LIBRARIES)


$(LIBZ):
	(cd $(ZLIBDIR) && $(MAKE) -f $(ZMAKEFILE) libz$(LIBEXT))

$(LIBPNG):
	(cd $(PLIBDIR) && $(MAKE) -f $(PMAKEFILE) libpng$(LIBEXT))

$(LIBJPEG):
	(cd $(JLIBDIR) && $(MAKE) -f $(JMAKEFILE) libjpeg$(LIBEXT))


clean:
	rm -f dejpeg$(EXE) *$(OBJEXT)
	(cd $(ZLIBDIR) && $(MAKE) -f $(ZMAKEFILE) clean)
	(cd $(PLIBDIR) && $(MAKE) -f $(PMAKEFILE) clean)
	(cd $(JLIBDIR) && $(MAKE) -f $(JMAKEFILE) clean)

