#
# $Source: x:/prj/tech/libsrc/lg/RCS/makefile $
# $Revision: 2.1 $
# $Author: ept $
# $Date: 1995/10/02 20:07:18 $
#

MAKETOP = $(CWD)
PROJECT = windex

%include $(MRC)\env.rc
%include $(MRC)\init.rc

LIBTARGS = lg

lg_OBJS = bugtrak datapath dbg dbgcfg dbgpp exit lgprntf lgsprntf \
	memall memgrow memstat oheapwal stack tmpalloc

lg_HDRS = bugtrak.h datapath.h dbg.h dbgmacro.inc dbgpp.h lg.h \
	lgalt.h lgprntf.h lgsprntf.h memall.h memmacro.inc memstat.h \
	tmpalloc.h lg.inc memall.inc dbg.inc

# can't do
# h2i_files = lg.inc memall.inc dbg.inc

%include $(MRC)/make.rc

