#	----------------------------------
#	Makefile for DBG library
#	----------------------------------
# $Header: x:/prj/tech/libsrc/lg/RCS/makefile 1.12 1993/07/09 09:35:12 rex Exp $
# $Log: makefile $
# Revision 1.12  1993/07/09 09:35:12  rex
# Dependencies
#
# Revision 1.11  1993/05/01  20:33:02  xemu
# added bugtrak to the LG library
# 
# Revision 1.10  1993/04/22  11:39:53  rex
# Added kb.lib to test prog libs
# 
# Revision 1.9  1993/04/08  10:28:16  rex
# Added MemStat.C
# 
# Revision 1.8  1993/03/24  12:12:37  matt
# install assembly macros header file
# 
# Revision 1.7  1993/03/24  12:02:27  matt
# More asm header junk
# 
# Revision 1.6  1993/03/22  19:10:22  matt
# Added list of .inc files to be converted from .h files.
# 
# Revision 1.5  1993/03/22  18:04:18  rex
# Fixed plurality of DBGMAKEH_SRCS
# 
# Revision 1.4  1993/03/04  18:45:27  rex
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

LG_SRCS = dbg.c dbgcfg.c datapath.c exit.c memall.c memstat.c bugtrak.c
LG_OBJS = dbg.obj dbgcfg.obj datapath.obj exit.obj memall.obj memstat.obj \
        bugtrak.obj
LG_HDRS = lg.h lgalt.h dbg.h datapath.h memmacro.inc dbgmacro.inc \
	memall.h memstat.h bugtrak.h
LG_H2I  = lg.inc memall.inc dbg.inc

DBGMAKEH_SRCS = dbgmakeh.c
DBGMAKEH_OBJS = dbgmakeh.obj
DBGMAKEH_LIBS = lg.lib

DBGTEST_SRCS = dbgtest.c
DBGTEST_OBJS = dbgtest.obj
DBGTEST_LIBS = lg.lib kb.lib mprintf.lib

!include $(INIT)/make.rc

### OPUS MKMF:  Do not remove this line!  Automatic dependencies follow.

bugtrak.obj: array.h bugtrak.h dbg.h dbgmacro.h env.h error.h fcntl.h io.h \
	 lg.h lgalt.h libdbg.h malloc.h memall.h memmacro.h mprintf.h \
	 stat.h stdio.h string.h time.h types.h

datapath.obj: datapath.h dos.h fcntl.h i86.h malloc.h memall.h memmacro.h \
	 stdarg.h stdio.h stdlib.h string.h types.h

dbg.obj: dbg.h dbgmacro.h malloc.h memall.h memmacro.h mprintf.h stdarg.h \
	 stdio.h stdlib.h string.h types.h

dbgcfg.obj: conio.h ctype.h datapath.h dbg.h dbgmacro.h error.h kb.h \
	 kbcook.h lg.h lgalt.h libdbg.h malloc.h memall.h memmacro.h \
	 mprintf.h stdarg.h stdio.h stdlib.h string.h types.h

dbgmakeh.obj: conio.h ctype.h dbg.h dbgmacro.h stdarg.h stdio.h stdlib.h \
	 string.h types.h

dbgtest.obj: conio.h datapath.h dbg.h dbgmacro.h lg.h lgalt.h libdbg.h \
	 malloc.h memall.h memmacro.h memstat.h mprintf.h stdarg.h stdio.h \
	 stdlib.h string.h types.h

exit.obj: dbg.h dbgmacro.h lg.h lgalt.h libdbg.h malloc.h memall.h \
	 memmacro.h stdio.h stdlib.h types.h

memall.obj: dbg.h dbgmacro.h dos.h i86.h malloc.h memall.h memmacro.h \
	 stdio.h string.h types.h

memstat.obj: dbg.h dbgmacro.h dos.h i86.h lg.h lgalt.h libdbg.h malloc.h \
	 memall.h memmacro.h memstat.h stdio.h string.h types.h
