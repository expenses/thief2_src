#
# $Source: x:/prj/tech/libsrc/star/RCS/makefile $
# $Revision: 2.1 $
# $Author: ept $
# $Date: 1995/10/18 12:57:56 $
#
# Makefile for Star library
#
# $Log: makefile $
# Revision 2.1  1995/10/18 12:57:56  ept
# This is the makefile for the new make system.
#
# Revision 1.1.1.1  1995/03/20  16:14:41  jaemz
# New make system
# 
# Revision 1.1  1994/10/24  23:27:56  jaemz
# Initial revision
# 
#

MAKETOP = $(CWD)

PROJECT = tech 

%include $(MRC)\env.rc

%include $(MRC)\init.rc

# normal and stereo version
LIBTARGS = star ststar

# this is kind of bad, since you lose other dependencies
ststar_DEFINES = STEREO_ON

star_OBJS = star star3d
star_HDRS = star.h 

ststar_OBJS = ststar star3d
ststar_HDRS = star.h

ststar.c: star.c
        copy star.c ststar.c

!include $(INIT)/make.rc

