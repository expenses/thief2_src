#
# $Source: x:/prj/tech/libsrc/matrix/RCS/makefile $
# $Revision: 1.13 $
# $Author: DAVET $
# $Date: 1997/08/13 11:07:09 $
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

.PATH.sed = $(.PATH.c)

mxd.sed .ALWAYS: 
        @%set fullpath_for_sed_script = $(.TARGET)

%if ($(LGFIRSTTARGET,lc) != 'install') 
.BEFORE: mxd.sed matrixd.h matrixds.h mxsymd.h
%endif

build_double_version: 
        @echo Building double version of $(source)
        -@chmod +w $(target)
        @sed -f $(fullpath_for_sed_script) $(source) > $(target,F)
        @chmod -w $(target)

vecvecd.c .MISER .NODEFAULT: vecvec.c
        %do build_double_version source=$(.SOURCE) target=$(.TARGET)

matmatd.c .MISER .NODEFAULT: matmat.c
        %do build_double_version source=$(.SOURCE) target=$(.TARGET)

matvecd.c .MISER .NODEFAULT: matvec.c
        %do build_double_version source=$(.SOURCE) target=$(.TARGET)

vecangd.c .MISER .NODEFAULT: vecang.c
        %do build_double_version source=$(.SOURCE) target=$(.TARGET)

matangd.c .MISER .NODEFAULT: matang.c
        %do build_double_version source=$(.SOURCE) target=$(.TARGET)

transd.c .MISER .NODEFAULT: trans.c
        %do build_double_version source=$(.SOURCE) target=$(.TARGET)

matrixd.h .MISER .NODEFAULT: matrix.h
        %do build_double_version source=$(.SOURCE) target=$(.TARGET)

matrixds.h .MISER .NODEFAULT: matrixs.h
        %do build_double_version source=$(.SOURCE) target=$(.TARGET)

mxsymd.h .MISER .NODEFAULT: mxsym.h
        %do build_double_version source=$(.SOURCE) target=$(.TARGET)

%include $(MRC)\make.rc

