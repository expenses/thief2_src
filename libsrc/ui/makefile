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
REGTEST_OBJS = lg.lib mprintf.lib

ARRAYTST_OBJS = array.obj arraytst.obj
ARRAYTST_LIBS = lg.lib mprintf.lib

GADTEST_OBJS = gadgets.obj rect.obj gadtest.obj region.obj 
GADTEST_LIBS = lg.lib mprintf.lib 2d.lib res.lib

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
!include makefile.rcs
!include ../inc/makefile.rcs
!endif

### OPUS MKMF:  Do not remove this line!  Automatic dependencies follow.

