#
# $Source: x:/prj/tech/libsrc/lg/RCS/makefile $
# $Revision: 1.23 $
# $Author: kaboom $
# $Date: 1994/08/04 22:27:03 $
#
# makefile for lg library.
#

DEBUG_ON = 1
SPEW_ON = 1
WARN_ON = 1
DBG_ON = 1

COMPILER = watcom
SRCDIR = r:\prj\lib\src\lg
RELDIR = r:\prj\lib\rel
OBJDIR = obj
LIBDIR = $(RELDIR)
INCDIR = $(RELDIR)
BINDIR = n:\bin
INCPATH = . $(SRCDIR) $(RELDIR)
LIBPATH = $(RELDIR)
PROG_TARGETS = dbgmakeh dbgtest stacktst
LIB_TARGETS = lg
INST_TARGETS = lg dbgmakeh
EXTRA_TARGETS =

LG_SRCS = bugtrak.c dbg.c dbgcfg.c datapath.c exit.c \
                lgprntf.c lgsprntf.c memall.c memgrow.c memstat.c \
                stack.c oheapwal.asm tmpalloc.c
LG_OBJS = bugtrak.obj dbg.obj dbgcfg.obj datapath.obj exit.obj \
                lgsprntf.obj memall.obj memgrow.obj memstat.obj \
                lgprntf.obj stack.obj oheapwal.obj tmpalloc.obj
LG_HDRS = lg.h lgalt.h dbg.h datapath.h memmacro.inc dbgmacro.inc \
	memall.h memstat.h bugtrak.h lgprntf.h lgsprntf.h tmpalloc.h 
LG_H2I  = lg.inc memall.inc dbg.inc

DBGMAKEH_SRCS = dbgmakeh.c
DBGMAKEH_OBJS = dbgmakeh.obj
DBGMAKEH_LIBS = lg.lib

DBGTEST_SRCS = dbgtest.c
DBGTEST_OBJS = dbgtest.obj
DBGTEST_LIBS = _lg.lib _input.lib _mprintf.lib

STACKTST_OBJS = stacktst.obj
STACKTST_LIBS = lg.lib mprintf.lib

%include $(INIT)\make.rc

### OPUS MKMF:  Do not remove this line!  Automatic dependencies follow.

bugtrak.obj objo\bugtrak.obj objd\bugtrak.obj: array.h bugtrak.h dbg.h dbgmacro.h env.h error.h fcntl.h io.h \
	 lg.h lgalt.h lgsprntf.h malloc.h memall.h memmacro.h stat.h \
	 stdarg.h stdio.h string.h time.h types.h

datapath.obj objo\datapath.obj objd\datapath.obj: datapath.h dos.h fcntl.h i86.h malloc.h memall.h memmacro.h \
	 stdarg.h stdio.h stdlib.h string.h types.h

dbg.obj objo\dbg.obj objd\dbg.obj: conio.h ctype.h dbg.h dbgmacro.h error.h kb.h kbcook.h keydefs.h \
	 lg.h lgalt.h malloc.h memall.h memmacro.h mprintf.h stdarg.h \
	 stdio.h stdlib.h string.h types.h

dbgcfg.obj objo\dbgcfg.obj objd\dbgcfg.obj: conio.h ctype.h datapath.h dbg.h dbgmacro.h error.h kb.h \
	 kbcook.h keydefs.h lg.h lgalt.h malloc.h memall.h memmacro.h \
	 mprintf.h stdarg.h stdio.h stdlib.h string.h types.h

dbgmakeh.obj objo\dbgmakeh.obj objd\dbgmakeh.obj: conio.h ctype.h dbg.h dbgmacro.h stdarg.h stdio.h stdlib.h \
	 string.h types.h

dbgtest.obj objo\dbgtest.obj objd\dbgtest.obj: conio.h datapath.h dbg.h dbgmacro.h lg.h lgalt.h malloc.h \
	 memall.h memmacro.h memstat.h mprintf.h stdarg.h stdio.h stdlib.h \
	 string.h types.h

exit.obj objo\exit.obj objd\exit.obj: dbg.h dbgmacro.h lg.h lgalt.h malloc.h memall.h memmacro.h \
	 stdio.h stdlib.h types.h

lgprntf.obj objo\lgprntf.obj objd\lgprntf.obj: dbg.h dbgmacro.h io.h lgsprntf.h malloc.h memall.h memmacro.h \
	 stat.h stdarg.h stdio.h tmpalloc.h types.h

lgsprntf.obj objo\lgsprntf.obj objd\lgsprntf.obj: ctype.h fix.h limits.h stdarg.h string.h types.h

memall.obj objo\memall.obj objd\memall.obj: _lg.h dbg.h dbgmacro.h dos.h i86.h malloc.h memall.h memmacro.h \
	 stdio.h string.h types.h

memgrow.obj objo\memgrow.obj objd\memgrow.obj: malloc.h stdio.h string.h

memstat.obj objo\memstat.obj objd\memstat.obj: _lg.h dbg.h dbgmacro.h dos.h i86.h lg.h lgalt.h malloc.h \
	 memall.h memmacro.h memstat.h mprintf.h stdio.h string.h types.h

stack.obj objo\stack.obj objd\stack.obj: dbg.h dbgmacro.h malloc.h memall.h memmacro.h stdio.h types.h

stacktst.obj objo\stacktst.obj objd\stacktst.obj: malloc.h memall.h memmacro.h stdio.h types.h

tmpalloc.obj objo\tmpalloc.obj objd\tmpalloc.obj: _lg.h dbg.h dbgmacro.h malloc.h memall.h memmacro.h stdio.h \
	 tmpalloc.h types.h
