#
# $Source: x:/prj/tech/libsrc/lg/RCS/makefile $
# $Revision: 2.3 $
# $Author: TOML $
# $Date: 1996/04/30 18:23:10 $
#

MAKETOP = $(CWD)
PROJECT = tech

%include $(MRC)\env.rc
%include $(MRC)\init.rc

LIBTARGS = lg

lg_OBJS = bugtrak datapath dbg dbgcfg dbgpp exit lgprntf lgsprntf \
	memall memgrow memstat oheapwal stack tmpalloc lglog lgassert

lg_HDRS = bugtrak.h datapath.h dbg.h dbgmacro.inc dbgpp.h lg.h \
	lgalt.h lgprntf.h lgsprntf.h memall.h memmacro.inc memstat.h \
	tmpalloc.h lg.inc memall.inc dbg.inc lglog.h lglocmsg.h lgassert.h

# can't do
# h2i_files = lg.inc memall.inc dbg.inc

%include $(MRC)/make.rc

