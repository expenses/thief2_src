#
# $Source: x:/prj/tech/libsrc/mprintf/RCS/makefile $
# $Author: dc $
# $Date: 1993/01/27 16:10:47 $
#

COMPILER = watcom
SRCDIR = n:\project\lib\src\mprintf
RELDIR = n:\project\lib\release

OBJDIR = obj
BINDIR =
LIBDIR = $(RELDIR)
INCDIR = $(RELDIR)
INCPATH = . $(SRCDIR) $(RELDIR)
LIBPATH = . $(RELDIR)
PROG_TARGETS = mtest
LIB_TARGETS = mprintf
HDR_TARGETS =
INST_TARGETS = mprintf

MPRINTF_OBJS = mprintf.obj

MPRINTF_HDRS = mprintf.h

MTEST_OBJS = mtest.obj
MTEST_LIBS = mprintf.lib

#MTEST_CFLAGS    = -d2
#MTEST_LDFLAGS   = debug all

#MPRINTF_CFLAGS  = -d2
#MPRINTF_LDFLAGS = debug all

!include $(INIT)/make.rc

### OPUS MKMF:  Do not remove this line!  Automatic dependencies follow.

mprintf.obj: conio.h dos.h fcntl.h i86.h io.h mprintf.h stat.h stdarg.h \
	 stdio.h string.h types.h

mtest.obj: ctype.h fcntl.h io.h mprintf.h stat.h stdarg.h stdio.h stdlib.h \
	 string.h time.h types.h
