#	----------------------------------
#	Makefile for GFILE library
#	----------------------------------
#
# $Header: x:/prj/tech/libsrc/gfile/RCS/makefile 1.4 1993/10/07 09:44:55 rex Exp $
# $Log: makefile $
# Revision 1.4  1993/10/07 09:44:55  rex
# Added bmp.obj
#
# Revision 1.3  1993/09/27  19:22:42  rex
# Added cel.obj
# 
# Revision 1.2  1993/09/27  18:01:07  rex
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

GFILE_OBJS = gfile.obj pcx.obj gif.obj cel.obj bmp.obj
GFILE_HDRS = gfile.h

BUILD_DIR	= n:\project\lib\build\gfile

!include n:/etc/make.rc

#	----------------------------------
#	Object file dependencies
#	----------------------------------

### OPUS MKMF:  Do not remove this line!  Automatic dependencies follow.

bmp.obj: 2d.h datapath.h dbg.h dbgmacro.h fix.h gfile.h lg.h lgalt.h \
	 malloc.h memall.h memmacro.h rect.h stdio.h stdlib.h string.h \
	 types.h

cel.obj: 2d.h datapath.h dbg.h dbgmacro.h fix.h gfile.h lg.h lgalt.h \
	 limits.h malloc.h memall.h memmacro.h pjfli.h pjstypes.h rect.h \
	 stddef.h stdio.h stdlib.h string.h types.h

gfile.obj: 2d.h datapath.h dbg.h dbgmacro.h fix.h fname.h gfile.h lg.h \
	 lgalt.h malloc.h memall.h memmacro.h rect.h stdio.h stdlib.h \
	 string.h types.h

gif.obj: 2d.h datapath.h dbg.h dbgmacro.h fix.h gfile.h lg.h lgalt.h \
	 malloc.h memall.h memmacro.h rect.h stdio.h stdlib.h string.h \
	 types.h

pcx.obj: 2d.h datapath.h dbg.h dbgmacro.h fix.h gfile.h lg.h lgalt.h \
	 malloc.h memall.h memmacro.h rect.h stdio.h stdlib.h string.h \
	 types.h
