#
# $Source: x:/prj/tech/libsrc/fix/RCS/makefile $
# $Revision: 1.7 $
# $Author: dfan $
# $Date: 1993/02/15 11:41:14 $
#

COMPILER = watcom
SRCDIR = n:\project\lib\src\fix
RELDIR = n:\project\lib\release
OBJDIR = obj
LIBDIR = $(RELDIR)
INCDIR = $(RELDIR)
INCPATH = . $(SRCDIR) $(RELDIR)
LIBPATH = $(RELDIR)

DEBUG_ON = yeah

PROG_TARGETS = fixtest fix24tst #sqrttest
LIB_TARGETS = fix
HDR_TARGETS = fix
INST_TARGETS = fix

FIX_HDRS = fix.h
FIX_OBJS = fix.obj fix_asm.obj

FIXTEST_OBJS = $(FIX_OBJS) fixtest.obj

FIX24TST_OBJS = $(FIX_OBJS) fix24tst.obj

# SQRTTEST_OBJS = $(FIX_OBJS) sqrttest.obj

!include $(INIT)/make.rc

fix.obj: fix.h

fixtest.obj: fix.h

fix24tst.obj: fix.h
