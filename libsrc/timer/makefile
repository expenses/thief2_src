#
# $Source: x:/prj/tech/libsrc/timer/RCS/makefile $
# $Revision: 1.2 $
# $Author: lmfeeney $
# $Date: 1994/10/17 17:47:52 $
#

DEBUG_ON = 1

COMPILER = watcom
SRCDIR = r:\prj\lib\src\timer
RELDIR = r:\prj\lib\rel
OBJDIR = obj
LIBDIR = $(RELDIR)
INCDIR = $(RELDIR)
INCPATH = . $(SRCDIR) $(INCDIR)
LIBPATH = $(RELDIR)
LIB_TARGETS = timer 
PROG_TARGETS =
INST_TARGETS = TIMER 

TMRT.C_CFLAGS = -zu -s 

timer_OBJS = tmact.obj tmadd.obj tmclose.obj tmd.obj \
             tmdeact.obj tmfast.obj tminit.obj tmphnd.obj \
             tmrate.obj tmrem.obj tmsden.obj tmrt.obj \
             tmsfreq.obj tmsper.obj tmftab.obj tmlibt.obj tmail.obj

TIMER_HDRS = timer.h tminit.h

!include $(INIT)\make.rc

timer.h:  tmdecl.h tmftab.h tmmac.h tmd.inc tmflag.inc tmlow.inc tmreg.inc
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

tmact.obj objo\tmact.obj objd\tmact.obj: tmd.inc tmflag.inc tmlow.inc tmrec.inc

tmadd.obj objo\tmadd.obj objd\tmadd.obj: tmd.inc tmdecl.inc tmflag.inc tmlow.inc tmrec.inc

tmail.obj objo\tmail.obj objd\tmail.obj: ail32.h tmftab.h

tmclose.obj objo\tmclose.obj objd\tmclose.obj: dpmi.inc tmd.inc tmhnd.inc tmreg.inc type.inc

tmd.obj objo\tmd.obj objd\tmd.obj: tmd.inc tmlow.inc

tmdeact.obj objo\tmdeact.obj objd\tmdeact.obj: tmd.inc tmflag.inc tmlow.inc tmrec.inc

tmfast.obj objo\tmfast.obj objd\tmfast.obj: tmd.inc tmdecl.inc tmflag.inc tmlow.inc tmrec.inc

tmftab.obj objo\tmftab.obj objd\tmftab.obj: tmdecl.h tmftab.h

tminit.obj objo\tminit.obj objd\tminit.obj: dpmi.inc tmd.inc tmdecl.inc tmflag.inc tmftab.inc tmhnd.inc \
	 tmlow.inc tmrec.inc tmreg.inc type.inc

tmphnd.obj objo\tmphnd.obj objd\tmphnd.obj: pic.inc tmd.inc tmflag.inc tmlow.inc tmrec.inc type.inc

tmrate.obj objo\tmrate.obj objd\tmrate.obj: tmd.inc tmlow.inc tmreg.inc

tmrem.obj objo\tmrem.obj objd\tmrem.obj: tmd.inc tmflag.inc tmlow.inc

tmsden.obj objo\tmsden.obj objd\tmsden.obj: tmd.inc tmdecl.inc tmflag.inc tmlow.inc tmrec.inc

tmsfreq.obj objo\tmsfreq.obj objd\tmsfreq.obj: tmd.inc tmdecl.inc tmflag.inc tmlow.inc tmrec.inc tmreg.inc

tmsper.obj objo\tmsper.obj objd\tmsper.obj: tmd.inc tmdecl.inc tmflag.inc tmlow.inc tmrec.inc tmreg.inc

tmtick.obj objo\tmtick.obj objd\tmtick.obj: tmlow.inc
