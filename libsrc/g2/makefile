#
# $Source: x:/prj/tech/libsrc/g2/RCS/makefile $
# $Revision: 1.12 $
# $Author: KEVIN $
# $Date: 1997/01/31 13:39:30 $
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
        init svgainit \
        setdrv g2d calcd \
        chkply chkedges chkedgeb \
        clplin clip clip2d \
        fl8cnv \
        fl8sf fl8usf fl8s \
        fl8fl8s fl8il \
        fl8lin fl8ulf fl8l \
        fl8lm fl8lmf fl8llmf \
        fl8fl8l fl8fl8ll \
        f16cnv \
        f16sf f16usf f16s \
        f16llmf f16lmf f16lm f16fl8ll \
        gencnv \
        gensf genusf gens \
        genline \
        genfl8s genrsds genrsdtm \
        genf16s genfl8l genf16l \
        genil gensil genlil \
        genply gensply \
        genlm genlmf genllmf \
        g2poly g2tm g2rot \
        g2circ g2el g2ov g2ring \
        g2makewr \
        grwater \
        lmap permap \
        raster rastel scale \
        sply tmapd trifan \
        fl8o2ni fl8nni fl8o2wi fl8lo2wi \
        trigrad trirast plyshell \
        golftmap shadtmap drytmap lh816 area

G2_HDRS= g2.h icanvas.inc

g2.h .REREAD :\
        stupid.h \
	icanvas.h plytyp.h \
        g2spoint.h \
        sftype.h lftype.h tftype.h pftype.h \
        g2d.h g2clip.h \
	grlin.h grtm.h grcurv.h \
	grply.h grscale.h init.h \
        buffer.h blnfcn.h \
        water.h \
        grchkply.h setdrv.h frame.h
	$(eko1) $(@,F) \<-- $(?,F)
	%do comp_h h_pre="\#include \<dev2d.h\>" h_in="$^" h_out="$(@,F)"

%include $(MRC)\make.rc
