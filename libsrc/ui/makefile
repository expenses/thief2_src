DEBUG_ON = 1
SPEW_ON = 1
WARN_ON = 1
DBG_ON = 1

PROG_TARGETS =  regtest arraytst gadtest texttest # citadel
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
        array.obj pushbutt.obj butarray.obj slider.obj textgadg.obj \
        qboxgadg.obj bugtrak.obj cursors.obj      
GADTEST_LIBS = lg.lib mprintf.lib 2d.lib res.lib kb.lib mouse.lib

TEXTTEST_OBJS = texttest.obj gadgets.obj region.obj slider.obj \
        textgadg.obj rect.obj pushbutt.obj \
        event.obj array.obj
TEXTTEST_LIBS = lg.lib mprintf.lib 2d.lib res.lib kb.lib mouse.lib

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
        @echo ROOTSRCDIR = $(ROOTSRCDIR)
        @echo SRCDIR = $(SRCDIR)
        @echo LIBPATH = $(LIBPATH)

### OPUS MKMF:  Do not remove this line!  Automatic dependencies follow.

butarray.obj: 2d.h array.h assert.h base.h butarray.h conio.h datapath.h \
	 dbg.h dos.h error.h event.h fix.h gadgets.h gfx.h grindex.h i86.h \
	 lg.h lgalt.h libdbg.h malloc.h math.h memall.h mouse.h projdbg.h \
	 rect.h region.h res.h resource.h restypes.h stdarg.h stdio.h \
	 stdlib.h string.h types.h

gadgets.obj: 2d.h array.h assert.h base.h conio.h datapath.h dbg.h dos.h \
	 error.h event.h fix.h gadgets.h gfx.h grindex.h i86.h lg.h lgalt.h \
	 libdbg.h malloc.h math.h memall.h mouse.h projdbg.h rect.h \
	 region.h res.h resource.h restypes.h stdarg.h stdio.h stdlib.h \
	 string.h types.h

gadtest.obj: 2d.h array.h assert.h base.h butarray.h conio.h datapath.h \
	 dbg.h dos.h error.h event.h fix.h gadgets.h gfx.h grindex.h i86.h \
	 io.h kb.h kbcook.h lg.h lgalt.h libdbg.h malloc.h math.h memall.h \
	 mouse.h mprintf.h projdbg.h pushbutt.h rect.h region.h res.h \
	 resource.h restypes.h slider.h stat.h stdarg.h stdio.h stdlib.h \
	 string.h types.h

pushbutt.obj: 2d.h array.h assert.h base.h conio.h datapath.h dbg.h dos.h \
	 error.h event.h fix.h gadgets.h grindex.h i86.h lg.h lgalt.h \
	 libdbg.h malloc.h math.h memall.h mouse.h projdbg.h pushbutt.h \
	 rect.h region.h res.h resource.h restypes.h stdarg.h stdio.h \
	 stdlib.h string.h types.h

slider.obj: 2d.h array.h assert.h base.h conio.h datapath.h dbg.h dos.h \
	 error.h event.h fix.h gadgets.h grindex.h i86.h lg.h lgalt.h \
	 libdbg.h malloc.h math.h memall.h mouse.h projdbg.h pushbutt.h \
	 rect.h region.h res.h resource.h restypes.h slider.h stdarg.h \
	 stdio.h stdlib.h string.h types.h
