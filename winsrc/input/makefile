

COMPILER = watcom
SRCDIR = n:\project\lib\src\input
RELDIR = n:\project\lib\release
BUILD_DIR = N:\project\lib\build\input

OBJDIR = obj
BINDIR =
LIBDIR = $(RELDIR)
INCDIR = $(RELDIR)
INCPATH = . $(SRCDIR) $(RELDIR)
LIBPATH = . $(RELDIR)
PROG_TARGETS = 
LIB_TARGETS = input
HDR_TARGETS =  
INST_TARGETS = input

INPUT_OBJS = mouse.obj  mousedev.obj kbclose.obj kbcstat.obj \
        kbd.obj kbflush.obj kbgen.obj \
        kbgflag.obj kbgstat.obj \
        kbinit.obj \
        kblnext.obj \
        kbnext.obj kbphnd.obj kbrhnd.obj \
        kbsflag.obj kbssig.obj kbsstat.obj \
        kbstate.obj kbcook.obj kbasctab.obj  joyflat.obj kbbflush.obj


INPUT_HDRS = mouse.h kb.h kbcook.h joystick.h


MORTIMER_OBJS = mousetst.obj
MORTIMER_LIBS = input.lib lg.lib mprintf.lib

JOYTEST_OBJS = joytest.obj
JOYTEST_LIBS = input.lib


!include $(INIT)/make.rc


kb.h: kbdecl.h kbarray.inc kbd.inc kberr.inc kbflag.inc \
      kblow.inc kbscan.inc kbstate.inc
        @rm $(@,F)
        !foreach FILE $(^, M\.h)
          @cat $(FILE) >>$(@,F)
        !end
        !foreach FILE $(^, M\.inc)
          @inc2h $(FILE) $(@,F)
        !end




### OPUS MKMF:  Do not remove this line!  Automatic dependencies follow.

mouse.obj: dbg.h dos.h error.h fix.h  i86.h lg.h lgalt.h \
	 libdbg.h malloc.h math.h memall.h mouse.h stdarg.h stdio.h types.h

mousetst.obj: dbg.h error.h fix.h  lg.h lgalt.h libdbg.h \
	 malloc.h math.h memall.h mouse.h stdarg.h stdio.h stdlib.h types.h


kbclose.obj: dpmi.inc kbd.inc kberr.inc kbhnd.inc kblow.inc type.inc

kbcstat.obj: kbd.inc kblow.inc type.inc

kbd.obj: kbd.inc kblow.inc

kbflush.obj: kbd.inc kblow.inc type.inc

kbgen.obj: kbreg.inc

kbgflag.obj: kbd.inc kblow.inc type.inc

kbgstat.obj: kbd.inc kblow.inc type.inc

kbinit.obj: dpmi.inc kbarray.inc kbd.inc kberr.inc kbhnd.inc kbint.inc \
	 kblow.inc type.inc

kblnext.obj: kbd.inc kbflag.inc kblow.inc type.inc

kbnext.obj: kbd.inc kbflag.inc kblow.inc type.inc

kbphnd.obj: kbarray.inc kbd.inc kbflag.inc kblow.inc kbreg.inc kbscan.inc \
	 type.inc

kbsflag.obj: kbd.inc kblow.inc

kbssig.obj: kbd.inc kblow.inc type.inc

kbsstat.obj: kbd.inc kblow.inc type.inc

kbstate.obj: kbarray.inc kbd.inc kblow.inc type.inc


joytest.obj: joystick.h stdio.h stdlib.h types.h


