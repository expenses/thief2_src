#	----------------------------------
#	Makefile for RES library
#	----------------------------------
# 

DEBUG_ON = 1
DBG_ON = 1
WARN_ON = 1
SPEW_ON = 1

COMPILER = watcom
SRCDIR = n:\project\lib\src\res
RELDIR = n:\project\lib\release
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

RES_OBJS = lzw.obj res.obj restypes.obj resbuild.obj
RES_HDRS = lzw.h res.h restypes.h

RESDUMP_OBJS	= resdump.obj
RESDUMP_LIBS	= _res.lib _lg.lib mprintf.lib

RESPACK_OBJS	= respack.obj
RESPACK_LIBS	= _res.lib _lg.lib mprintf.lib

RESTEST_OBJS	= restest.obj
RESTEST_LIBS	= _res.lib _lg.lib mprintf.lib

!include n:/etc/make.rc

#	----------------------------------
#	Object file dependencies
#	----------------------------------

### OPUS MKMF:  Do not remove this line!  Automatic dependencies follow.

lzw.obj: dbg.h io.h lg.h lgalt.h libdbg.h lzw.h malloc.h memall.h stat.h \
	 stdio.h stdlib.h string.h types.h

res.obj: datapath.h dbg.h fcntl.h io.h lg.h lgalt.h libdbg.h lzw.h malloc.h \
	 memall.h res.h restypes.h stat.h stdio.h stdlib.h string.h types.h

resbuild.obj: datapath.h dbg.h fcntl.h io.h lg.h lgalt.h libdbg.h lzw.h \
	 malloc.h memall.h res.h restypes.h stat.h stdio.h stdlib.h \
	 string.h types.h

resdump.obj: datapath.h dbg.h io.h lg.h lgalt.h libdbg.h malloc.h memall.h \
	 res.h restypes.h stat.h stdio.h stdlib.h string.h types.h

respack.obj: datapath.h dbg.h io.h lg.h lgalt.h libdbg.h malloc.h memall.h \
	 res.h restypes.h stat.h stdio.h stdlib.h string.h types.h

restest.obj: conio.h ctype.h datapath.h dbg.h fcntl.h io.h lg.h lgalt.h \
	 libdbg.h lzw.h malloc.h memall.h mprintf.h res.h restypes.h stat.h \
	 stdio.h stdlib.h string.h types.h

restypes.obj: datapath.h dbg.h lg.h lgalt.h libdbg.h malloc.h memall.h \
	 res.h restypes.h stdio.h stdlib.h types.h
