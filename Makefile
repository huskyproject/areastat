# Generic Makefile for estt

ifeq ($(DEBIAN), 1)
# Every Debian-Source-Paket has one included.
include /usr/share/husky/huskymak.cfg
else
include ../huskymak.cfg
endif

ifeq ($(DEBUG), 1)
  CFLAGS = -I$(INCDIR) -Ih $(DEBCFLAGS) $(WARNFLAGS)
  LFLAGS = $(DEBLFLAGS)
else
  CFLAGS = -I$(INCDIR) -Ih $(OPTCFLAGS) $(WARNFLAGS)
  LFLAGS = $(OPTLFLAGS)
endif

ifneq ($(DYNLIBS), 1)
  LFLAGS += -static -lc
endif

ifeq ($(SHORTNAME), 1)
  LIBS  = -L$(LIBDIR) -lfidoconf -lsmapi
else
  LIBS  = -L$(LIBDIR) -lfidoconfig -lsmapi
endif

CDEFS=-D$(OSTYPE) -DUNAME=\"$(UNAME)\" $(ADDCDEFS)

SRC_DIR=.

OBJS= estt.o

estt: $(OBJS)
		$(CC) $(OBJS) $(LFLAGS) $(LIBS) -o estt


%.o: $(SRC_DIR)%.c
		$(CC) $(CFLAGS) $(CDEFS) -c $<
     
info:
	makeinfo --no-split estt.texi

html:
	export LC_ALL=C; makeinfo --html --no-split estt.texi

docs: info html

FORCE:

man: FORCE
	gzip -9c man/estt.1 > estt.1.gz

clean:
		rm -f *.o *~ src/*.o src/*~

distclean: clean
	-$(RM) $(RMOPT) estt
	-$(RM) $(RMOPT) estt.info
	-$(RM) $(RMOPT) estt.html
	-$(RM) $(RMOPT) estt.1.gz

all: estt docs man

install: all
	$(INSTALL) $(IBOPT) estt $(BINDIR)
ifdef INFODIR
	-$(MKDIR) $(MKDIROPT) $(INFODIR)
	$(INSTALL) $(IMOPT) estt.info $(INFODIR)
	-install-info --info-dir=$(INFODIR)  $(INFODIR)$(DIRSEP)estt.info
endif
ifdef HTMLDIR
	-$(MKDIR) $(MKDIROPT) $(HTMLDIR)
	$(INSTALL) $(IMOPT) estt*html $(HTMLDIR)
endif
ifdef MANDIR
	-$(MKDIR) $(MKDIROPT) $(MANDIR)$(DIRSEP)man1
	$(INSTALL) $(IMOPT) estt.1.gz $(MANDIR)$(DIRSEP)man1
endif

uninstall:
	$(RM) $(RMOPT) $(BINDIR)$(DIRSEP)estt$(EXE)
ifdef INFODIR
	$(RM) $(RMOPT) $(INFODIR)$(DIRSEP)estt.info
endif
ifdef HTMLDIR
	$(RM) $(RMOPT) $(HTMLDIR)$(DIRSEP)estt.html
endif
ifdef MANDIR
	$(RM) $(RMOPT) $(MANDIR)$(DIRSEP)man1$(DIRSEP)estt.1.gz
endif

