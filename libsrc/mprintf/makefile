#
# $Source: x:/prj/tech/libsrc/mprintf/RCS/makefile $
# $Revision: 1.1 $
# $Author: kaboom $
# $Date: 1992/11/25 14:57:22 $
#
# Makefile for Watcom C/Micosoft ASM targets.
#
# $Log: makefile $
# Revision 1.1  1992/11/25 14:57:22  kaboom
# Initial revision
#

COMPILER = watcom
SRCDIR = n:\project\lib\src\mprintf
RELDIR = n:\project\lib\release
OBJDIR = obj
BINDIR =
LIBDIR = n:\project\lib\release
INCDIR = $(RELDIR)
INCPATH = . $(SRCDIR) $(RELDIR)
LIBPATH = $(RELDIR)
PROG_TARGETS =
LIB_TARGETS = mprintf
HDR_TARGETS =
INST_TARGETS = mprintf

MPRINTF_OBJS = \
  mprintf.obj

!include n:/etc/make.rc

### OPUS MKMF:  Do not remove this line!  Automatic dependencies follow.

mprintf.obj: conio.h dos.h fcntl.h i86.h io.h stat.h stdarg.h stdio.h \
	 string.h types.h
