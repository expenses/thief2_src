#	----------------------------------
#	Makefile for GFILE library
#	----------------------------------
#
# $Header: x:/prj/tech/libsrc/gfile/RCS/makefile 1.2 1993/09/27 18:01:07 rex Exp $
# $Log: makefile $
# Revision 1.2  1993/09/27 18:01:07  rex
# Added gif.obj
#
# Revision 1.1  1993/09/27  16:38:55  rex
# Initial revision
# 
# 

DEBUG_ON = 1
DBG_ON = 1
WARN_ON = 1
SPEW_ON = 1

COMPILER = watcom
SRCDIR = n:\project\lib\src\gfile
RELDIR = n:\project\lib\release
OBJDIR = obj
LIBDIR = $(RELDIR)
INCDIR = $(RELDIR)
BINDIR =
INCPATH = . $(SRCDIR) $(RELDIR)
LIBPATH = $(RELDIR)

LIB_TARGETS  = gfile
INST_TARGETS = gfile

GFILE_OBJS = gfile.obj pcx.obj gif.obj
GFILE_HDRS = gfile.h

BUILD_DIR	= n:\project\lib\build\gfile

!include n:/etc/make.rc

#	----------------------------------
#	Object file dependencies
#	----------------------------------

### OPUS MKMF:  Do not remove this line!  Automatic dependencies follow.

gfile.obj: 2d.h datapath.h dbg.h dbgmacro.h fix.h fname.h gfile.h lg.h \
	 lgalt.h malloc.h memall.h memmacro.h rect.h stdio.h stdlib.h \
	 string.h types.h

gif.obj: 2d.h datapath.h dbg.h dbgmacro.h fix.h gfile.h lg.h lgalt.h \
	 malloc.h memall.h memmacro.h rect.h stdio.h stdlib.h string.h \
	 types.h

pcx.obj: 2d.h datapath.h dbg.h dbgmacro.h fix.h gfile.h lg.h lgalt.h \
	 malloc.h memall.h memmacro.h rect.h stdio.h stdlib.h string.h \
	 types.h
