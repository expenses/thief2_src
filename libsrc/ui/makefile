DEBUG_ON = 1
SPEW_ON = 1
WARN_ON = 1
DBG_ON = 1

PROG_TARGETS =  regtest arraytst gadtest # citadel
LIB_TARGETS =
INST_TARGETS =

# The following are groups of objs, not really programs to be compiled

XEMU_OBJS = shodan.obj critai.obj physics.obj combat.obj mainui.obj

MAHK_OBJS = manta.obj gamesys.obj bablstuf.obj grunge.obj

DC_OBJS = rend.obj world.obj editor.obj cspace.obj

# Here's the list for compiling whole programs

CITADEL_OBJS = main.obj init.obj $(XEMU_OBJS) $(DC_OBJS) $(MAHK_OBJS)
CITADEL_LIBS = $(LG_LIBS) 

REGTEST_OBJS = region.obj rect.obj test.obj
REGTEST_LIBS = lg.lib mprintf.lib

ARRAYTST_OBJS = array.obj arraytst.obj
ARRAYTST_LIBS = lg.lib mprintf.lib

GADTEST_OBJS = gadgets.obj rect.obj gadtest.obj region.obj event.obj \
        array.obj
GADTEST_LIBS = lg.lib mprintf.lib 2d.lib res.lib mouse.lib kb.lib

# Libraries

!if $(DEBUG_ON)
LG_LIBS	= _lg.lib _mprintf.lib _fix.lib _2d.lib _3d.lib _range.lib  _mouse.lib \
        _edms.lib  _res.lib
!else
LG_LIBS	= lg.lib mprintf.lib fix.lib 2d.lib 3d.lib range.lib mouse.lib \
        edms.lib res.lib
!endif

!include $(INIT)\make.rc

!if !defined(NONET)
# !include makefile.rcs
# !include inc/makefile.rcs
!endif

blah .PHONY:
        @echo SRCDIR = $(SRCDIR)
        @echo LIBPATH = $(LIBPATH)

### OPUS MKMF:  Do not remove this line!  Automatic dependencies follow.

array.obj: array.h dbg.h error.h lg.h lgalt.h libdbg.h malloc.h memall.h \
	 memasm.h stdio.h string.h types.h

gadgets.obj: 2d.h assert.h base.h conio.h dbg.h dos.h error.h fix.h \
	 gadgets.h grindex.h i86.h lg.h lgalt.h libdbg.h malloc.h math.h \
	 memall.h memasm.h projdbg.h rect.h region.h stdarg.h stdio.h \
	 stdlib.h string.h types.h

rect.obj: assert.h base.h conio.h dos.h i86.h rect.h stdio.h stdlib.h \
	 string.h types.h

region.obj: assert.h base.h conio.h dbg.h dos.h error.h i86.h lg.h lgalt.h \
	 libdbg.h malloc.h memall.h memasm.h projdbg.h rect.h region.h \
	 stdio.h stdlib.h string.h types.h
