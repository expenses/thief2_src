#
# $Source: x:/prj/tech/libsrc/dev2d/RCS/makefile $
# $Revision: 1.10 $
# $Author: KEVIN $
# $Date: 1996/08/07 13:17:06 $
#

MAKETOP = $(CWD)

PROJECT = tech

%include $(MRC)\env.rc

%include $(MRC)\init.rc

LIBTARGS = dev2d

EXETARGS = gdtest

irrupt_OBJS = \
        f16bm fl8bm bk8bm mxbm \
        genf16 genfl8 genmx genrsd8 \
        fl8fl8 fl8bk8 fl8mx \
        bk8fl8 mxfl8 f16f16 f16fl8 \
        gdbm gdgbm canvas bitmap grnull \
        astring hflip

dev2d_OBJS = \
        astring bitmap blend gr \
        bank canvas mxlatch screen grd \
        grnull grmalloc valloc \
        init svgainit cominit \
        mode \
        bk8cnv bk8p8f \
        bk8vlinf bk8hlinf bk8ubmf \
        bk8pix bk8lin bk8bm  \
        bk8pix8 bk8hlin bk8vlin \
        bk8fl8 \
        fl8cnv fl8p8f fl8p16f \
        fl8vlinf fl8hlinf fl8ubmf \
        fl8pix fl8lin fl8bm \
        fl8pix8 fl8hlin fl8vlin \
        fl8fl8 fl8mono fl8bk8 fl8mx fl8rsd8 \
        f16cnv f16p8f f16p16f \
        f16hlinf f16ubmf \
        f16pix f16lin f16bm \
        f16pix8 f16pix16 f16hlin \
        f16fl8 f16mono f16f16 f16rsd8 \
        f16il \
        gdbm gdgbm gdrect gdlin gdpix \
        genbm \
        genlin genfl8u genf16u genmonou \
        genbmf genfl8 genmono genmx \
        genrsd8 genf16 \
        gpix \
        hflip lpix \
        mxcnv mxp8f \
        mxvlinf mxhlinf mxubmf \
        mxpix mxlin mxbm \
        mxpix8 mxhlin mxvlin \
        mxfl8 \
        nulcnv \
        pal pal16 ipal gpal \
        rsdcvt rsdunpck rsdblt8 rsdblt16 \
        smode stastk \
        tluctab \
        comdtab vgadtab fl8dtab w32dtab vsadtab \
        comdfcn \
        vesac vesaasm vsastate \
        vgaasm vgasmod vgastate \
        clpfl8

gdtest_OBJS = gdtest

gdtest_LIBS = lg dev2d mprintf

DEV2D_HDRS = dev2d.h bitmap.inc cseg.inc dseg.inc

dev2d.h .REREAD :\
        grs.h bitmap.h canvas.h screen.h mode.h \
        fill.h clip.h icanvas.h idevice.h grd.h context.h \
        bmftype.h linftype.h pixftype.h \
        gdbm.h gdlin.h gdrect.h gdpix.h grdev.h \
        pal.h pal16.h init.h \
        tluctab.h grmalloc.h grnull.h indexmac.h gr.h \
        rgb.h \
        stupid.h
	$(eko1) $(@,F) \<-- $(?,F)
	%do comp_h h_pre="\#include \<fix.h\>" h_in="$^" h_out="$(@,F)"

%include $(MRC)\make.rc

# this rule generates foo.c files from foo.tbl

.PATH.tbl = $(.PATH.c)

%.c .MISER .NODEFAULT: %.tbl
        @%setenv INCLUDE=$(IPATH)        
	$(eko1) $(@,F) \<-- nonexistent
        @tblmunge $(<,F) $(MAKETOP)\$(@,F)

