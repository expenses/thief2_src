#
# $Source: x:/prj/tech/libsrc/g2/RCS/makefile $
# $Revision: 1.22 $
# $Author: KEVIN $
# $Date: 1997/08/28 15:23:55 $
#

MAKETOP = $(CWD)

PROJECT = tech

%include $(MRC)\env.rc

%include $(MRC)\init.rc

LIBTARGS = g2

# set_canvas callback is in init.
irrupt_OBJS = \
        init

# all objs related to polygon rendering
POLY_OBJS = \
        grtm permap \
        tmsetup tf \
        genlm fl8lm \
        genfl8l genf16l genrsdtm \
        o88 lo88 co88 r88 t88oc \
        lht816 lh816 dlh816 t816oc \
        golftmap shadtmap drytmap shadhack area \
        trigrad trirast \
        genply gensply genrast \
        tmapd clip 

# all objs related to bitmap scaling
SCALE_OBJS = scshell fl8s gens \
        fl8fl8s genfl8s genrsds cc816

# all objs related to sloped line rendering
LINE_OBJS = clplin fl8lin fl8l genline 

# all inner loops (assembly and c)
INNER_LOOP_OBJS = \
        gen8il gen16il \
        o88il o88bil lo88il co88il n88il 

# function tables compiled from .tbl's
TABLE_OBJS = gencnv fl8cnv f16cnv \
        genlmf genllmf gensf genusf \
        fl8ulf fl8lmf fl8llmf fl8sf fl8usf

# miscellaneous primitives, initialization code, and global data
MISC_OBJS = blend init svgainit setdrv g2d g2makewr \
        g2rot g2circ g2el g2ov g2ring point rastel

# legacy code from terra nova; won't be linked into other apps
OLD_CRUFTY_TN_OBJS = chkply chkedges chkedgeb 

G2_OBJS = $(POLY_OBJS) $(SCALE_OBJS) $(LINE_OBJS) $(INNER_LOOP_OBJS) \
        $(TABLE_OBJS) $(MISC_OBJS) $(OLD_CRUFTY_TN_OBJS)

G2_HDRS= g2.h icanvas.inc

g2.h .REREAD :\
        stupid.h \
	icanvas.h plytyp.h \
        g2spoint.h \
        sftype.h lftype.h tftype.h pftype.h \
        g2d.h g2clip.h \
	grlin.h grtm.h grcurv.h \
	grply.h grscale.h g2tm.h init.h \
        buffer.h blnfcn.h \
        water.h \
        grchkply.h setdrv.h frame.h point.h
	$(eko1) $(@,F) \<-- $(?,F)
	%do comp_h h_pre="\#include \<dev2d.h\>" h_in="$^" h_out="$(@,F)"

%include $(MRC)\make.rc
