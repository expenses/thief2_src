#	----------------------------------
#	Makefile for RND library
#	----------------------------------
# 

DEBUG_ON = 1
DBG_ON = 1
WARN_ON = 1
SPEW_ON = 1

COMPILER = watcom
SRCDIR = n:\project\lib\src\res
RELDIR = n:\project\lib\release
OBJDIR = obj
LIBDIR = $(RELDIR)
INCDIR = $(RELDIR)
BINDIR = n:\bin
INCPATH = . $(SRCDIR) $(RELDIR)
LIBPATH = $(RELDIR)

PROG_TARGETS = rndtest
LIB_TARGETS = rnd
INST_TARGETS = rnd
EXTRA_TARGETS =

RND_SRCS	= rnd.c
RND_OBJS = rnd.obj
RND_HDRS	= rnd.h

RNDTEST_SRCS = rndtest.c
RNDTEST_OBJS = rndtest.obj
RNDTEST_LIBS = rnd.lib fix.lib lg.lib mprintf.lib

!include n:/etc/make.rc

#	----------------------------------
#	Object file dependencies
#	----------------------------------

### OPUS MKMF:  Do not remove this line!  Automatic dependencies follow.

rnd.obj: dbg.h fix.h lg.h lgalt.h libdbg.h malloc.h math.h memall.h rnd.h \
	 stdio.h types.h

rndtest.obj: conio.h ctype.h fix.h math.h rnd.h stdio.h stdlib.h string.h \
	 types.h
