DEBUG_ON = 1
SPEW_ON = 1
WARN_ON = 1
DBG_ON = 1


COMPILER = watcom
SRCDIR = n:\project\lib\src\ui
RELDIR = n:\project\lib\release
BUILD_DIR = N:\project\lib\build\ui

OBJDIR = obj
BINDIR =
LIBDIR = $(RELDIR)
INCDIR = $(RELDIR)
INCPATH = . $(SRCDIR) $(RELDIR)
LIBPATH = . $(RELDIR)
PROG_TARGETS = uitest
LIB_TARGETS =  ui
HDR_TARGETS =  
INST_TARGETS = ui

UI_OBJS = gadgets.obj region.obj cursors.obj event.obj slab.obj \
        pushbutt.obj butarray.obj qboxgadg.obj textgadg.obj slider.obj \
        texttool.obj tngfont.obj tng.obj  \
        tngqbox.obj tngtextg.obj tngpushb.obj tngslidr.obj tngbarry.obj
UI_HDRS = gadgets.h region.h cursors.h event.h slab.h pushbutt.h \
        butarray.h qboxgadg.h textgadg.h slider.h texttool.h \
        tngqbox.h tngtextg.h tngpushb.h tngslidr.h tngbarry.h tng.h \
        tngapp.h
UI_LIBS = 

UITEST_OBJS = uitest.obj
UITEST_LIBS = _lg.lib _2d.lib _input.lib _res.lib _fix.lib \
        _mprintf.lib dstruct.lib ui.lib


!include $(INIT)/make.rc

### OPUS MKMF:  Do not remove this line!  Automatic dependencies follow.

