#
# $Source: x:/prj/tech/winsrc/input/RCS/makefile $
# $Revision: 2.2 $
# $Author: JACOBSON $
# $Date: 1996/01/19 10:10:16 $
#

MAKETOP = $(CWD)

PROJECT = windex

%include $(MRC)\env.rc

%include $(MRC)\init.rc

LIBTARGS = input
EXETARGS = test

test_OBJS = test
test_LIBS = input

#input_dos4g_OBJS = \
# 	basjoy biosjoy fanjoy joyflat joystick kbarray kbasctab kbcntry \
#       kbbflush kbcstat kbflush kbgen kbgflag kbglob kbgstat kbint \
#       kblnext kblow kbnext kbphnd kbqueue kbrhnd kbsflag kbshut leds \
#       kbssig kbsstat kbstart kbstate mousesys mousedev ngpjoy kbcook
#       hatcal

#input_win32_OBJS = \
# 	joystick kbstart kbshut kbasctab mousesys mousedev \
#        kbsys kbdev kbcook

input_OBJS = \
 	joystick kbstart kbshut kbasctab mousesys mousedev \
        kbsys kbdev kbcook kbwin

# kbcntry?

#       kbarray kbasctab kbcntry kbbflush kbcook \
#	kbcstat kbflush kbgen kbgflag kbglob kbgstat kbint kblnext kblow \
#	kbnext kbphnd kbqueue kbrhnd kbsflag kbshut kbssig kbsstat \
#	kbstart kbstate leds mouse mousedev

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

# kb.h: kbs.h kbdecl.h kbscan.h kbcntry.h kbarray.inc kbd.inc kberr.inc kbflag.inc \
#         kblow.inc kbstate.inc

kb.h: kbs.h kbdecl.h kbscan.h kbmod.h kbarray.h kbstate.h 
        rm -f junk
        @echo cat $^ \>junk
        !foreach FILE $(^,M\.h)
          @cstrip $(FILE) >>junk
        !end
        !foreach FILE $(^,M\.inc)
          @inc2h $(FILE) junk
        !end
        @echo \#ifndef __KB_H >$(@,F)
        @echo \#define __KB_H >>$(@,F)
        sed '/\/\/.*$$/d' junk >>$(@,F)
        @echo \#endif /* !__KB_H */>>$(@,F)
 
%include $(MRC)\make.rc




