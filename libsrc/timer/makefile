#
# $Source: x:/prj/tech/libsrc/timer/RCS/makefile $
# $Revision: 2.2 $
# $Author: JACOBSON $
# $Date: 1996/01/30 11:45:22 $
#

MAKETOP = $(CWD)

PROJECT = windex

%include $(MRC)\env.rc

%include $(MRC)\init.rc

LIBTARGS = timer

irrupt_OBJS = tmrt

timer_OBJS = tmact tmadd tmclose tmd \
             tmdeact tmfast tminit tmphnd \
             tmrate tmrem tmsden tmrt \
             tmsfreq tmsper tmftab tmlibt tmail \
             tmwin32

timer_HDRS = timer.h tminit.h

timer.h: tmdecl.h tmftab.h tmmac.h tmd.inc tmflag.inc tmlow.inc tmreg.inc
	%do comp_h h_pre="\#include \<fix.h\>" h_in="$(^,M\.h)" inc_in="$(^,M\.inc)" h_out="$@"

%include $(MRC)\make.rc
