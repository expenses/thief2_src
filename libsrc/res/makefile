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
RELDIR = r:\prj\lib\rel
OBJDIR = obj
LIBDIR = $(RELDIR)
INCDIR = $(RELDIR)
BINDIR = n:\bin
INCPATH = . $(SRCDIR) $(RELDIR)
LIBPATH = $(RELDIR)

PROG_TARGETS = resdump respack restest lzwtest
LIB_TARGETS = res
INST_TARGETS = res resdump respack
EXTRA_TARGET =

RES_OBJS = lzw.obj lzwfd2bf.obj res.obj restypes.obj resfile.obj \
	resacc.obj refacc.obj resload.obj resmem.obj rescum.obj resbuild.obj \
	resmake.obj resexblk.obj resmisc.obj
RES_HDRS = lzw.h res.h restypes.h

RESDUMP_OBJS	= resdump.obj
RESDUMP_LIBS	= res.lib _lg.lib mprintf.lib

RESPACK_OBJS	= respack.obj
RESPACK_LIBS	= res.lib _lg.lib mprintf.lib

RESTEST_OBJS	= restest.obj
RESTEST_LIBS	= res.lib _lg.lib mprintf.lib

LZWTEST_OBJS	= lzwtest.obj
LZWTEST_LIBS	= res.lib _lg.lib mprintf.lib

!include n:/etc/make.rc

#	----------------------------------
#	Object file dependencies
#	----------------------------------

### OPUS MKMF:  Do not remove this line!  Automatic dependencies follow.

lzw.obj objo\lzw.obj objd\lzw.obj: dbg.h dbgmacro.h io.h lg.h lgalt.h lzw.h malloc.h memall.h \
	 memmacro.h stat.h stdio.h stdlib.h string.h types.h

lzwfd2bf.obj objo\lzwfd2bf.obj objd\lzwfd2bf.obj: dbg.h dbgmacro.h io.h lg.h lgalt.h lzw.h malloc.h memall.h \
	 memmacro.h stat.h stdio.h stdlib.h string.h types.h

lzwtest.obj objo\lzwtest.obj objd\lzwtest.obj: dbg.h dbgmacro.h lg.h lgalt.h lzw.h malloc.h memall.h \
	 memmacro.h stdio.h stdlib.h types.h

refacc.obj objo\refacc.obj objd\refacc.obj: _res.h datapath.h dbg.h dbgmacro.h io.h lg.h lgalt.h lzw.h \
	 malloc.h memall.h memmacro.h res.h res_.h restypes.h stat.h \
	 stdio.h stdlib.h string.h types.h

res.obj objo\res.obj objd\res.obj: _res.h datapath.h dbg.h dbgmacro.h io.h lg.h lgalt.h lzw.h \
	 malloc.h memall.h memmacro.h res.h res_.h restypes.h stat.h \
	 stdio.h stdlib.h string.h types.h

resacc.obj objo\resacc.obj objd\resacc.obj: _res.h datapath.h dbg.h dbgmacro.h lg.h lgalt.h malloc.h \
	 memall.h memmacro.h res.h res_.h restypes.h stdio.h stdlib.h \
	 string.h types.h

resbuild.obj objo\resbuild.obj objd\resbuild.obj: _res.h datapath.h dbg.h dbgmacro.h io.h lg.h lgalt.h lzw.h \
	 malloc.h memall.h memmacro.h res.h res_.h restypes.h stat.h \
	 stdio.h stdlib.h string.h types.h

rescum.obj objo\rescum.obj objd\rescum.obj: _res.h datapath.h dbg.h dbgmacro.h lg.h lgalt.h malloc.h \
	 memall.h memmacro.h res.h res_.h restypes.h stdio.h stdlib.h \
	 string.h types.h

resdump.obj objo\resdump.obj objd\resdump.obj: _res.h datapath.h dbg.h dbgmacro.h io.h lg.h lgalt.h malloc.h \
	 memall.h memmacro.h res.h res_.h restypes.h stat.h stdio.h \
	 stdlib.h string.h types.h

resexblk.obj objo\resexblk.obj objd\resexblk.obj: _res.h datapath.h dbg.h dbgmacro.h io.h lg.h lgalt.h lzw.h \
	 malloc.h memall.h memmacro.h res.h res_.h restypes.h stat.h \
	 stdio.h stdlib.h string.h types.h

resfile.obj objo\resfile.obj objd\resfile.obj: _res.h datapath.h dbg.h dbgmacro.h fcntl.h io.h lg.h lgalt.h \
	 malloc.h memall.h memmacro.h res.h res_.h restypes.h stat.h \
	 stdio.h stdlib.h string.h types.h

resload.obj objo\resload.obj objd\resload.obj: _res.h datapath.h dbg.h dbgmacro.h io.h lg.h lgalt.h lzw.h \
	 malloc.h memall.h memmacro.h res.h res_.h restypes.h stat.h \
	 stdio.h stdlib.h types.h

resmake.obj objo\resmake.obj objd\resmake.obj: _res.h datapath.h dbg.h dbgmacro.h lg.h lgalt.h malloc.h \
	 memall.h memmacro.h res.h res_.h restypes.h stdio.h stdlib.h \
	 string.h types.h

resmem.obj objo\resmem.obj objd\resmem.obj: _res.h datapath.h dbg.h dbgmacro.h lg.h lgalt.h malloc.h \
	 memall.h memmacro.h memstat.h res.h res_.h restypes.h stdio.h \
	 stdlib.h string.h types.h

resmisc.obj objo\resmisc.obj objd\resmisc.obj: datapath.h dbg.h dbgmacro.h lg.h lgalt.h malloc.h memall.h \
	 memmacro.h mprintf.h res.h restypes.h stdio.h stdlib.h types.h

respack.obj objo\respack.obj objd\respack.obj: datapath.h dbg.h dbgmacro.h io.h lg.h lgalt.h malloc.h \
	 memall.h memmacro.h res.h restypes.h stat.h stdio.h stdlib.h \
	 string.h types.h

resswap.obj objo\resswap.obj objd\resswap.obj: _res.h datapath.h dbg.h dbgmacro.h fcntl.h io.h lg.h lgalt.h \
	 lzw.h malloc.h memall.h memmacro.h res.h res_.h restypes.h stat.h \
	 stdio.h stdlib.h string.h types.h

restest.obj objo\restest.obj objd\restest.obj: _res.h conio.h ctype.h datapath.h dbg.h dbgmacro.h fcntl.h \
	 io.h lg.h lgalt.h lzw.h malloc.h memall.h memmacro.h mprintf.h \
	 res.h restypes.h stat.h stdio.h stdlib.h string.h types.h

restypes.obj objo\restypes.obj objd\restypes.obj: datapath.h dbg.h dbgmacro.h lg.h lgalt.h malloc.h memall.h \
	 memmacro.h res.h restypes.h stdio.h stdlib.h types.h
