DEBUG_ON = 1
SPEW_ON = 1
WARN_ON = 1
DBG_ON = 1


COMPILER = watcom
SRCDIR = n:\project\lib\src\dstruct
RELDIR = n:\project\lib\release

OBJDIR = obj
BINDIR =
LIBDIR = $(RELDIR)
INCDIR = $(RELDIR)
INCPATH = . $(SRCDIR) $(RELDIR)
LIBPATH = . $(RELDIR)
PROG_TARGETS = hashtest arraytst fnamtest symtest
LIB_TARGETS = dstruct
HDR_TARGETS =  
INST_TARGETS = dstruct

DSTRUCT_OBJS = array.obj fname.obj hash.obj hheap.obj lllist.obj llist.obj \
	rect.obj strall.obj symtab.obj
DSTRUCT_HDRS = array.h fname.h hash.h hheap.h llist.h rect.h slist.h \
	strall.h symtab.h

ARRAYTST_OBJS = arraytst.obj array.obj
ARRAYTST_LIBS = _lg.lib _mprintf.lib

FNAMTEST_OBJS = fnamtest.obj
FNAMTEST_LIBS = lg.lib mprintf.lib dstruct.lib

HASHTEST_OBJS = hashtest.obj  hashlex.obj hash.obj
HASHTEST_LIBS = lg.lib mprintf.lib

SYMTEST_OBJS  = symtest.obj
SYMTEST_LIBS  = lg.lib mprintf.lib fix.lib dstruct.lib

hashtest.c: hashtest.y 
	 yacc -D hashtok.h -o hashtest.c hashtest.y

hashlex.c: hashlex.lex hashtok.h 
         lex -o hashlex.c hashlex.lex

!include $(INIT)/make.rc

### OPUS MKMF:  Do not remove this line!  Automatic dependencies follow.

array.obj: array.h dbg.h error.h lg.h lgalt.h libdbg.h malloc.h memall.h \
	 stdio.h string.h types.h

arraytst.obj: array.h dbg.h error.h lg.h lgalt.h libdbg.h malloc.h memall.h \
	 stdio.h stdlib.h types.h

fname.obj: fname.h string.h types.h

fnamtest.obj: conio.h ctype.h fname.h stdio.h stdlib.h types.h

hash.obj: dbg.h error.h hash.h lg.h lgalt.h libdbg.h malloc.h memall.h \
	 stdio.h stdlib.h string.h types.h

hashtest.obj: dbg.h error.h hash.h hashtest.h lg.h lgalt.h libdbg.h \
	 malloc.h memall.h mprintf.h stdio.h stdlib.h string.h types.h

hheap.obj: dbg.h hheap.h lg.h lgalt.h libdbg.h llist.h malloc.h memall.h \
	 stdio.h stdlib.h string.h types.h

llist.obj: llist.h malloc.h memall.h string.h types.h

lllist.obj: llist.h types.h

rect.obj: rect.h

strall.obj: dbg.h hheap.h lg.h lgalt.h libdbg.h llist.h malloc.h memall.h \
	 stdio.h strall.h string.h types.h

symtab.obj: ctype.h dbg.h fix.h hheap.h lg.h lgalt.h libdbg.h llist.h \
	 malloc.h memall.h slist.h stdio.h strall.h string.h symtab.h \
	 types.h

symtest.obj: conio.h ctype.h dbg.h fix.h hheap.h lg.h lgalt.h libdbg.h \
	 llist.h malloc.h memall.h slist.h stdio.h stdlib.h strall.h \
	 symtab.h types.h
