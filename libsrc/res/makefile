#
# $Source: x:/prj/tech/libsrc/res/RCS/makefile $
# $Revisiion: $
# $Author: TOML $
# $Date: 1997/03/06 21:46:48 $

MAKETOP = $(CWD)

PROJECT = tech

%include $(MRC)\env.rc

%include $(MRC)\init.rc

LIBTARGS = res

res_OBJS = lzw lzwfd2bf res restypes resfile resacc refacc \
           resload resmem rescum resbuild resmake resexblk \
           resmisc resarq resthred resagg resguid rescache \
           pkzip


nostackchk_OBJS = pkzip

%if '$(flav)'=='dbg'
res_opt_objs = lzw
%endif

res_HDRS = lzw.h res.h restypes.h resrect.h resguid.h resagg.h resthred.h

%include  $(MRC)\make.rc

