# $RCSFile$
# $Author: kaboom $
# $Date: 1992/08/25 08:47:39 $
#
# Makefile for Watcom C/Micosoft ASM
#
# $Log: makefile $
# Revision 1.3  1992/08/25 08:47:39  kaboom
# Added INST_TARGETS macro, a list of which targets should be installed when
# a make install is done.  Executables are copied to $(BIN), libraries are
# copied to $(LIB) and library headers are copied to $(INC).
#
# Revision 1.2  1992/08/24  17:27:40  kaboom
# Added RCS keywords and header at top of file.
# 

SRCDIR = n:\project\lib\src\fix
VPATH =
VPATH.c = $(SRCDIR)
VPATH.h = $(SRCDIR)
VPATH.asm = $(SRCDIR)

OBJDIR = .\obj
INCDIR = .\

PROG_TARGETS =
LIB_TARGETS = fix
INST_TARGETS = fix

FIX_OBJS = fix.obj
FIX_HDRS = fix.h

WATCOM = p:\w90
BIN = n:\bin
LIB = n:\lib
INC = n:\inc

CC = wcc386p
CDEBUG = -d2
COPT = -ox
CFLAGS = -3r -w4 -zq -i=$(INCDIR) -i=$(SRCDIR)\inc -fo=$@ $(CCDEBUG) $(CCOPT)
LD = wlinkp
LDEBUG = debug all
LDFLAGS = system dos4g option caseexact option map option quiet $(LDDEBUG)
AS = ml
ASFLAGS = /nologo /c /Cp /Zd /Fo$@
AR = wlib
ARFLAGS = -q -n
CP = copy
RM = rm -f
MAKEDEPEND = mkmf

TARGEXE = $(PROG_TARGETS,>.exe)
TARGLNK = $(PROG_TARGETS,>.lnk)
TARGLIB = $(LIB_TARGETS,>.lib)
TARGAR  = $(LIB_TARGETS,>.ar)

# clear suffixes so make won't try to make .exe's from .obj's
.SUFFIXES:

.SUFFIXES: .obj .c .asm

# punt -c option for obj->c rule
.c.obj:
	$(CC) $(CFLAGS) $<

.asm.obj:
	$(AS) $(ASFLAGS) $<

#.obj.exe:

all .PHONY: $(PROG_TARGETS) $(LIB_TARGETS)

$(PROG_TARGETS) .PHONY: $*.exe

$(LIB_TARGETS) .PHONY: $*.lib

$(TARGLNK): makefile
        %do makelnk TARGET=$*

$(TARGEXE): $*.lnk $($*_OBJS,<$(OBJDIR)\) $($*_LIBS)
        %do runlnk TARGET=$*

$(TARGAR): makefile
        %do makear TARGET=$*

$(TARGLIB): $*.ar $($*_OBJS,<$(OBJDIR)\)
        %do runar TARGET=$*

debug .PHONY:
        @%set CCDEBUG=$(CDEBUG)
        @%set LDDEBUG=$(LDEBUG)

optimize .PHONY:
        @%set CCOPT=$(COPT)

depend .PHONY:
        $(MAKEDEPEND) *.c

install .PHONY: $(INST_TARGETS)
        !foreach TARGET $(INST_TARGETS)
          !if '$(PROG_TARGETS,M$(TARGET))'
            $(CP) $(TARGET).exe $(BIN)
          !else if '$(LIB_TARGETS,M$(TARGET))'
            $(CP) $(TARGET).lib $(LIB)
            !foreach FILE $($(TARGET)_HDRS)
              $(CP) $(SRCDIR)\$(INCDIR)\$(FILE) $(INC)
            !end
          !endif
        !end

uninstall .PHONY:
        $(RM) $(TARGEXE,<$(BIN)\)

clean .PHONY:
        $(RM) *.exe *.lib $(OBJDIR)\*.obj *.map *.err

makelnk .PHONY:
        @echo $(LDFLAGS) >$@
        !foreach FILE $($(TARGET)_OBJS,<$(OBJDIR)\)
          @echo file $(FILE) >>$@
        !end
        !foreach FILE $($(TARGET)_LIBS)
          @echo library $(FILE) >>$@
        !end
        @echo name $(TARGET).exe >>$@

runlnk .PHONY:
        $(LD) @$(TARGET).lnk

makear: .PHONY:
        @echo $(TARGET).lib $($(TARGET)_OBJS,<$(OBJDIR)\) >$@

runar: .PHONY:
        $(AR) $(ARFLAGS) @$(TARGET).ar
