DEBUG_ON = 1
SPEW_ON = 1
WARN_ON = 1
DBG_ON = 1


COMPILER = watcom
SRCDIR = r:\prj\lib\src\dstruct
RELDIR = r:\prj\lib\release

OBJDIR = obj
BINDIR =
LIBDIR = $(RELDIR)
INCDIR = $(RELDIR)
INCPATH = . $(SRCDIR) $(RELDIR)
LIBPATH = . $(RELDIR)
PROG_TARGETS = arraytst fnamtest hashtest msgtest symtest #pqtest
LIB_TARGETS = dstruct
HDR_TARGETS =  
INST_TARGETS = dstruct

DSTRUCT_OBJS = array.obj fname.obj hash.obj hheap.obj lllist.obj llist.obj \
	message.obj pqueue.obj rect.obj strall.obj symtab.obj symdump.obj
DSTRUCT_HDRS = array.h fname.h hash.h hheap.h llist.h message.h pqueue.h \
	rect.h slist.h strall.h symtab.h

ARRAYTST_OBJS = arraytst.obj
ARRAYTST_LIBS = _lg.lib _mprintf.lib dstruct.lib

FNAMTEST_OBJS = fnamtest.obj
FNAMTEST_LIBS = _lg.lib mprintf.lib dstruct.lib

HASHTEST_OBJS = hashtest.obj hashlex.obj
HASHTEST_LIBS = _lg.lib mprintf.lib dstruct.lib

MSGTEST_OBJS  = msgtest.obj
MSGTEST_LIBS  = _lg.lib mprintf.lib fix.lib dstruct.lib

PQTEST_OBJS   = pqtest.obj
PQTEST_LIBS   = _lg.lib dstruct.lib mprintf.lib

SYMTEST_OBJS  = symtest.obj
SYMTEST_LIBS  = _lg.lib mprintf.lib fix.lib dstruct.lib

!include $(INIT)/make.rc

hashtest.c: hashtest.y 
	 yacc -D hashtok.h -o hashtest.c hashtest.y

hashlex.c: hashlex.lex hashtok.h 
         lex -o hashlex.c hashlex.lex

### OPUS MKMF:  Do not remove this line!  Automatic dependencies follow.

array.obj: array.h dbg.h dbgmacro.h error.h lg.h lgalt.h malloc.h memall.h \
	 memmacro.h stdio.h string.h types.h

arraytst.obj: array.h dbg.h dbgmacro.h error.h lg.h lgalt.h malloc.h \
	 memall.h memmacro.h stdio.h stdlib.h types.h

fname.obj: fname.h string.h types.h

fnamtest.obj: conio.h ctype.h fname.h stdio.h stdlib.h types.h

hash.obj: _dstruct.h dbg.h dbgmacro.h error.h hash.h lg.h lgalt.h malloc.h \
	 memall.h memmacro.h stdio.h stdlib.h string.h types.h

hashlex.obj: dbg.h dbgmacro.h hashtest.h hashtok.h lg.h lgalt.h malloc.h \
	 memall.h memmacro.h stdio.h stdlib.h string.h types.h

hashtest.obj: _dstruct.h dbg.h dbgmacro.h error.h hash.h hashtest.h lg.h \
	 lgalt.h malloc.h memall.h memmacro.h mprintf.h stdio.h stdlib.h \
	 string.h types.h

hheap.obj: dbg.h dbgmacro.h hheap.h lg.h lgalt.h llist.h malloc.h memall.h \
	 memmacro.h stdio.h stdlib.h string.h types.h

llist.obj: llist.h malloc.h memall.h memmacro.h string.h types.h

lllist.obj: llist.h types.h

message.obj: _dstruct.h dbg.h dbgmacro.h lg.h lgalt.h malloc.h memall.h \
	 memmacro.h message.h stdio.h stdlib.h string.h types.h

msgtest.obj: _dstruct.h conio.h ctype.h dbg.h dbgmacro.h lg.h lgalt.h \
	 malloc.h memall.h memmacro.h message.h mprintf.h stdio.h stdlib.h \
	 string.h types.h

pqueue.obj: dbg.h dbgmacro.h error.h io.h lg.h lgalt.h malloc.h memall.h \
	 memmacro.h pqueue.h stat.h stdio.h string.h types.h

rect.obj: rect.h

strall.obj: dbg.h dbgmacro.h hheap.h lg.h lgalt.h llist.h malloc.h memall.h \
	 memmacro.h stdio.h strall.h string.h types.h

symtab.obj: ctype.h dbg.h dbgmacro.h fix.h hheap.h lg.h lgalt.h llist.h \
	 malloc.h memall.h memmacro.h slist.h stdio.h strall.h string.h \
	 symtab.h types.h

symtest.obj: conio.h ctype.h dbg.h dbgmacro.h fix.h hheap.h lg.h lgalt.h \
	 llist.h malloc.h memall.h memmacro.h slist.h stdio.h stdlib.h \
	 strall.h symtab.h types.h

