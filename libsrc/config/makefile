DEBUG_ON = 1
SPEW_ON = 1
WARN_ON = 1
DBG_ON = 1


COMPILER = watcom
SRCDIR = n:\project\lib\src\config
RELDIR = r:\prj\lib\rel

OBJDIR = obj
BINDIR =
LIBDIR = $(RELDIR)
INCDIR = $(RELDIR)
INCPATH = . $(SRCDIR) $(RELDIR)
LIBPATH = . $(RELDIR)
PROG_TARGETS = cnfgtest
LIB_TARGETS = config
HDR_TARGETS =  
INST_TARGETS = config

CNFGTEST_OBJS = cnfgtest.obj
CNFGTEST_LIBS = _config.lib _dstruct.lib _lg.lib _mprintf.lib

CONFIG_OBJS = config.obj
CONFIG_HDRS = config.h

first_target: config _config

!include $(INIT)/make.rc

### OPUS MKMF:  Do not remove this line!  Automatic dependencies follow.

