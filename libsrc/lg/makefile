#	----------------------------------
#	Makefile for DBG library
#	----------------------------------
# $Header: x:/prj/tech/libsrc/lg/RCS/makefile 1.21 1994/07/08 17:39:28 rex Exp $
# $Log: makefile $
# Revision 1.21  1994/07/08 17:39:28  rex
# Moved to r:
#
# Revision 1.20  1994/06/01  17:14:11  junochoe
# added tmpalloc.c temporary allocation functions
# 
# Revision 1.19  1994/02/28  10:53:02  rex
# Changed kb.lib to input.lib
# 
# Revision 1.18  1993/11/03  19:06:12  tjs
# Dependencies
# 
# Revision 1.17  1993/11/03  18:30:29  tjs
# Added lgsprntf
# 
# Revision 1.16  1993/09/30  18:37:28  rex
# Added memgrow.obj
# 
# Revision 1.15  1993/09/13  11:11:17  dfan
# Add memstack stuff
# 
# Revision 1.14  1993/08/11  17:31:47  rex
# Dependencies
# 
# Revision 1.13  1993/08/06  11:00:03  rex
# Dependencies
# 
# Revision 1.12  1993/07/09  09:35:12  rex
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

LG_SRCS = bugtrak.c dbg.c dbgcfg.c datapath.c exit.c memall.c memgrow.c \
		memstat.c stack.c lgsprntf.c tmpalloc.c
LG_OBJS = bugtrak.obj dbg.obj dbgcfg.obj datapath.obj exit.obj memall.obj \
		memgrow.obj memstat.obj stack.obj lgsprntf.obj \
                tmpalloc.obj
LG_HDRS = lg.h lgalt.h dbg.h datapath.h memmacro.inc dbgmacro.inc \
	memall.h memstat.h bugtrak.h lgsprntf.h tmpalloc.h
LG_H2I  = lg.inc memall.inc dbg.inc

DBGMAKEH_SRCS = dbgmakeh.c
DBGMAKEH_OBJS = dbgmakeh.obj
DBGMAKEH_LIBS = lg.lib

DBGTEST_SRCS = dbgtest.c
DBGTEST_OBJS = dbgtest.obj
DBGTEST_LIBS = _lg.lib _input.lib _mprintf.lib

STACKTST_OBJS = stacktst.obj
STACKTST_LIBS = lg.lib mprintf.lib

!include $(INIT)/make.rc

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

lgsprntf.obj objo\lgsprntf.obj objd\lgsprntf.obj: ctype.h fix.h limits.h stdarg.h string.h types.h

memall.obj objo\memall.obj objd\memall.obj: _lg.h dbg.h dbgmacro.h dos.h i86.h malloc.h memall.h memmacro.h \
	 stdio.h string.h types.h

memgrow.obj objo\memgrow.obj objd\memgrow.obj: malloc.h stdio.h string.h

memstat.obj objo\memstat.obj objd\memstat.obj: _lg.h dbg.h dbgmacro.h dos.h i86.h lg.h lgalt.h malloc.h \
	 memall.h memmacro.h memstat.h stdio.h string.h types.h

stack.obj objo\stack.obj objd\stack.obj: dbg.h dbgmacro.h malloc.h memall.h memmacro.h stdio.h types.h

stacktst.obj objo\stacktst.obj objd\stacktst.obj: malloc.h memall.h memmacro.h stdio.h types.h

tmpalloc.obj objo\tmpalloc.obj objd\tmpalloc.obj: dbg.h dbgmacro.h malloc.h stdio.h tmpalloc.h types.h
