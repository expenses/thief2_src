#
# $Source: x:/prj/tech/libsrc/ui/RCS/makefile $
# $Revision: 1.27 $
# $Author: mahk $
# $Date: 1994/07/09 18:56:31 $

DEBUG_ON = 1
SPEW_ON = 1
WARN_ON = 1
DBG_ON = 1

COMPILER = watcom
SRCDIR = r:\prj\lib\src\ui
RELDIR = r:\prj\lib\release
BUILD_DIR = r:\prj\lib\build\ui

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
CURDRW.C_CFLAGS=-zu
UI_OBJS = gadgets.obj region.obj cursors.obj curdrw.obj event.obj slab.obj \
        pushbutt.obj butarray.obj qboxgadg.obj textgadg.obj slider.obj \
        texttool.obj tngfont.obj tng.obj  hotkey.obj\
        tngqbox.obj tngtextg.obj tngpushb.obj tngslidr.obj tngbarry.obj \
        plain.obj tngplain.obj menu.obj tngmenu.obj
UI_HDRS = gadgets.h region.h cursors.h event.h slab.h pushbutt.h \
        butarray.h qboxgadg.h textgadg.h slider.h texttool.h \
        tngqbox.h tngtextg.h tngpushb.h tngslidr.h tngbarry.h tng.h \
        tngapp.h hotkey.h plain.h tngplain.h menu.h tngmenu.h \
        curdat.h curtyp.h

UITEST_OBJS = uitest.obj
UITEST_LIBS = _lg.lib _2d.lib _input.lib _res.lib _fix.lib \
        _mprintf.lib _dstruct.lib ui.lib _fault.lib

!include $(INIT)/make.rc

### OPUS MKMF:  Do not remove this line!  Automatic dependencies follow.

butarray.obj objo\butarray.obj objd\butarray.obj: 2d.h 2dres.h array.h butarray.h ctype.h cursors.h datapath.h \
	 dbg.h dbgmacro.h error.h event.h fcntl.h fix.h gadgets.h io.h kb.h \
	 kbcook.h lg.h lgalt.h malloc.h memall.h memmacro.h mouse.h rect.h \
	 region.h res.h restypes.h slab.h slider.h slist.h stat.h stdio.h \
	 stdlib.h string.h tng.h tngapp.h tngbarry.h tngslidr.h types.h

curdrw.obj objo\curdrw.obj objd\curdrw.obj: 2d.h _2d.h _3d.h _babl2.h _dstruct.h _edms.h _lg.h _mouse.h \
	 _network.h _parse.h _res.h _sound.h _testlib.h _ui.h array.h \
	 curdat.h cursors.h curtyp.h dbg.h dbgmacro.h error.h fix.h lg.h \
	 lgalt.h libdbg.h malloc.h memall.h memmacro.h mouse.h rect.h \
	 region.h slab.h slist.h stdio.h stdlib.h string.h types.h

cursors.obj objo\cursors.obj objd\cursors.obj: 2d.h _2d.h _3d.h _babl2.h _dstruct.h _edms.h _lg.h _mouse.h \
	 _network.h _parse.h _res.h _sound.h _testlib.h _ui.h array.h \
	 cursors.h curtyp.h dbg.h dbgmacro.h error.h fix.h lg.h lgalt.h \
	 libdbg.h malloc.h memall.h memmacro.h mouse.h rect.h region.h \
	 slab.h slist.h stdio.h stdlib.h string.h types.h

event.obj objo\event.obj objd\event.obj: 2d.h _ui.h array.h ctype.h cursors.h dbg.h dbgmacro.h error.h \
	 event.h fix.h kb.h kbcook.h lg.h lgalt.h malloc.h memall.h \
	 memmacro.h mouse.h rect.h region.h slab.h slist.h stdio.h stdlib.h \
	 types.h

gadgets.obj objo\gadgets.obj objd\gadgets.obj: 2d.h 2dres.h _ui.h array.h ctype.h cursors.h datapath.h dbg.h \
	 dbgmacro.h error.h event.h fix.h gadgets.h kb.h kbcook.h lg.h \
	 lgalt.h malloc.h memall.h memmacro.h mouse.h rect.h region.h res.h \
	 restypes.h slab.h slist.h stdio.h stdlib.h string.h tng.h tngapp.h \
	 types.h

hotkey.obj objo\hotkey.obj objd\hotkey.obj: _dstruct.h _ui.h array.h ctype.h dbg.h dbgmacro.h error.h \
	 hash.h hotkey.h kb.h kbcook.h lg.h lgalt.h malloc.h memall.h \
	 memmacro.h stdio.h string.h types.h

hottest.obj objo\hottest.obj objd\hottest.obj: _dstruct.h array.h ctype.h dbg.h dbgmacro.h error.h hash.h \
	 hotkey.h kb.h kbcook.h lg.h lgalt.h malloc.h memall.h memmacro.h \
	 stdio.h stdlib.h types.h

menu.obj objo\menu.obj objd\menu.obj: 2d.h 2dres.h _dstruct.h _ui.h array.h ctype.h cursors.h \
	 datapath.h dbg.h dbgmacro.h error.h event.h fix.h gadgets.h hash.h \
	 hotkey.h kb.h kbcook.h lg.h lgalt.h llist.h malloc.h memall.h \
	 memmacro.h menu.h mouse.h rect.h region.h res.h restypes.h slab.h \
	 slist.h stdio.h stdlib.h string.h texttool.h tng.h tngapp.h \
	 tngmenu.h types.h

monofont.obj objo\monofont.obj objd\monofont.obj: mprintf.h rect.h string.h texttool.h

plain.obj objo\plain.obj objd\plain.obj: 2d.h 2dres.h _ui.h array.h cursors.h datapath.h dbg.h dbgmacro.h \
	 error.h event.h fix.h gadgets.h lg.h lgalt.h malloc.h memall.h \
	 memmacro.h mouse.h plain.h rect.h region.h res.h restypes.h slab.h \
	 slist.h stdio.h stdlib.h tng.h tngapp.h tngplain.h types.h

pushbutt.obj objo\pushbutt.obj objd\pushbutt.obj: 2d.h 2dres.h _ui.h array.h cursors.h datapath.h dbg.h \
	 dbgmacro.h error.h event.h fix.h gadgets.h lg.h lgalt.h malloc.h \
	 memall.h memmacro.h mouse.h pushbutt.h rect.h region.h res.h \
	 restypes.h slab.h slist.h stdio.h stdlib.h string.h tng.h tngapp.h \
	 tngpushb.h types.h

qboxgadg.obj objo\qboxgadg.obj objd\qboxgadg.obj: 2d.h 2dres.h _ui.h array.h ctype.h cursors.h datapath.h dbg.h \
	 dbgmacro.h error.h event.h fcntl.h fix.h gadgets.h io.h kb.h \
	 kbcook.h lg.h lgalt.h malloc.h memall.h memmacro.h mouse.h \
	 qboxgadg.h rect.h region.h res.h restypes.h slab.h slist.h stat.h \
	 stdio.h stdlib.h string.h tng.h tngapp.h tngqbox.h types.h

realfont.obj objo\realfont.obj objd\realfont.obj: 2d.h fakefont.h fcntl.h fix.h io.h mprintf.h rect.h stat.h \
	 stdlib.h string.h texttool.h types.h

region.obj objo\region.obj objd\region.obj: 2d.h 2dres.h _ui.h array.h cursors.h datapath.h dbg.h \
	 dbgmacro.h error.h event.h fix.h gadgets.h lg.h lgalt.h malloc.h \
	 memall.h memmacro.h mouse.h rect.h region.h res.h restypes.h \
	 slab.h slist.h stdio.h stdlib.h tng.h tngapp.h types.h

slab.obj objo\slab.obj objd\slab.obj: 2d.h array.h cursors.h dbg.h dbgmacro.h error.h fix.h lg.h \
	 lgalt.h malloc.h memall.h memmacro.h mouse.h rect.h region.h \
	 slab.h slist.h stdio.h types.h

slider.obj objo\slider.obj objd\slider.obj: 2d.h 2dres.h _ui.h array.h cursors.h datapath.h dbg.h \
	 dbgmacro.h error.h event.h fix.h gadgets.h lg.h lgalt.h malloc.h \
	 memall.h memmacro.h mouse.h rect.h region.h res.h restypes.h \
	 slab.h slider.h slist.h stdio.h stdlib.h string.h tng.h tngapp.h \
	 tngslidr.h types.h

textgadg.obj objo\textgadg.obj objd\textgadg.obj: 2d.h 2dres.h _ui.h array.h cursors.h datapath.h dbg.h \
	 dbgmacro.h error.h event.h fakefont.h fix.h gadgets.h lg.h lgalt.h \
	 malloc.h memall.h memmacro.h mouse.h rect.h region.h res.h \
	 restypes.h slab.h slider.h slist.h stdio.h stdlib.h string.h \
	 textgadg.h texttool.h tng.h tngapp.h tngslidr.h tngtextg.h types.h

texttool.obj objo\texttool.obj objd\texttool.obj: ctype.h dbg.h dbgmacro.h fakefont.h lg.h lgalt.h malloc.h \
	 memall.h memmacro.h mprintf.h rect.h stdio.h string.h texttool.h \
	 tngfont.h types.h

tng.obj objo\tng.obj objd\tng.obj: 2d.h 2dres.h _ui.h datapath.h dbg.h dbgmacro.h error.h fix.h \
	 gadgets.h lg.h lgalt.h malloc.h memall.h memmacro.h rect.h \
	 region.h res.h restypes.h slist.h stdio.h stdlib.h tng.h tngapp.h \
	 types.h

tngbarry.obj objo\tngbarry.obj objd\tngbarry.obj: 2d.h 2dres.h _ui.h barrykey.h ctype.h datapath.h dbg.h \
	 dbgmacro.h error.h fcntl.h fix.h gadgets.h io.h kb.h kbcook.h \
	 keydefs.h lg.h lgalt.h malloc.h memall.h memmacro.h mprintf.h \
	 rect.h region.h res.h restypes.h slist.h stat.h stdio.h stdlib.h \
	 tng.h tngapp.h tngbarry.h tngslidr.h types.h

tngfont.obj objo\tngfont.obj objd\tngfont.obj: 2d.h 2dres.h datapath.h dbg.h dbgmacro.h error.h fakefont.h \
	 fcntl.h fix.h gadgets.h io.h lg.h lgalt.h malloc.h memall.h \
	 memmacro.h mprintf.h rect.h region.h res.h restypes.h slist.h \
	 stat.h stdio.h stdlib.h string.h texttool.h tng.h tngapp.h \
	 tngfont.h types.h

tngmenu.obj objo\tngmenu.obj objd\tngmenu.obj: 2d.h 2dres.h _dstruct.h _ui.h array.h barrykey.h ctype.h \
	 datapath.h dbg.h dbgmacro.h error.h fix.h gadgets.h hash.h \
	 hotkey.h kb.h kbcook.h keydefs.h lg.h lgalt.h llist.h malloc.h \
	 memall.h memmacro.h rect.h region.h res.h restypes.h slist.h \
	 stdio.h stdlib.h string.h texttool.h tng.h tngapp.h tngfont.h \
	 tngmenu.h types.h

tngplain.obj objo\tngplain.obj objd\tngplain.obj: 2d.h 2dres.h _ui.h datapath.h dbg.h dbgmacro.h error.h fix.h \
	 gadgets.h lg.h lgalt.h malloc.h memall.h memmacro.h rect.h \
	 region.h res.h restypes.h slist.h stdio.h stdlib.h tng.h tngapp.h \
	 tngplain.h types.h

tngpushb.obj objo\tngpushb.obj objd\tngpushb.obj: 2d.h 2dres.h _ui.h datapath.h dbg.h dbgmacro.h error.h \
	 fcntl.h fix.h gadgets.h io.h lg.h lgalt.h malloc.h memall.h \
	 memmacro.h rect.h region.h res.h restypes.h slist.h stat.h stdio.h \
	 stdlib.h tng.h tngapp.h tngpushb.h types.h

tngqbox.obj objo\tngqbox.obj objd\tngqbox.obj: 2d.h 2dres.h _ui.h ctype.h datapath.h dbg.h dbgmacro.h error.h \
	 fcntl.h fix.h gadgets.h io.h lg.h lgalt.h malloc.h math.h memall.h \
	 memmacro.h rect.h region.h res.h restypes.h slist.h stat.h stdio.h \
	 stdlib.h string.h texttool.h tng.h tngapp.h tngqbox.h tngslidr.h \
	 tngtextg.h types.h

tngslidr.obj objo\tngslidr.obj objd\tngslidr.obj: 2d.h 2dres.h _ui.h datapath.h dbg.h dbgmacro.h error.h fix.h \
	 gadgets.h lg.h lgalt.h malloc.h memall.h memmacro.h rect.h \
	 region.h res.h restypes.h slist.h stdio.h stdlib.h tng.h tngapp.h \
	 tngslidr.h types.h

tngtextg.obj objo\tngtextg.obj objd\tngtextg.obj: 2d.h 2dres.h _ui.h datapath.h dbg.h dbgmacro.h error.h \
	 fakefont.h fcntl.h fix.h gadgets.h io.h lg.h lgalt.h malloc.h \
	 memall.h memmacro.h rect.h region.h res.h restypes.h slist.h \
	 stat.h stdio.h stdlib.h texttool.h tng.h tngapp.h tngfont.h \
	 tngslidr.h tngtextg.h types.h

uitest.obj objo\uitest.obj objd\uitest.obj: 2d.h 2dres.h _dstruct.h array.h butarray.h ctype.h cursors.h \
	 data\gamescr.h datapath.h dbg.h dbgmacro.h error.h event.h fault.h \
	 fix.h gadgets.h hash.h hotkey.h io.h kb.h kbcook.h keydefs.h lg.h \
	 lgalt.h malloc.h memall.h memmacro.h menu.h mouse.h mprintf.h \
	 plain.h pushbutt.h qboxgadg.h rect.h region.h res.h restypes.h \
	 slab.h slider.h slist.h stat.h stdio.h stdlib.h string.h \
	 textgadg.h texttool.h tng.h tngapp.h tngbarry.h tngplain.h \
	 tngpushb.h tngqbox.h tngslidr.h tngtextg.h types.h
