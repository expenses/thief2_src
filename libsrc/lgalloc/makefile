#
# $Source: x:/prj/tech/libsrc/lgalloc/RCS/makefile $
# $Revision: 1.2 $
# $Author: TOML $
# $Date: 1997/07/10 11:26:41 $
#
#

MAKETOP = $(CWD)

PROJECT = tech

%include $(MRC)\env.rc

%include $(MRC)\init.rc

LIBTARGS = memcore

EXETARGS = memtest
#heapperf

EXETYPE = con

memcore_OBJS = allcguid memcore membase primallc stdalloc
#heapimp

memcore_HDRS = allocapi.h allcguid.h allocovr.h

memtest_objs = memtest
memtest_libs = memcore

#heapperf_objs = heapperf
#heapperf_libs = multheap lg mprintf

%include $(MRC)\make.rc
