#
# $Source: x:/prj/tech/libsrc/lg/RCS/makefile $
# $Revision: 2.21 $
# $Author: TOML $
# $Date: 1997/09/06 15:00:55 $
#

MAKETOP = $(CWD)
PROJECT = tech

%include $(MRC)\env.rc
%include $(MRC)\init.rc

LIBTARGS = lg

lg_OBJS = bugtrak datapath dbg dbgcfg dbgpp exit lgprntf lgsprntf \
    memgrow memstat oheapwal stack tmpalloc lglog lgassert \
    stktrace  breakkey hashfns thrdtool appagg codewrit \
    dpathdir memreq crc32 findfile coremutx timings

lg_HDRS = bugtrak.h datapath.h dbg.h dbgmacro.inc dbgpp.h lg.h \
	lgalt.h lgprntf.h lgsprntf.h memmacro.inc memstat.h \
	tmpalloc.h lg.inc dbg.inc lglog.h lglocmsg.h lgassert.h \
	stktrace.h breakkey.h hashfns.h thrdtool.h \
	codewrit.h memreq.h crc32.h findfile.h coremutx.h timings.h

# can't do
# h2i_files = lg.inc memall.inc dbg.inc

%include $(MRC)/make.rc

