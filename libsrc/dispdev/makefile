#
# $Source: x:/prj/tech/libsrc/dispdev/RCS/makefile $
# $Revision: 1.7 $
# $Author: TOML $
# $Date: 1996/10/21 17:12:10 $
#
# Makefile for windisp library
#

MAKETOP = $(CWD)

PROJECT = tech

%include $(MRC)\env.rc

%include $(MRC)\init.rc

LIBTARGS = dispdev

dispdev_win32_OBJS = wdisp wdspprov wdspmode wdispdd wdispgdi wdcreate wdspdynf
dispdev_dos4g_OBJS = ddisp

dispdev_HDRS = wdispapi.h wdspguid.h

%include $(MRC)\make.rc

