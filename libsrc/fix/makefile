SRC_DIR = n:\project\lib\src\fix
VPATH =
VPATH.c = $(SRC_DIR)
VPATH.h = $(SRC_DIR)
VPATH.asm = $(SRC_DIR)

OBJDIR = .\obj
INCDIR = .\

PROG_TARGETS =
LIB_TARGETS = fix

FIX_OBJS = \
        fix.obj

WATCOM = p:\w90
BIN = n:\bin

CC = wcc386p
CDEBUG = -d2
COPT = -ox
CFLAGS = -3r -w4 -zq -i=$(INCDIR) -i=$(SRC_DIR)\inc -fo=$@ $(CCDEBUG) $(CCOPT)
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

install .PHONY:
        !foreach FILE $(TARGEXE)
          $(CP) $(FILE) $(BIN)
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
