#
# $Source: x:/prj/tech/libsrc/g2/RCS/makefile $
# $Revision: 1.2 $
# $Author: KEVIN $
# $Date: 1996/04/26 10:44:56 $
#

MAKETOP = $(CWD)

PROJECT = tech

%include $(MRC)\env.rc

%include $(MRC)\init.rc

LIBTARGS = g2

# set_canvas callback is in init.
irrupt_OBJS = \
        init

G2_OBJS = \
        blend \
        init svgainit cominit \
        setdrv g2d calcd \
        chkply chkedges chkedgeb \
        clplin clip2d \
        fl8cnv \
        fl8sf fl8usf fl8s \
        fl8fl8s fl8il \
        fl8lin fl8ulf fl8l \
        fl8lm fl8lmf fl8llmf \
        fl8fl8l fl8fl8ll \
        gencnv \
        gensf genusf gens \
        genline \
        genfl8s genrsds \
        genfl8l genf16l \
        genil gensil genlil \
        genply gensply \
        genlm genlmf genllmf \
        g2poly g2tm \
        g2circ g2el g2ov g2ring \
        grwater \
        lmap \
        raster rastel scale \
        sply tmapd trifan \
        fl8o2ni fl8nni fl8o2wi fl8lo2wi

G2_HDRS= g2.h icanvas.inc

g2.h .REREAD :\
        stupid.h \
	icanvas.h plytyp.h \
        sftype.h lftype.h tftype.h pftype.h \
        g2d.h \
	grlin.h grtm.h grcurv.h \
	grply.h grscale.h init.h \
        buffer.h blnfcn.h \
        water.h \
        grchkply.h setdrv.h frame.h
	$(eko1) $(@,F) \<-- $(?,F)
	%do comp_h h_pre="\#include \<dev2d.h\>" h_in="$^" h_out="$(@,F)"

%include $(MRC)\make.rc

# this rule generates foo.c files from foo.tbl
%.c .MISER .NODEFAULT: %.tbl
        @%setenv INCLUDE=$(IPATH)        
	$(eko1) $(@,F) \<-- nonexistent
        @tblmunge $(<,F) $(MAKETOP)\$(@,F)
