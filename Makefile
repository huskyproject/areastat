# areastat/Makefile
#
# This file is part of areastat, part of the Husky fidonet software project
# Use with GNU version of make v.3.82 or later
# Requires: husky enviroment
#

areastat_LIBS := $(smapi_TARGET_BLD) $(huskylib_TARGET_BLD)

areastat_CDEFS := $(CDEFS) -I$(smapi_ROOTDIR) \
			  -I$(huskylib_ROOTDIR) -I$(areastat_ROOTDIR)$(areastat_H_DIR)

areastat_TARGET     = areastat$(_EXE)
areastat_TARGET_BLD = $(areastat_BUILDDIR)$(areastat_TARGET)
areastat_TARGET_DST = $(BINDIR_DST)$(areastat_TARGET)

ifdef MAN1DIR
    areastat_MAN1PAGES := areastat.1
    areastat_MAN1BLD := $(areastat_BUILDDIR)$(areastat_MAN1PAGES).gz
    areastat_MAN1DST := $(DESTDIR)$(MAN1DIR)$(DIRSEP)$(areastat_MAN1PAGES).gz
endif
ifdef MAN5DIR
    areastat_MAN5PAGES := areastat.conf.5
    areastat_MAN5BLD := $(areastat_BUILDDIR)$(areastat_MAN5PAGES).gz
    areastat_MAN5DST := $(DESTDIR)$(MAN5DIR)$(DIRSEP)$(areastat_MAN5PAGES).gz
endif

.PHONY: areastat_build areastat_install areastat_uninstall areastat_clean \
	areastat_distclean areastat_depend areastat_rmdir_DEP areastat_rm_DEPS \
	areastat_clean_OBJ areastat_main_distclean

areastat_build: $(areastat_TARGET_BLD) $(areastat_MAN1BLD) $(areastat_MAN5BLD)

ifneq ($(MAKECMDGOALS), depend)
 ifneq ($(MAKECMDGOALS), distclean)
  ifneq ($(MAKECMDGOALS), uninstall)
   include $(hptsqfix_DEPS)
  endif
 endif
endif


# Build application
$(areastat_TARGET_BLD): $(areastat_ALL_OBJS) $(areastat_LIBS) | do_not_run_make_as_root
	$(CC) $(LFLAGS) $(EXENAMEFLAG) $@ $^

# Compile .c files
$(areastat_ALL_OBJS): $(areastat_ALL_SRC) | $(areastat_OBJDIR) do_not_run_make_as_root
	$(CC) $(areastat_CFLAGS) $(areastat_CDEFS) -o $@ $<

$(areastat_OBJDIR): | do_not_run_make_as_root $(areastat_BUILDDIR)
	[ -d $@ ] || $(MKDIR) $(MKDIROPT) $@


# Build man pages
ifdef MAN1DIR
    $(areastat_MAN1BLD): $(areastat_MANDIR)$(areastat_MAN1PAGES) | do_not_run_make_as_root
	gzip -c $< > $@
else
    $(areastat_MAN1BLD): ;
endif
ifdef MAN5DIR
    $(areastat_MAN5BLD): $(areastat_MANDIR)$(areastat_MAN5PAGES) | $(areastat_BUILDDIR) do_not_run_make_as_root
	gzip -c $< > $@
else
    $(areastat_MAN5BLD): ;
endif

# Install
ifneq ($(MAKECMDGOALS), install)
    areastat_install: ;
else
    areastat_install: $(areastat_TARGET_DST) areastat_install_man1 \
					     areastat_install_man5;
endif

$(areastat_TARGET_DST): $(areastat_TARGET_BLD) | $(DESTDIR)$(BINDIR)
	$(INSTALL) $(IBOPT) $< $(DESTDIR)$(BINDIR); \
	$(TOUCH) "$@"

ifndef MAN1DIR
    areastat_install_man1: ;
else
    areastat_install_man1: $(areastat_MAN1DST)

    $(areastat_MAN1DST): $(areastat_MAN1BLD) | $(DESTDIR)$(MAN1DIR)
	$(INSTALL) $(IMOPT) $< $(DESTDIR)$(MAN1DIR); $(TOUCH) "$@"
endif
ifndef MAN5DIR
    areastat_install_man5: ;
else
    areastat_install_man5: $(areastat_MAN5DST)

    $(areastat_MAN5DST): $(areastat_MAN5BLD) | $(DESTDIR)$(MAN5DIR)
	$(INSTALL) $(IMOPT) $< $(DESTDIR)$(MAN5DIR); $(TOUCH) "$@"

$(DESTDIR)$(MAN5DIR): | $(DESTDIR)$(MANDIR)
	[ -d $@ ] || $(MKDIR) $(MKDIROPT) $@
endif


# Clean
areastat_clean: areastat_clean_OBJ
	-[ -d "$(areastat_OBJDIR)" ] && $(RMDIR) $(areastat_OBJDIR) || true

areastat_clean_OBJ:
	-$(RM) $(RMOPT) $(areastat_OBJDIR)*

# Distclean
areastat_distclean: areastat_main_distclean areastat_rmdir_DEP
	-[ -d "$(areastat_BUILDDIR)" ] && $(RMDIR) $(areastat_BUILDDIR) || true

areastat_rmdir_DEP: areastat_rm_DEPS
	-[ -d "$(areastat_DEPDIR)" ] && $(RMDIR) $(areastat_DEPDIR) || true

areastat_rm_DEPS:
	-$(RM) $(RMOPT) $(areastat_DEPDIR)*

areastat_main_distclean: areastat_clean
	-$(RM) $(RMOPT) $(areastat_TARGET_BLD)
ifdef MAN1DIR
	-$(RM) $(RMOPT) $(areastat_MAN1BLD)
endif
ifdef MAN5DIR
	-$(RM) $(RMOPT) $(areastat_MAN5BLD)
endif


# Uninstall
areastat_uninstall:
	-$(RM) $(RMOPT) $(areastat_TARGET_DST)
ifdef MAN1DIR
	-$(RM) $(RMOPT) $(areastat_MAN1DST)
endif
ifdef MAN5DIR
	-$(RM) $(RMOPT) $(areastat_MAN5BLD)
endif
