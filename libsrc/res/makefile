#
# $Source: x:/prj/tech/libsrc/res/RCS/makefile $
# $Revisiion: $
# $Author: TOML $
# $Date: 1997/01/14 16:36:38 $

MAKETOP = $(CWD)

PROJECT = tech

%include $(MRC)\env.rc

%include $(MRC)\init.rc

LIBTARGS = res

res_OBJS = lzw lzwfd2bf res restypes resfile resacc refacc \
           resload resmem rescum resbuild resmake resexblk \
           resmisc resarq resthred resagg resguid \
           pkzip
           #crc32 explode implode

nostackchk_OBJS = pkzip

%if '$(flav)'=='dbg'
res_opt_objs = lzw
%endif

res_HDRS = lzw.h res.h restypes.h resrect.h resguid.h resagg.h

%include  $(MRC)\make.rc

