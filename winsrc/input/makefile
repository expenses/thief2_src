#
# $Source: x:/prj/tech/winsrc/input/RCS/makefile $
# $Revision: 1.8 $
# $Author: kaboom $
# $Date: 1993/11/19 03:02:01 $
#

DEBUG_ON=1

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
LIB_TARGETS = input
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

!include $(INIT)/make.rc

kb.h: kbdecl.h kbarray.inc kbd.inc kberr.inc kbflag.inc \
        kblow.inc kbscan.inc kbstate.inc
        rm -f junk
        @echo cat $^ \>junk
        !foreach FILE $(^,M\.h)
          @cat $(FILE) >>junk
        !end
        !foreach FILE $(^,M\.inc)
          @inc2h $(FILE) junk
        !end
        @echo \#ifndef __KB_H >$(@,F)
        @echo \#define __KB_H >>$(@,F)
        sed '/\/\/.*$$/d' junk >>$(@,F)
        @echo \#endif /* !__KB_H */>>$(@,F)

### OPUS MKMF:  Do not remove this line!  Automatic dependencies follow.

kbasctab.obj: ctype.h dbg.h error.h kb.h kbcook.h lg.h lgalt.h libdbg.h \
	 malloc.h memall.h stdio.h types.h

kbbflush.obj: type.inc

kbclose.obj: dpmi.inc kbd.inc kberr.inc kbhnd.inc kblow.inc type.inc

kbcook.obj: ctype.h dbg.h error.h kb.h kbcook.h lg.h lgalt.h libdbg.h \
	 malloc.h memall.h stdio.h types.h

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

kbrhnd.obj: kbarray.inc kbflag.inc kblow.inc kbreg.inc kbscan.inc type.inc

kbsflag.obj: kbd.inc kblow.inc

kbssig.obj: kbd.inc kblow.inc type.inc

kbsstat.obj: kbd.inc kblow.inc type.inc

kbstate.obj: kbarray.inc kbd.inc kblow.inc type.inc

mouse.obj: dbg.h dos.h error.h i86.h lg.h lgalt.h libdbg.h malloc.h \
	 memall.h mouse.h stdio.h types.h

