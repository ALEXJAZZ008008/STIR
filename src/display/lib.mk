#
# $Id$
#

dir := display

#******* first find out which files we need and what compilation options

ifeq "$(GRAPHICS)" "X"
#** CURSES: which curses library to use when having GRAPHICS=X
ifeq ($(SYSTEM),LINUX)
CURSES=ncurses
else
ifeq ($(SYSTEM),CYGWIN)
CURSES=curses
else
CURSES=curses
endif
endif
GRAPHOPT = -DSTIR_SIMPLE_BITMAPS -DSC_XWINDOWS
GRAPH_LIBS = -l$(CURSES) -lX11 -L /usr/X11/lib -L /usr/X11R6/lib
$(dir)_LIB_SOURCES := display_array.cxx gen.c screengen.c screen.c

else

ifeq "$(GRAPHICS)" "PGM"
GRAPHOPT = -DSTIR_PGM
$(dir)_LIB_SOURCES := display_array.cxx
else
ifeq "$(GRAPHICS)" "MATHLINK"
# this presumably needs a Mathlink library, depends on your system though
GRAPH_LIBS =
GRAPHOPT = -DSTIR_MATHLINK
$(dir)_LIB_SOURCES := display_array.cxx  mathlinkhelp.c
else
GRAPHOPT =
$(dir)_LIB_SOURCES := display_array.cxx
endif
endif
endif

CFLAGS+=$(GRAPHOPT)
SYS_LIBS+=$(GRAPH_LIBS)

include $(WORKSPACE)/lib.mk
