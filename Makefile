# $Id$
#
# Generic Makefile for areastat (estt by Dmitry Rusov)

.PHONY: man clean distclean uninstall all install

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
  LIBS  = -L$(LIBDIR) -lhusky -lsmapi
else
#  LIBS  = -L$(LIBDIR) -lhusky -lfidoconfig -lsmapi
  LIBS  = -L$(LIBDIR) -lhusky -lsmapi
endif

CDEFS=-D$(OSTYPE) -DUNAME=\"$(UNAME)\" $(ADDCDEFS)

SRC_DIR=src/

OBJS= areastat.o
MANS= man/areastat.1 man/areastat.conf.5

areastat: $(OBJS)
		$(CC) $(OBJS) $(LFLAGS) $(LIBS) -o areastat


%.o: $(SRC_DIR)%.c
		$(CC) $(CFLAGS) $(CDEFS) -c $<

man: $(foreach m,$(MANS),$(m).gz)

%.gz: %
	gzip -9c $< > $@

clean:
		rm -f *.o *~ src/*.o src/*~

distclean: clean
	-$(RM) $(RMOPT) areastat

all: areastat man

install: all
	$(INSTALL) $(IBOPT) areastat $(BINDIR)
ifdef MANDIR
	-$(MKDIR) $(MKDIROPT) $(MANDIR)$(DIRSEP)man1
	for m in $(MANS); do $(INSTALL) $(IMOPT) $$m.gz $(MANDIR)$(DIRSEP)man`echo $$m | sed s/.*\\.//`/; done
endif

uninstall:
	$(RM) $(RMOPT) $(BINDIR)$(DIRSEP)areastat$(EXE)
ifdef MANDIR
	$(RM) $(RMOPT) $(MANDIR)$(DIRSEP)man1$(DIRSEP)areastat.1.gz
endif

