#
# $Source: x:/prj/tech/libsrc/r3d/RCS/makefile $
# $Revision: 1.19 $
# $Author: TOML $
# $Date: 1996/10/17 17:23:00 $
#
# Makefile for r3d library
#

MAKETOP = $(CWD)

PROJECT = tech

%include $(MRC)\env.rc

%include $(MRC)\init.rc

LIBTARGS = r3d

# These only work under dos, so it breaks under windows, they should
# be in a test directory instead
# EXETARGS = r3dtest1
# r3dtest1_LIBS = r3d 2d lg matrix mprintf
# r3dtest1_OBJS = testcore test1

r3d_OBJS = \
  block    \
  clip     \
  clipclip \
  clipglob \
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
  utils    \
  view     \
  xfrmclip \
  xfrmglob \
  xfrmlin  \
  xfrmproj \
  xfrmunsc

r3d_HDRS= r3d.h r3ds.h

r3d_newline_hack=" "
r3d_newline=$(r3d_newline_hack,W\n)
r3d_dependent_includes="\#include \<matrix.h\>"$(r3d_newline)"\#include \<r3ds.h\>"$(r3d_newline)"\#include \<g2.h\>"

r3d.h .REREAD :\
        xfrmtab.h xform.h \
        clip.h clipcon.h clipplan.h clipprim.h \
        primcon.h prim.h primtab.h primface.h primconv.h \
        ctxts.h object.h space.h view.h block.h r3dctxt.h init.h \
        utils.h
	$(eko1) $(@,F) \<-- $(?,F)
        %do comp_h h_pre=$(r3d_dependent_includes) h_in="$^" h_out="$(@,F)"

%include $(MRC)\make.rc


