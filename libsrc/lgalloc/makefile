#
# $Source: x:/prj/tech/libsrc/lgalloc/RCS/makefile $
# $Revision: 1.1 $
# $Author: TOML $
# $Date: 1997/07/08 10:47:59 $
#
#

MAKETOP = $(CWD)

PROJECT = tech

%include $(MRC)\env.rc

%include $(MRC)\init.rc

LIBTARGS = multheap

EXETARGS = mhtest heapperf

EXETYPE = con

multheap_OBJS = memcore heap poolcore pool poolimp
#heapimp

multheap_HDRS =

mhtest_objs = mhtest
mhtest_libs = multheap

heapperf_objs = heapperf
heapperf_libs = multheap lg mprintf

%include $(MRC)\make.rc
