DEBUG_ON = 1
SPEW_ON = 1
WARN_ON = 1
DBG_ON = 1

PROG_TARGETS =  regtest arraytst gadtest texttest workgadg testfont
LIB_TARGETS = ui
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
        array.obj pushbutt.obj butarray.obj slider.obj slab.obj \
        qboxgadg.obj cursors.obj bugtrak.obj      
GADTEST_LIBS = _lg.lib _mprintf.lib _2d.lib _res.lib _kb.lib _mouse.lib

TEXTTEST_OBJS = texttest.obj gadgets.obj region.obj slider.obj \
        textgadg.obj rect.obj pushbutt.obj monofont.obj \
        event.obj array.obj cursors.obj slab.obj texttool.obj realfont.obj
TEXTTEST_LIBS = _2d.lib _res.lib _kb.lib _mouse.lib _mprintf.lib _lg.lib

WORKGADG_OBJS = workgadg.obj gadgets.obj region.obj cursors.obj slab.obj \
        bugtrak.obj qboxgadg.obj pushbutt.obj slider.obj butarray.obj \
        musicai.obj array.obj rect.obj textgadg.obj event.obj
WORKGADG_LIBS = _lg.lib _mprintf.lib _2d.lib _res.lib _kb.lib _mouse.lib

UI_OBJS = gadgets.obj region.obj cursors.obj event.obj slab.obj \
        pushbutt.obj butarray.obj qboxgadg.obj textgadg.obj slider.obj \
        rect.obj array.obj texttool.obj realfont.obj monofont.obj
UI_HDRS = gadgets.h region.h cursors.h event.h slab.h pushbutt.h \
        butarray.h qboxgadg.h textgadg.h slider.h rect.h array.h \
        texttool.h 
UI_LIBS = 

TESTFONT_OBJS = testfont.obj 
TESTFONT_LIBS = 2d.lib mprintf.lib lg.lib

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

