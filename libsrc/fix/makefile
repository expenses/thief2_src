#
# $Source: x:/prj/tech/libsrc/fix/RCS/makefile $
# $Revision: 1.10 $
# $Author: rex $
# $Date: 1993/11/04 11:06:32 $
#

COMPILER = watcom95
SRCDIR = n:\project\lib\src\fix
RELDIR = n:\project\lib\release
OBJDIR = obj
LIBDIR = $(RELDIR)
INCDIR = $(RELDIR)
INCPATH = . $(SRCDIR) $(RELDIR)
LIBPATH = $(RELDIR)

DEBUG_ON = yeah

PROG_TARGETS = fixtest fix24tst fmaketab trigerr #sqrttest
LIB_TARGETS = fix
HDR_TARGETS = fix
INST_TARGETS = fix

FIX_HDRS = fix.h fix.inc
FIX_OBJS = fix.obj fixsprnt.obj fix_asm.obj

FIXTEST_OBJS = $(FIX_OBJS) fixtest.obj

FIX24TST_OBJS = $(FIX_OBJS) fix24tst.obj

FMAKETAB_OBJS = fmaketab.obj

# SQRTTEST_OBJS = $(FIX_OBJS) sqrttest.obj

TRIGERR_OBJS = $(FIX_OBJS) trigerr.obj

!include $(INIT)/make.rc

trigtab.h: fmaketab.exe
	fmaketab > trigtab.h

fix.obj: fix.h trigtab.h

fixtest.obj: fix.h

fix24tst.obj: fix.h
### OPUS MKMF:  Do not remove this line!  Generated dependencies follow.

fix.obj: fix.h stdio.h stdlib.h trigtab.h types.h

fix24tst.obj: ctype.h fix.h stdio.h stdlib.h string.h types.h

fix_11.obj: fix.h stdio.h stdlib.h types.h

fix_old.obj: fix.h stdio.h stdlib.h types.h

fixsprnt.obj: fix.h stdio.h stdlib.h types.h

fixtest.obj: ctype.h fix.h stdio.h stdlib.h string.h types.h

floattst.obj: fix.h stdio.h types.h

fmaketab.obj: generic.h iomanip.h iostream.h math.h stddef.h stdlib.h \
	 streambu.h string.h

sqrttest.obj: fix.h stdio.h stdlib.h types.h

trigerr.obj: ctype.h fix.h math.h stdio.h stdlib.h string.h types.h
