#
# $Source: x:/prj/tech/winsrc/input/RCS/makefile $
# $Revision: 2.1 $
# $Author: ept $
# $Date: 1995/10/02 20:05:11 $
#

MAKETOP = $(CWD)

PROJECT = windex

%include $(MRC)\env.rc

%include $(MRC)\init.rc

LIBTARGS = input

input_OBJS = \
	basjoy biosjoy fanjoy joyflat joystick kbarray kbasctab kbcntry kbbflush \
        kbcook kbcstat kbflush kbgen kbgflag kbglob kbgstat kbint kblnext kblow \
	kbnext kbphnd kbqueue kbrhnd kbsflag kbshut kbssig kbsstat \
	kbstart kbstate leds mouse mousedev ngpjoy hatcal

input_HDRS = joystick.h kb.h kbcook.h keydefs.h mouse.h mousevel.h

# fix this, want right h2i args

# h2i_files = kbflag kbarray

# OK, it's busted and I'm getting headers from R:
# kbflag.inc : kbflag.h
# 	h2inc /Ni $^ $@

# kbarray.inc : kbarray.h
#	h2inc /Ni $^ $@

# kb.h : kbs.h kbdecl.h kbscan.h kbcntry.h kbarray.inc kbd.inc kberr.h 
#	kbflag.inc kblow.inc kbstate.inc
#	%do comp_h h_pre="" h_in="$(^,M\.h)" inc_in="$(^,M\.inc)" h_out="$@"
 
%include $(MRC)\make.rc




