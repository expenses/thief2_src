#
# $Source: x:/prj/tech/libsrc/matrix/RCS/makefile $
# $Revision: 1.8 $
# $Author: dc $
# $Date: 1997/08/12 06:59:53 $
#
# Makefile for Matrix library
#

MAKETOP = $(CWD)

PROJECT = tech

%include $(MRC)\env.rc

%include $(MRC)\init.rc

LIBTARGS = matrix

# line 2 of each of these is the generated double precision stuff

matrix_OBJS = \
        vecvec matmat matvec vecang matang trans transasm dmat \
        vecvecd matmatd matvecd vecangd matangd transd 

matrix_HDRS = \
        matrix.h matrixs.h dmat.h dmats.h mxsym.h mxang.h mxcvrt.h \
        matrixd.h matrixds.h mxsymd.h

# zany override fun

# WARNING: do not hand edit *d.c or *d.h
#  they are built from the sed script, which has zany rules
#  see matdbl.txt in libsrc/matrix

build_double_version:
        @echo Build double for $(target)
        @sed -f mxd.sed $(source) > $(target)

#        @chmod -w $(target)

vecvecd.c: vecvec.c
        %do build_double_version source=$(.SOURCE) target=$(.TARGET)

matmatd.c: matmat.c
        %do build_double_version source=$(.SOURCE) target=$(.TARGET)

matvecd.c: matvec.c
        %do build_double_version source=$(.SOURCE) target=$(.TARGET)

vecangd.c: vecang.c
        %do build_double_version source=$(.SOURCE) target=$(.TARGET)

matangd.c: matang.c
        %do build_double_version source=$(.SOURCE) target=$(.TARGET)

transd.c: trans.c
        %do build_double_version source=$(.SOURCE) target=$(.TARGET)

matrixd.h: matrix.h
        %do build_double_version source=$(.SOURCE) target=$(.TARGET)

matrixds.h: matrixs.h
        %do build_double_version source=$(.SOURCE) target=$(.TARGET)

mxsymd.h: mxsym.h
        %do build_double_version source=$(.SOURCE) target=$(.TARGET)

%include $(MRC)\make.rc

