#
# $Header: x:/prj/tech/libsrc/prof/RCS/makefile 1.2 1995/01/16 14:18:18 buzzard Exp $
# $Log: makefile $
# Revision 1.2  1995/01/16 14:18:18  buzzard
# build dir
#
# Revision 1.1  1995/01/16  14:12:08  buzzard
# Initial revision
# 
#

DEV_BASE = r:\prj\lib

COMPILER = watcom
SRCDIR = $(DEV_BASE)\src\prof
RELDIR = $(DEV_BASE)\rel
OBJDIR = objo
LIBDIR = $(RELDIR)
INCDIR = $(RELDIR)
INCPATH = . $(SRCDIR) $(RELDIR)
LIBPATH = $(RELDIR)
BUILD_DIR = $(DEV_BASE)\build\prof

# normal and stereo version
LIB_TARGETS = prof
INST_TARGETS = prof

# this is kind of bad, since you lose other dependencies
PROF_OBJS = prof.obj
PROF_HDRS = prof.h

!include $(INIT)/make.rc

### OPUS MKMF:  Do not remove this line!  Automatic dependencies follow.
