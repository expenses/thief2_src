#
# $Source: x:/prj/tech/libsrc/lgalloc/RCS/makefile $
# $Revision: 1.3 $
# $Author: TOML $
# $Date: 1997/07/10 17:12:30 $
#
#

MAKETOP = $(CWD)

PROJECT = tech

%include $(MRC)\env.rc

%include $(MRC)\init.rc

LIBTARGS = lgalloc

EXETARGS = memtest
#heapperf

EXETYPE = con

lgalloc_OBJS = allcguid memall memcore membase primallc pool poolimp \
               multpool dbgalloc
#heapimp

lgalloc_HDRS = allocapi.h allcguid.h allocovr.h memall.h virtmem.h \
               pool.h poolimp.h

memtest_objs = memtest
memtest_libs = lgalloc lg mprintf

#heapperf_objs = heapperf
#heapperf_libs = multheap lg mprintf

%include $(MRC)\make.rc
