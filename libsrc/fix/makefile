#
# $Source: x:/prj/tech/libsrc/fix/RCS/makefile $
# $Revision: 1.9 $
# $Author: dfan $
# $Date: 1993/06/01 14:13:44 $
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
FIX_OBJS = fix.obj fix_asm.obj

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
