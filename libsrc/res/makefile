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

lzw.obj: _2d.h _3d.h _babl2.h _dstruct.h _edms.h _lg.h _mouse.h _network.h \
	 _parse.h _res.h _sound.h _testlib.h _ui.h dbg.h dbgmacro.h io.h \
	 lg.h lgalt.h libdbg.h lzw.h malloc.h memall.h memmacro.h stat.h \
	 stdio.h stdlib.h string.h types.h

res.obj: _2d.h _3d.h _babl2.h _dstruct.h _edms.h _lg.h _mouse.h _network.h \
	 _parse.h _res.h _sound.h _testlib.h _ui.h datapath.h dbg.h \
	 dbgmacro.h fcntl.h io.h lg.h lgalt.h libdbg.h lzw.h malloc.h \
	 memall.h memmacro.h memstat.h res.h restypes.h stat.h stdio.h \
	 stdlib.h string.h types.h

resbuild.obj: _2d.h _3d.h _babl2.h _dstruct.h _edms.h _lg.h _mouse.h \
	 _network.h _parse.h _res.h _sound.h _testlib.h _ui.h datapath.h \
	 dbg.h dbgmacro.h fcntl.h io.h lg.h lgalt.h libdbg.h lzw.h malloc.h \
	 memall.h memmacro.h res.h restypes.h stat.h stdio.h stdlib.h \
	 string.h types.h

resdump.obj: _2d.h _3d.h _babl2.h _dstruct.h _edms.h _lg.h _mouse.h \
	 _network.h _parse.h _res.h _sound.h _testlib.h _ui.h datapath.h \
	 dbg.h dbgmacro.h io.h lg.h lgalt.h libdbg.h malloc.h memall.h \
	 memmacro.h res.h restypes.h stat.h stdio.h stdlib.h string.h \
	 types.h

respack.obj: _2d.h _3d.h _babl2.h _dstruct.h _edms.h _lg.h _mouse.h \
	 _network.h _parse.h _res.h _sound.h _testlib.h _ui.h datapath.h \
	 dbg.h dbgmacro.h io.h lg.h lgalt.h libdbg.h malloc.h memall.h \
	 memmacro.h res.h restypes.h stat.h stdio.h stdlib.h string.h \
	 types.h

restest.obj: _2d.h _3d.h _babl2.h _dstruct.h _edms.h _lg.h _mouse.h \
	 _network.h _parse.h _res.h _sound.h _testlib.h _ui.h conio.h \
	 ctype.h datapath.h dbg.h dbgmacro.h fcntl.h io.h lg.h lgalt.h \
	 libdbg.h lzw.h malloc.h memall.h memmacro.h mprintf.h res.h \
	 restypes.h stat.h stdio.h stdlib.h string.h types.h

restypes.obj: _2d.h _3d.h _babl2.h _dstruct.h _edms.h _lg.h _mouse.h \
	 _network.h _parse.h _res.h _sound.h _testlib.h _ui.h datapath.h \
	 dbg.h dbgmacro.h lg.h lgalt.h libdbg.h malloc.h memall.h \
	 memmacro.h res.h restypes.h stdio.h stdlib.h types.h
