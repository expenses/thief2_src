#
# $Source: x:/prj/tech/libsrc/recorder/RCS/makefile $
# $Revision: 1.4 $
# $Author: TOML $
# $Date: 1996/09/05 11:52:41 $
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

recorder_OBJS = recorder rectempl

recorder_HDRS =

%include $(MRC)\make.rc

