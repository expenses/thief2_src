#
# $Header: x:/prj/tech/libsrc/prof/RCS/makefile 1.7 1995/01/17 13:07:05 dfan Exp $
# $Log: makefile $
# Revision 1.7  1995/01/17 13:07:05  dfan
# Turn off stack checking the right way
# OBJDIR was set to objo, causing much unanticipated havoc
#
# Revision 1.6  1995/01/17  12:54:01  buzzard
# fix stack checking
# 
# Revision 1.5  1995/01/17  11:25:51  buzzard
# no stack checking
# 
# Revision 1.4  1995/01/17  07:54:12  buzzard
# dependencies
# 
# Revision 1.3  1995/01/16  14:20:15  buzzard
# duh
# 
# Revision 1.2  1995/01/16  14:18:18  buzzard
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
OBJDIR = obj
LIBDIR = $(RELDIR)
INCDIR = $(RELDIR)
INCPATH = . $(SRCDIR) $(RELDIR)
LIBPATH = $(RELDIR)
BUILD_DIR = $(DEV_BASE)\build\prof

# normal and stereo version
LIB_TARGETS = prof
INST_TARGETS = prof

PROF_OBJS = prof.obj
PROF_HDRS = prof.h
PROF_CFLAGS = -s

!include $(INIT)/make.rc

### OPUS MKMF:  Do not remove this line!  Automatic dependencies follow.

prof.obj objo\prof.obj objd\prof.obj: fix.h lgsprntf.h prof.h stdarg.h stdio.h stdlib.h string.h \
	 timer.h types.h
