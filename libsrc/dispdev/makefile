#
# $Source: x:/prj/tech/libsrc/dispdev/RCS/makefile $
# $Revision: 1.10 $
# $Author: KEVIN $
# $Date: 1997/10/10 15:04:58 $
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
                     wdcreate wdisptls wdspgshl dddynf 
dispdev_dos4g_OBJS = ddisp

dispdev_HDRS = wdispapi.h wdspguid.h dddynf.h

%include $(MRC)\make.rc

