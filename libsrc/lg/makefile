#	----------------------------------
#	Makefile for DBG library
#	----------------------------------
# $Header: x:/prj/tech/libsrc/lg/RCS/makefile 1.4 1993/03/04 18:45:27 rex Exp $
# $Log: makefile $
# Revision 1.4  1993/03/04 18:45:27  rex
# Removed libdbg.h
#
# Revision 1.3  1993/03/04  11:51:27  rex
# Added exit.c
# /
# 
# Revision 1.2  1993/01/29  17:31:52  rex
# *** empty log message ***
# 
# Revision 1.1  1993/01/29  09:48:03  rex
# Initial revision
# 

DEBUG_ON = 1
SPEW_ON = 1
WARN_ON = 1
DBG_ON = 1

COMPILER = watcom
SRCDIR = n:\project\lib\src\lg
RELDIR = n:\project\lib\release
OBJDIR = obj
LIBDIR = $(RELDIR)
INCDIR = $(RELDIR)
BINDIR = n:\bin
INCPATH = . $(SRCDIR) $(RELDIR)
LIBPATH = $(RELDIR)
PROG_TARGETS = dbgmakeh dbgtest
LIB_TARGETS = lg
INST_TARGETS = lg dbgmakeh
EXTRA_TARGETS =

LG_SRCS = memall.c dbg.c dbgcfg.c datapath.c exit.c
LG_OBJS = memall.obj dbg.obj dbgcfg.obj datapath.obj exit.obj
LG_HDRS = lg.h lgalt.h memall.h dbg.h datapath.h

DBGMAKEH_SRC = dbgmakeh.c
DBGMAKEH_OBJS = dbgmakeh.obj
DBGMAKEH_LIBS = lg.lib

DBGTEST_SRCS = dbgtest.c
DBGTEST_OBJS = dbgtest.obj
DBGTEST_LIBS = lg.lib mprintf.lib


!include $(INIT)/make.rc

### OPUS MKMF:  Do not remove this line!  Automatic dependencies follow.

datapath.obj: datapath.h malloc.h memall.h stdio.h string.h types.h

dbg.obj: dbg.h malloc.h memall.h mprintf.h stdarg.h stdio.h stdlib.h \
	 string.h types.h

dbgcfg.obj: conio.h ctype.h datapath.h dbg.h mprintf.h stdarg.h stdio.h \
	 stdlib.h string.h types.h

dbgmakeh.obj: conio.h ctype.h dbg.h stdarg.h stdio.h stdlib.h string.h \
	 types.h

dbgtest.obj: conio.h dbg.h lg.h lgalt.h libdbg.h malloc.h memall.h \
	 mprintf.h stdarg.h stdio.h stdlib.h string.h types.h

exit.obj: dbg.h lg.h lgalt.h libdbg.h malloc.h memall.h stdio.h stdlib.h \
	 types.h

memall.obj: dbg.h dos.h i86.h malloc.h memall.h stdio.h string.h types.h
