# 	$Header: x:/prj/tech/libsrc/lgd3d/RCS/makefile 1.1 1997/06/02 16:40:42 KEVIN Exp $

MAKETOP = $(CWD)

PROJECT = tech

%include $(MRC)\env.rc

# only SYS=WIN32 makes sense
SYS = WIN32

%include $(MRC)\init.rc

SPEC_EXETARGS = d3dtest

LIBTARGS = lgd3d

lgd3d_OBJS = setup trifan blit texture tmgr

lgd3d_HDRS = lgd3d.h

d3dtest_OBJS = comshell d3dtest $(lgd3d_OBJS)

d3dtest_LIBS = dxguid grtb $(TECH_SYS_LIBS) $(TECH_CORE_LIBS) $(TECH_GRAPH_LIBS)

%include $(MRC)\make.rc
