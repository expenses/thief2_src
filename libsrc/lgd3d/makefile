# 	$Header: x:/prj/tech/libsrc/lgd3d/RCS/makefile 1.7 1997/10/06 14:57:30 KEVIN Exp $

MAKETOP = $(CWD)

PROJECT = tech

%include $(MRC)\env.rc

# only SYS=WIN32 makes sense
SYS = WIN32

%include $(MRC)\init.rc

SPEC_EXETARGS = d3dtest

LIBTARGS = lgd3d


lgd3d_OBJS = tmgr tmgrold

lgd3d_WIN32_OBJS = setup trifan blit texture 

lgd3d_HDRS = lgd3d.h tmgr.h


d3dtest_WIN32_OBJS = $(lgd3d_WIN32_OBJS)

d3dtest_OBJS = comshell d3dtest $(lgd3d_OBJS)

d3dtest_LIBS = dxguid grtb $(TECH_SYS_LIBS) $(TECH_CORE_LIBS) $(TECH_GRAPH_LIBS)

%include $(MRC)\make.rc
