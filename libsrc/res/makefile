#	----------------------------------
#	Makefile for RES library
#	----------------------------------
# 

DEBUG_ON = 1
DBG_ON = 1
WARN_ON = 1
SPEW_ON = 1

COMPILER = watcom
SRCDIR = r:\prj\lib\src\res
RELDIR = r:\prj\lib\release
OBJDIR = obj
LIBDIR = $(RELDIR)
INCDIR = $(RELDIR)
BINDIR = n:\bin
INCPATH = . $(SRCDIR) $(RELDIR)
LIBPATH = $(RELDIR)

PROG_TARGETS = resdump respack restest
LIB_TARGETS = res
INST_TARGETS = res resdump respack
EXTRA_TARGET =

RES_OBJS = lzw.obj res.obj restypes.obj resfile.obj resacc.obj refacc.obj \
	resload.obj resmem.obj rescum.obj resbuild.obj resmake.obj resexblk.obj
RES_HDRS = lzw.h res.h restypes.h

RESDUMP_OBJS	= resdump.obj
RESDUMP_LIBS	= res.lib _lg.lib mprintf.lib

RESPACK_OBJS	= respack.obj
RESPACK_LIBS	= res.lib _lg.lib mprintf.lib

RESTEST_OBJS	= restest.obj
RESTEST_LIBS	= res.lib _lg.lib mprintf.lib

!include n:/etc/make.rc

#	----------------------------------
#	Object file dependencies
#	----------------------------------

### OPUS MKMF:  Do not remove this line!  Automatic dependencies follow.

lzw.obj: dbg.h dbgmacro.h io.h lg.h lgalt.h lzw.h malloc.h memall.h \
	 memmacro.h stat.h stdio.h stdlib.h string.h types.h

refacc.obj: _res.h datapath.h dbg.h dbgmacro.h io.h lg.h lgalt.h lzw.h \
	 malloc.h memall.h memmacro.h res.h res_.h restypes.h stat.h \
	 stdio.h stdlib.h string.h types.h

res.obj: _res.h datapath.h dbg.h dbgmacro.h io.h lg.h lgalt.h lzw.h \
	 malloc.h memall.h memmacro.h res.h res_.h restypes.h stat.h \
	 stdio.h stdlib.h string.h types.h

resacc.obj: _res.h datapath.h dbg.h dbgmacro.h lg.h lgalt.h malloc.h \
	 memall.h memmacro.h res.h res_.h restypes.h stdio.h stdlib.h \
	 string.h types.h

resbuild.obj: _res.h datapath.h dbg.h dbgmacro.h io.h lg.h lgalt.h lzw.h \
	 malloc.h memall.h memmacro.h res.h res_.h restypes.h stat.h \
	 stdio.h stdlib.h string.h types.h

rescum.obj: _res.h datapath.h dbg.h dbgmacro.h lg.h lgalt.h malloc.h \
	 memall.h memmacro.h res.h res_.h restypes.h stdio.h stdlib.h \
	 string.h types.h

resdump.obj: _res.h datapath.h dbg.h dbgmacro.h io.h lg.h lgalt.h malloc.h \
	 memall.h memmacro.h res.h res_.h restypes.h stat.h stdio.h \
	 stdlib.h string.h types.h

resexblk.obj: _res.h datapath.h dbg.h dbgmacro.h io.h lg.h lgalt.h lzw.h \
	 malloc.h memall.h memmacro.h res.h res_.h restypes.h stat.h \
	 stdio.h stdlib.h string.h types.h

resfile.obj: _res.h datapath.h dbg.h dbgmacro.h fcntl.h io.h lg.h lgalt.h \
	 malloc.h memall.h memmacro.h res.h res_.h restypes.h stat.h \
	 stdio.h stdlib.h string.h types.h

resload.obj: _res.h datapath.h dbg.h dbgmacro.h io.h lg.h lgalt.h lzw.h \
	 malloc.h memall.h memmacro.h res.h res_.h restypes.h stat.h \
	 stdio.h stdlib.h types.h

resmake.obj: _res.h datapath.h dbg.h dbgmacro.h lg.h lgalt.h malloc.h \
	 memall.h memmacro.h res.h res_.h restypes.h stdio.h stdlib.h \
	 string.h types.h

resmem.obj: _res.h datapath.h dbg.h dbgmacro.h lg.h lgalt.h malloc.h \
	 memall.h memmacro.h memstat.h res.h res_.h restypes.h stdio.h \
	 stdlib.h types.h

respack.obj: datapath.h dbg.h dbgmacro.h io.h lg.h lgalt.h malloc.h \
	 memall.h memmacro.h res.h restypes.h stat.h stdio.h stdlib.h \
	 string.h types.h

restest.obj: _res.h conio.h ctype.h datapath.h dbg.h dbgmacro.h fcntl.h \
	 io.h lg.h lgalt.h lzw.h malloc.h memall.h memmacro.h mprintf.h \
	 res.h restypes.h stat.h stdio.h stdlib.h string.h types.h

restypes.obj: datapath.h dbg.h dbgmacro.h lg.h lgalt.h malloc.h memall.h \
	 memmacro.h res.h restypes.h stdio.h stdlib.h types.h
