#
# $Source: x:/prj/tech/libsrc/r3d/RCS/makefile $
# $Revision: 1.7 $
# $Author: dc $
# $Date: 1996/05/12 00:39:17 $
#
# Makefile for r3d library
#

MAKETOP = $(CWD)

PROJECT = tech

%include $(MRC)\env.rc

%include $(MRC)\init.rc

LIBTARGS = r3d 

EXETARGS = r3dtest1

r3dtest1_LIBS = r3d 2d lg matrix mprintf
r3dtest1_OBJS = testcore test1

r3d_OBJS = \
  block    \
  clipclip \
  clipglob \
  cliplin  \
  clipplan \
  clipproj \
  clipunsc \
  ctxtguts \
  ctxtmang \
  ctxtmult \
  frame    \
  init     \
  object   \
  prim     \
  primconv \
  primface \
  primfunc \
  primmisc \
  primpoly \
  space    \
  view     \
  xfrmclip \
  xfrmglob \
  xfrmlin  \
  xfrmproj \
  xfrmunsc

r3d_HDRS = r3d.h r3ds.h

%include $(MRC)\make.rc


