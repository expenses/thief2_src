#
# $Source: x:/prj/tech/libsrc/lg/RCS/makefile $
# $Revision: 2.17 $
# $Author: TOML $
# $Date: 1997/02/15 17:44:14 $
#

MAKETOP = $(CWD)
PROJECT = tech

%include $(MRC)\env.rc
%include $(MRC)\init.rc

LIBTARGS = lg

lg_OBJS = bugtrak datapath dbg dbgcfg dbgpp exit lgprntf lgsprntf \
    memall memgrow memstat oheapwal stack tmpalloc lglog lgassert \
    stktrace pool poolimp breakkey hashfns thrdtool appagg codewrit \
    dpathdir memreq crc32 findfile

lg_HDRS = bugtrak.h datapath.h dbg.h dbgmacro.inc dbgpp.h lg.h \
	lgalt.h lgprntf.h lgsprntf.h memall.h memmacro.inc memstat.h \
	tmpalloc.h lg.inc memall.inc dbg.inc lglog.h lglocmsg.h lgassert.h \
	stktrace.h pool.h poolimp.h breakkey.h hashfns.h thrdtool.h \
	codewrit.h memreq.h crc32.h findfile.h virtmem.h

# can't do
# h2i_files = lg.inc memall.inc dbg.inc

%include $(MRC)/make.rc

