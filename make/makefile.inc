# $Id$
#
# Common makefiles part (include-file) for fidostat

# Use defines before including this file:
# _OBJ	object files suffix begins with point (".o", ".obj", ... )
# _LIB	library files suffix begins with point (".a", ".lib", ... )
# _DLL	dinamic linking library files suffix begins with point (".dll", ".so", ...)
# LIBPREFIX  library filename prefix ("lib", "", ...)
# LIBSUFFIX  library filename suffix (compiler ID, three characters, somewhere empty)
# DLLPREFIX  dinamic linking library prefix (usually empty)
# DLLSUFFIX  dinamic linking library suffix (compiler ID or empty)
# O, B, L, H prefixes to use with names of objects, binaries, libraries and headers respectively

# Module name
LIBNAME = areastat

# Sources directory
_SRC_DIR= ../src/
# Headers directory
H_DIR	= ../h/

# Header files
HEADERS	= $(H)cvsdate.h $(H)areastat.h $(H)version.h

# Object files (please sort list to easy check by human)
OBJS	= $(O)areastat$(_OBJ)


# Binary file(s) to build from sources
PROGRAMS= $(B)$(LIBNAME)$(_EXE)

# List of libraries required to build binary file(s)
LIBS	=
