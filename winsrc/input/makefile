#
# $Source: x:/prj/tech/winsrc/input/RCS/makefile $
# $Revision: 1.13 $
# $Author: mahk $
# $Date: 1994/06/21 05:34:20 $
#

DEBUG_ON= 1

COMPILER= watcom
SRCDIR= n:\project\lib\src\input
RELDIR= R:\prj\lib\rel
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
INPUT_HDRS= mouse.h kb.h kbcook.h joystick.h mousevel.h

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

basjoy.obj objo\basjoy.obj objd\basjoy.obj: basjoy.h

joystick.obj objo\joystick.obj objd\joystick.obj: basjoy.h joystick.h ngpjoy.h

kbasctab.obj objo\kbasctab.obj objd\kbasctab.obj: ctype.h kb.h kbcook.h

kbclose.obj objo\kbclose.obj objd\kbclose.obj: kbd.inc kberr.inc kbhnd.inc kblow.inc

kbcook.obj objo\kbcook.obj objd\kbcook.obj: ctype.h kb.h kbcook.h

kbcstat.obj objo\kbcstat.obj objd\kbcstat.obj: kbd.inc kblow.inc

kbd.obj objo\kbd.obj objd\kbd.obj: kbd.inc kblow.inc

kbflush.obj objo\kbflush.obj objd\kbflush.obj: kbd.inc kblow.inc

kbgen.obj objo\kbgen.obj objd\kbgen.obj: kbreg.inc

kbgflag.obj objo\kbgflag.obj objd\kbgflag.obj: kbd.inc kblow.inc

kbgstat.obj objo\kbgstat.obj objd\kbgstat.obj: kbd.inc kblow.inc

kbinit.obj objo\kbinit.obj objd\kbinit.obj: kbarray.inc kbclose.asm kbd.inc kberr.inc kbflag.inc kbhnd.inc \
	 kbint.inc kblow.inc kbphnd.asm kbreg.inc kbrhnd.asm kbscan.inc

kblnext.obj objo\kblnext.obj objd\kblnext.obj: kbd.inc kbflag.inc kblow.inc

kbnext.obj objo\kbnext.obj objd\kbnext.obj: kbd.inc kbflag.inc kblow.inc

kbphnd.obj objo\kbphnd.obj objd\kbphnd.obj: kbarray.inc kbd.inc kbflag.inc kblow.inc kbreg.inc kbscan.inc

kbqueue.obj objo\kbqueue.obj objd\kbqueue.obj: kbs.h

kbrhnd.obj objo\kbrhnd.obj objd\kbrhnd.obj: kbarray.inc kbflag.inc kblow.inc kbreg.inc kbscan.inc

kbsflag.obj objo\kbsflag.obj objd\kbsflag.obj: kbd.inc kblow.inc

kbshut.obj objo\kbshut.obj objd\kbshut.obj: kbint.h kblow.h

kbssig.obj objo\kbssig.obj objd\kbssig.obj: kbarray.h

kbsstat.obj objo\kbsstat.obj objd\kbsstat.obj: kbd.inc kblow.inc

kbstart.obj objo\kbstart.obj objd\kbstart.obj: kbarray.h kbglob.h kbint.h kblow.h kbqueue.h kbs.h string.h

kbstate.obj objo\kbstate.obj objd\kbstate.obj: kbarray.inc kbd.inc kblow.inc

mouse.obj objo\mouse.obj objd\mouse.obj: dos.h i86.h mouse.h mousevel.h stdlib.h

ngpjoy.obj objo\ngpjoy.obj objd\ngpjoy.obj: dos.h i86.h ngpjoy.h

testcook.obj objo\testcook.obj objd\testcook.obj: ctype.h kb.h kbcook.h
