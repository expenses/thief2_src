DEBUG_ON = 1
SPEW_ON = 1
WARN_ON = 1
DBG_ON = 1


COMPILER = watcom
SRCDIR = r:\prj\lib\src\dstruct
RELDIR = r:\prj\lib\rel

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

DSTRUCT_OBJS = array.obj fname.obj hash.obj hheap.obj huffde.obj huffen.obj \
	lllist.obj llist.obj message.obj pqueue.obj rect.obj strall.obj \
	symtab.obj symdump.obj
DSTRUCT_HDRS = array.h fname.h hash.h hheap.h huff.h huffde.h llist.h \
	message.h pqueue.h rect.h slist.h strall.h symtab.h

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

array.obj objo\array.obj objd\array.obj: array.h dbg.h dbgmacro.h error.h lg.h lgalt.h malloc.h memall.h \
	 memmacro.h stdio.h string.h types.h

arraytst.obj objo\arraytst.obj objd\arraytst.obj: array.h dbg.h dbgmacro.h error.h lg.h lgalt.h malloc.h \
	 memall.h memmacro.h stdio.h stdlib.h types.h

fname.obj objo\fname.obj objd\fname.obj: fname.h string.h types.h

fnamtest.obj objo\fnamtest.obj objd\fnamtest.obj: conio.h ctype.h fname.h stdio.h stdlib.h types.h

hash.obj objo\hash.obj objd\hash.obj: _dstruct.h dbg.h dbgmacro.h error.h hash.h lg.h lgalt.h malloc.h \
	 memall.h memmacro.h stdio.h stdlib.h string.h types.h

hashlex.obj objo\hashlex.obj objd\hashlex.obj: dbg.h dbgmacro.h hashtest.h hashtok.h lg.h lgalt.h malloc.h \
	 memall.h memmacro.h stdio.h stdlib.h string.h types.h

hashtest.obj objo\hashtest.obj objd\hashtest.obj: _dstruct.h dbg.h dbgmacro.h error.h hash.h hashtest.h lg.h \
	 lgalt.h malloc.h memall.h memmacro.h mprintf.h stdio.h stdlib.h \
	 string.h types.h

hheap.obj objo\hheap.obj objd\hheap.obj: dbg.h dbgmacro.h hheap.h lg.h lgalt.h llist.h malloc.h memall.h \
	 memmacro.h stdio.h stdlib.h string.h types.h

huffde.obj objo\huffde.obj objd\huffde.obj: dbg.h dbgmacro.h huff.h stdio.h string.h types.h

huffen.obj objo\huffen.obj objd\huffen.obj: dbg.h dbgmacro.h huff.h stdio.h stdlib.h string.h types.h

llist.obj objo\llist.obj objd\llist.obj: llist.h malloc.h memall.h memmacro.h string.h types.h

lllist.obj objo\lllist.obj objd\lllist.obj: llist.h types.h

message.obj objo\message.obj objd\message.obj: _dstruct.h dbg.h dbgmacro.h lg.h lgalt.h lgsprntf.h malloc.h \
	 memall.h memmacro.h message.h stdarg.h stdio.h stdlib.h string.h \
	 types.h

msgtest.obj objo\msgtest.obj objd\msgtest.obj: _dstruct.h conio.h ctype.h dbg.h dbgmacro.h lg.h lgalt.h \
	 malloc.h memall.h memmacro.h message.h mprintf.h stdio.h stdlib.h \
	 string.h types.h

pqueue.obj objo\pqueue.obj objd\pqueue.obj: dbg.h dbgmacro.h error.h io.h lg.h lgalt.h malloc.h memall.h \
	 memmacro.h pqueue.h stat.h stdio.h string.h types.h

rect.obj objo\rect.obj objd\rect.obj: rect.h

strall.obj objo\strall.obj objd\strall.obj: dbg.h dbgmacro.h hheap.h lg.h lgalt.h llist.h malloc.h memall.h \
	 memmacro.h stdio.h strall.h string.h types.h

symdump.obj objo\symdump.obj objd\symdump.obj: ctype.h dbg.h dbgmacro.h fix.h hheap.h lg.h lgalt.h llist.h \
	 malloc.h memall.h memmacro.h slist.h stdio.h string.h symtab.h \
	 types.h

symtab.obj objo\symtab.obj objd\symtab.obj: ctype.h dbg.h dbgmacro.h fix.h hheap.h lg.h lgalt.h lgsprntf.h \
	 llist.h malloc.h memall.h memmacro.h slist.h stdarg.h stdio.h \
	 strall.h string.h symtab.h types.h

symtest.obj objo\symtest.obj objd\symtest.obj: conio.h ctype.h dbg.h dbgmacro.h fix.h hheap.h lg.h lgalt.h \
	 llist.h malloc.h memall.h memmacro.h slist.h stdio.h stdlib.h \
	 strall.h symtab.h types.h
