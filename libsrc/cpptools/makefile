#
# $Source: x:/prj/tech/libsrc/cpptools/RCS/makefile $
# $Revision: 1.14 $
# $Author: TOML $
# $Date: 1996/11/05 17:13:24 $
#
#

MAKETOP = $(CWD)

PROJECT = tech

%include $(MRC)\env.rc

%include $(MRC)\init.rc

LIBTARGS = cpptools

EXETARGS =

cpptools_OBJS = delmlist dynfunc filepath filespec fnamutil splitpat str \
                ptrset hashset dlist dynarray relocptr pdynarr

cpptools_HDRS = delmlist.h dynfunc.h filekind.h filepath.h filespec.h  \
                fnamutil.h splitpat.h str.h hashset.h hshsttem.h dlist.h \
                dlisttem.h templexp.h ptrset.h dynarray.h relocptr.h \
                hshstimp.h pdynarr.h circque.h


%include $(MRC)\make.rc
