#
# $Source: x:/prj/tech/libsrc/fix/RCS/makefile $
# $Revision: 1.6 $
# $Author: kaboom $
# $Date: 1993/01/27 17:57:34 $
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

PROG_TARGETS = fixtest sqrttest
LIB_TARGETS = fix
HDR_TARGETS = fix
INST_TARGETS = fix

FIX_HDRS = fix.h
FIX_OBJS = fix.obj fix_asm.obj
FIX_CFLAGS = -d2

FIXTEST_OBJS = fix.obj fix_asm.obj fixtest.obj
FIXTEST_CFLAGS = -d2

SQRTTEST_OBJS = fix.obj fix_asm.obj sqrttest.obj
SQRTTEST_CFLAGS = -d2

!include $(INIT)/make.rc

fix.c:	fix.h
