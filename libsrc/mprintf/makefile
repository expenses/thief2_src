#
# $Source: x:/prj/tech/libsrc/mprintf/RCS/makefile $
# $Revision: 1.4 $
# $Author: kaboom $
# $Date: 1994/08/10 20:19:05 $
#

COMPILER = watcom
SRCDIR = r:\prj\lib\src\mprintf
RELDIR = r:\prj\lib\rel
BUILD_DIR = r:\prj\lib\build\mprintf
DEBUG_ON = 1

OBJDIR = objd
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
MTEST_LIBS = mprintf.lib lg.lib

%include $(INIT)\make.rc

### OPUS MKMF:  Do not remove this line!  Automatic dependencies follow.

mprintf.obj: conio.h dos.h fcntl.h i86.h io.h mprintf.h stat.h stdarg.h \
	 stdio.h string.h types.h

mtest.obj: ctype.h fcntl.h io.h mprintf.h stat.h stdarg.h stdio.h stdlib.h \
	 string.h time.h types.h
