#
# $Source: x:/prj/tech/libsrc/star/RCS/makefile $
# $Revision: 1.1 $
# $Author: jaemz $
# $Date: 1994/10/24 23:27:56 $
#
# Makefile for Star library
#
# $Log: makefile $
# Revision 1.1  1994/10/24 23:27:56  jaemz
# Initial revision
#
#

COMPILER = watcom
SRCDIR = r:\prj\lib\src\star
RELDIR = r:\prj\lib\rel
OBJDIR = objo
LIBDIR = $(RELDIR)
INCDIR = $(RELDIR)
INCPATH = . $(SRCDIR) $(RELDIR)
LIBPATH = $(RELDIR)

# normal and stereo version
LIB_TARGETS = star  ststar
INST_TARGETS = star ststar

# this is kind of bad, since you lose other dependencies
objo\ststar.obj : star.c
	%do "%.obj : %.c" CFLAGS+="-DSTEREO_ON"

objd\ststar.obj : star.c
	%do "%.obj : %.c" CFLAGS+="-DSTEREO_ON"

ststar.obj : star.c
	%do "%.obj : %.c" CFLAGS+="-DSTEREO_ON"
	
STAR_OBJS = star.obj star3d.obj
STAR_HDRS = star.h 

STSTAR_OBJS = ststar.obj star3d.obj
STSTAR_HDRS = star.h

!include $(INIT)/make.rc

### OPUS MKMF:  Do not remove this line!  Automatic dependencies follow.

