#
# $Source: x:/prj/tech/libsrc/dispdev/RCS/makefile $
# $Revision: 1.9 $
# $Author: TOML $
# $Date: 1997/09/10 14:00:45 $
#
# Makefile for windisp library
#

MAKETOP = $(CWD)

PROJECT = tech

%include $(MRC)\env.rc

%include $(MRC)\init.rc

LIBTARGS = dispdev

dispdev_objs = dispbase
dispdev_win32_OBJS = wdisp wdspprov wdspmode wdispdd wddmode wdispgdi \
                     wdcreate wdspdynf wdisptls wdspgshl
dispdev_dos4g_OBJS = ddisp

dispdev_HDRS = wdispapi.h wdspguid.h

%include $(MRC)\make.rc

