#
# $Source: x:/prj/tech/winsrc/input/RCS/makefile $
# $Revision: 1.15 $
# $Author: kaboom $
# $Date: 1994/08/15 16:50:27 $
#

DEBUG_ON= 1

COMPILER= watcom
SRCDIR= r:\prj\lib\src\input
RELDIR= r:\prj\lib\rel
BUILD_DIR= r:\prj\lib\build\input

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
        leds.obj \
        joyflat.obj joystick.obj ngpjoy.obj basjoy.obj \
        mouse.obj  mousedev.obj
INPUT_HDRS= mouse.h kb.h kbcook.h joystick.h mousevel.h

!include $(INIT)/make.rc

kb.h: kbs.h kbdecl.h kbscan.h kbarray.inc kbd.inc kberr.inc kbflag.inc \
        kblow.inc kbstate.inc
        rm -f junk
        @echo cat $^ \>junk
        !foreach FILE $(^,M\.h)
          @cstrip $(FILE) >>junk
        !end
        !foreach FILE $(^,M\.inc)
          @inc2h $(FILE) junk
        !end
        @echo \#ifndef __KB_H >$(@,F)
        @echo \#define __KB_H >>$(@,F)
        sed '/\/\/.*$$/d' junk >>$(@,F)
        @echo \#endif /* !__KB_H */>>$(@,F)

### OPUS MKMF:  Do not remove this line!  Automatic dependencies follow.

basjoy.obj objo\basjoy.obj objd\basjoy.obj: _input.h basjoy.h dbg.h dbgmacro.h stdio.h types.h

joystick.obj objo\joystick.obj objd\joystick.obj: basjoy.h joystick.h ngpjoy.h

kbasctab.obj objo\kbasctab.obj objd\kbasctab.obj: ctype.h error.h kb.h kbcook.h

kbbflush.obj objo\kbbflush.obj objd\kbbflush.obj: type.inc

kbcook.obj objo\kbcook.obj objd\kbcook.obj: ctype.h dbg.h dbgmacro.h error.h kb.h kbcook.h kbmod.h kbscan.h \
	 lg.h lgalt.h malloc.h memall.h memmacro.h stdio.h types.h

kbcstat.obj objo\kbcstat.obj objd\kbcstat.obj: kbd.inc kblow.inc type.inc

kbflush.obj objo\kbflush.obj objd\kbflush.obj: kbd.inc kblow.inc type.inc

kbgen.obj objo\kbgen.obj objd\kbgen.obj: kbreg.inc

kbgflag.obj objo\kbgflag.obj objd\kbgflag.obj: kbd.inc kblow.inc type.inc

kbgstat.obj objo\kbgstat.obj objd\kbgstat.obj: kbd.inc kblow.inc type.inc

kblnext.obj objo\kblnext.obj objd\kblnext.obj: kbd.inc kbflag.inc kblow.inc type.inc

kblow.obj objo\kblow.obj objd\kblow.obj: dpmi.h i86.h

kbnext.obj objo\kbnext.obj objd\kbnext.obj: kbd.inc kbflag.inc kblow.inc type.inc

kbphnd.obj objo\kbphnd.obj objd\kbphnd.obj: fault.inc kbarray.inc kbd.inc kbflag.inc kblow.inc kbreg.inc \
	 kbscan.inc macros.inc type.inc types.inc

kbqueue.obj objo\kbqueue.obj objd\kbqueue.obj: kbs.h

kbrhnd.obj objo\kbrhnd.obj objd\kbrhnd.obj: kbarray.inc kbflag.inc kblow.inc kbreg.inc kbscan.inc type.inc

kbsflag.obj objo\kbsflag.obj objd\kbsflag.obj: kbd.inc kblow.inc

kbshut.obj objo\kbshut.obj objd\kbshut.obj: dpmi.h i86.h kbint.h kblow.h

kbssig.obj objo\kbssig.obj objd\kbssig.obj: kbarray.h

kbsstat.obj objo\kbsstat.obj objd\kbsstat.obj: kbd.inc kblow.inc type.inc

kbstart.obj objo\kbstart.obj objd\kbstart.obj: dpmi.h i86.h kbarray.h kbglob.h kbint.h kblow.h kbqueue.h \
	 kbs.h string.h

kbstate.obj objo\kbstate.obj objd\kbstate.obj: kbarray.inc kbd.inc kblow.inc type.inc

leds.obj objo\leds.obj objd\leds.obj: conio.h kbmod.h

mouse.obj objo\mouse.obj objd\mouse.obj: _mouse.h dbg.h dbgmacro.h dos.h dpmi.h error.h i86.h lg.h \
	 lgalt.h malloc.h memall.h memmacro.h mouse.h mousevel.h stdio.h \
	 stdlib.h types.h

ngpjoy.obj objo\ngpjoy.obj objd\ngpjoy.obj: _input.h dbg.h dbgmacro.h dos.h i86.h ngpjoy.h stdio.h types.h
