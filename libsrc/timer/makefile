#
# $Source: x:/prj/tech/libsrc/timer/RCS/makefile $
# $Revision: 1.1 $
# $Author: kaboom $
# $Date: 1993/05/20 16:17:41 $
#

DEBUG_ON = 1

COMPILER = watcom
SRCDIR = n:\project\lib\src\timer
RELDIR = n:\project\lib\release
OBJDIR = obj
LIBDIR = $(RELDIR)
INCDIR = $(RELDIR)
INCPATH = . $(SRCDIR) $(INCDIR)
LIBPATH = $(RELDIR)
LIB_TARGETS = timer
PROG_TARGETS =
INST_TARGETS = timer

TIMER_OBJS = tmact.obj tmadd.obj tmclose.obj tmd.obj \
             tmdeact.obj tmfast.obj tminit.obj tmphnd.obj \
             tmrate.obj tmrem.obj tmsden.obj \
             tmsfreq.obj tmsper.obj tmtick.obj
TIMER_HDRS = timer.h

!include $(INIT)\make.rc

timer.h: tmdecl.h tmd.inc tmflag.inc tmlow.inc tmreg.inc
        rm -f junk
        @echo cat $^ \>junk
        !foreach FILE $(^,M\.h)
          @cat $(FILE) >>junk
        !end
        !foreach FILE $(^,M\.inc)
          @inc2h $(FILE) junk
        !end
        @echo \#ifndef __TIMER_H >$(@,F)
        @echo \#define __TIMER_H >>$(@,F)
        sed '/\/\/.*$$/d' junk >>$(@,F)
        @echo \#endif /* !__TIMER_H */>>$(@,F)

### OPUS MKMF:  Do not remove this line!  Automatic dependencies follow.

tmact.obj: tmd.inc tmflag.inc tmlow.inc tmrec.inc

tmadd.obj: tmd.inc tmdecl.inc tmflag.inc tmlow.inc tmrec.inc

tmclose.obj: dpmi.inc tmd.inc tmreg.inc type.inc

tmd.obj: tmd.inc tmlow.inc

tmdeact.obj: tmd.inc tmflag.inc tmlow.inc tmrec.inc

tmfast.obj: tmd.inc tmdecl.inc tmflag.inc tmlow.inc tmrec.inc

tminit.obj: dpmi.inc tmd.inc tmdecl.inc tmflag.inc tmhnd.inc tmlow.inc \
	 tmrec.inc tmreg.inc type.inc

tmphnd.obj: pic.inc tmd.inc tmflag.inc tmlow.inc tmrec.inc type.inc

tmrate.obj: tmd.inc tmlow.inc tmreg.inc

tmrem.obj: tmd.inc tmflag.inc tmlow.inc

tmsden.obj: tmd.inc tmdecl.inc tmflag.inc tmlow.inc tmrec.inc

tmsfreq.obj: tmd.inc tmdecl.inc tmflag.inc tmlow.inc tmrec.inc tmreg.inc

tmsper.obj: tmd.inc tmdecl.inc tmflag.inc tmlow.inc tmrec.inc tmreg.inc

tmtick.obj: tmlow.inc
