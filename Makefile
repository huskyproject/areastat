# $Id$
#
# Generic Makefile for areastat (estt by Dmitry Rusov)

.PHONY: docs html info mans clean distclean uninstall all install

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

SRC_DIR=src/

OBJS= areastat.o
MANS= man/areastat.1 man/areastat.conf.5

areastat: $(OBJS)
		$(CC) $(OBJS) $(LFLAGS) $(LIBS) -o areastat


%.o: $(SRC_DIR)%.c
		$(CC) $(CFLAGS) $(CDEFS) -c $<
     
info:
	makeinfo --no-split areastat.texi

html:
	export LC_ALL=C; makeinfo --html --no-split areastat.texi

docs: #info html


man: $(foreach m,$(MANS),$(m).gz)

%.gz: %
	gzip -9c $< > $@

clean:
		rm -f *.o *~ src/*.o src/*~

distclean: clean
	-$(RM) $(RMOPT) areastat
	-$(RM) $(RMOPT) areastat.info
	-$(RM) $(RMOPT) areastat.html
	-for m in $(MANS); do $(RM) $(RMOPT) $$m ; done

all: areastat docs man

install: all
	$(INSTALL) $(IBOPT) areastat $(BINDIR)
ifdef INFODIR
	-$(MKDIR) $(MKDIROPT) $(INFODIR)
	$(INSTALL) $(IMOPT) areastat.info $(INFODIR)
	-install-info --info-dir=$(INFODIR)  $(INFODIR)$(DIRSEP)areastat.info
endif
ifdef HTMLDIR
	-$(MKDIR) $(MKDIROPT) $(HTMLDIR)
	$(INSTALL) $(IMOPT) areastat*html $(HTMLDIR)
endif
ifdef MANDIR
	-$(MKDIR) $(MKDIROPT) $(MANDIR)$(DIRSEP)man1
	for m in $(MANS); do $(INSTALL) $(IMOPT) $$m.gz $(MANDIR)$(DIRSEP)man`echo $$m | sed s/.*\\.//`/; done
endif

uninstall:
	$(RM) $(RMOPT) $(BINDIR)$(DIRSEP)areastat$(EXE)
ifdef INFODIR
	$(RM) $(RMOPT) $(INFODIR)$(DIRSEP)areastat.info
endif
ifdef HTMLDIR
	$(RM) $(RMOPT) $(HTMLDIR)$(DIRSEP)areastat.html
endif
ifdef MANDIR
	$(RM) $(RMOPT) $(MANDIR)$(DIRSEP)man1$(DIRSEP)areastat.1.gz
endif

