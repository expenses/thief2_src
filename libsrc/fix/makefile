

# $Source: x:/prj/tech/libsrc/fix/RCS/makefile $
# $Revision: 2.3 $
# $Author: KEVIN $
# $Date: 1996/10/03 09:42:36 $

MAKETOP = $(CWD)

PROJECT = tech

%include $(MRC)\env.rc

%include $(MRC)\init.rc

LIBTARGS = fix

# need to deal with making trigtab.c  with fmaketab !!

fix_OBJS = fix fix_pow fixsprnt fix_asm atofix fix24 f_exp trigtab \
        fixdiv

irrupt_OBJS = fix

fix_HDRS = fix.h altfix.h fix.inc

%include $(MRC)\make.rc
