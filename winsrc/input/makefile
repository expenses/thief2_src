#
# $Source: x:/prj/tech/winsrc/input/RCS/makefile $
# $Revision: 1.11 $
# $Author: kaboom $
# $Date: 1994/02/12 18:36:20 $
#

DEBUG_ON= 1

COMPILER= watcom
SRCDIR= n:\project\lib\src\input
RELDIR= n:\project\lib\release
BUILD_DIR= n:\project\lib\build\input

OBJDIR= objo
BINDIR=
LIBDIR= $(RELDIR)
INCDIR= $(RELDIR)
INCPATH= . ..\dpmi $(SRCDIR) $(RELDIR)
LIBPATH= . $(RELDIR)
LIB_TARGETS= input
INST_TARGETS= input

INPUT_OBJS= \
        kbarray.obj \
        kbbflush.obj kbcstat.obj \
        kbflush.obj \
        kblow.obj kbgen.obj \
        kbgflag.obj kbglob.obj kbgstat.obj \
        kbint.obj \
        kblnext.obj \
        kbnext.obj \
        kbqueue.obj \
        kbphnd.obj kbrhnd.obj \
        kbsflag.obj kbshut.obj \
        kbssig.obj kbsstat.obj \
        kbstart.obj \
        kbstate.obj kbcook.obj kbasctab.obj \
        joyflat.obj joystick.obj ngpjoy.obj basjoy.obj \
        mouse.obj  mousedev.obj
INPUT_HDRS= mouse.h kb.h kbcook.h joystick.h

!include $(INIT)/make.rc

kb.h: kbs.h kbdecl.h kbarray.inc kbd.inc kberr.inc kbflag.inc \
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

basjoy.obj: _input.h basjoy.h dbg.h dbgmacro.h stdio.h types.h

joystick.obj: basjoy.h joystick.h ngpjoy.h

kbasctab.obj: ctype.h dbg.h dbgmacro.h error.h kb.h kbcook.h lg.h lgalt.h \
	 malloc.h memall.h memmacro.h stdio.h types.h

kbbflush.obj: type.inc

kbclose.obj: dpmi.inc kbd.inc kberr.inc kbhnd.inc kblow.inc macros.inc \
	 type.inc types.inc

kbcook.obj: ctype.h dbg.h dbgmacro.h error.h kb.h kbcook.h lg.h lgalt.h \
	 malloc.h memall.h memmacro.h stdio.h types.h

kbcstat.obj: kbd.inc kblow.inc type.inc

kbd.obj: kbd.inc kblow.inc

kbflush.obj: kbd.inc kblow.inc type.inc

kbgen.obj: kbreg.inc

kbgflag.obj: kbd.inc kblow.inc type.inc

kbgstat.obj: kbd.inc kblow.inc type.inc

kbinit.obj: dpmi.inc kbarray.inc kbclose.asm kbd.inc kberr.inc kbflag.inc \
	 kbhnd.inc kbint.inc kblow.inc kbphnd.asm kbreg.inc kbrhnd.asm \
	 kbscan.inc macros.inc type.inc types.inc

kblnext.obj: kbd.inc kbflag.inc kblow.inc type.inc

kblow.obj: dpmi.h

kbnext.obj: kbd.inc kbflag.inc kblow.inc type.inc

kbphnd.obj: kbarray.inc kbd.inc kbflag.inc kblow.inc kbreg.inc kbscan.inc \
	 macros.inc type.inc types.inc

kbqueue.obj: kbs.h

kbrhnd.obj: kbarray.inc kbflag.inc kblow.inc kbreg.inc kbscan.inc type.inc

kbsflag.obj: kbd.inc kblow.inc

kbshut.obj: dpmi.h kbint.h kblow.h

kbssig.obj: kbd.inc kblow.inc type.inc

kbsstat.obj: kbd.inc kblow.inc type.inc

kbstart.obj: dpmi.h kbarray.h kbglob.h kbint.h kblow.h kbqueue.h kbs.h \
	 string.h

kbstate.obj: kbarray.inc kbd.inc kblow.inc type.inc

mouse.obj: _mouse.h dbg.h dbgmacro.h dos.h dpmi.h error.h i86.h lg.h \
	 lgalt.h malloc.h memall.h memmacro.h mouse.h stdio.h stdlib.h \
	 types.h

ngpjoy.obj: _input.h dbg.h dbgmacro.h dos.h i86.h ngpjoy.h stdio.h types.h
