#
# $Source: x:/prj/tech/libsrc/recorder/RCS/makefile $
# $Revision: 1.3 $
# $Author: TOML $
# $Date: 1996/08/27 13:09:48 $
#
# Makefile for recorder library
#

MAKETOP = $(CWD)

PROJECT = tech

%include $(MRC)\env.rc

%include $(MRC)\init.rc

LIBTARGS = recorder

EXETARGS = rectest
EXETYPE = con

rectest_OBJS =          rectest
rectest_LIBS =          mprintf lg recorder  \
                        cpptools comtools compapis appcore \
                        templgen mprintf
rectest_COMPILER_LIBS = uuid

dev_rectest = rectest
dbg_rectest = rectest
opt_rectest = rectest

recorder_OBJS = recorder recdata rectempl

recorder_HDRS =

%include $(MRC)\make.rc

