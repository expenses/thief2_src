#
# $Source: x:/prj/tech/libsrc/r3d/RCS/makefile $
# $Revision: 1.9 $
# $Author: JAEMZ $
# $Date: 1996/05/13 18:06:26 $
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

r3d_HDRS= r3d.h r3ds.h

r3d.h .REREAD :\
        grspoint.h r3ds.h rtex.h  xfrmtab.h xform.h \
        clip.h clipcon.h cliptab.h primcon.h prim.h primtab.h \
        ctxts.h object.h space.h view.h block.h r3dctxt.h init.h 
	$(eko1) $(@,F) \<-- $(?,F)
        %do comp_h h_pre="\#include \<matrix.h\>  \#include \<2d.h\>" h_in="$^" h_out="$(@,F)"

%include $(MRC)\make.rc


