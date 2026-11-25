/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/buftagf.cpp,v 2.2 2000/03/07 19:56:48 toml Exp $

#include <lg.h>
#include <vernum.h>
#include <tagfile_.h>

#include <config.h>
#include <cfgdbg.h>

// implement hash sets
#include <hshsttem.h>

// Must be last header
#include <dbmem.h>
#include <lg_file.h>

////////////////////////////////////////////////////////////
// TagFileVersion
//

static const TagVersion MyVersion =
{
  0,
  1,
};

static uchar Deadbeef[] = { 0xDE, 0xAD, 0xBE, 0xEF};

////////////////////////////////////////////////////////////
// TAG TABLE IMPLEMENTATION
//



tHashSetKey TagFileTable::GetKey(tHashSetNode node) const
{
   TagTableEntry* e = (TagTableEntry*)node;
   return (tHashSetKey)(&e->key);
}

////////////////////////////////////////

#define WRITE(file,x) Verify(lg_fwrite(&(x),sizeof(x),1,file) == 1)

void TagFileTable::Write(LGFILE* file)
{
   tHashSetHandle idx;
   const TagTableEntry* e;
   ulong items = GetCount();
   WRITE(file,items);
   for (e = GetFirst(idx); e != NULL; e = GetNext(idx))
   {
      WRITE(file,*e);
   }
}

////////////////////////////////////////

TagFileTable::~TagFileTable()
{
   tHashSetHandle handle;
   TagTableEntry* e;

   for (e = GetFirst(handle); e != NULL; e = GetNext(handle))
   {
      delete e;
   }
}

////////////////////////////////////////

#define READ(file,x) Verify(lg_fread(&(x),sizeof(x),1,file) == 1)

void TagFileTable::Read(LGFILE* file)
{
   ulong items;
   READ(file,items);
   for (int i = 0; i < items; i++)
   {
      TagTableEntry e;
      READ(file,e);
      Insert(new TagTableEntry(e));
   }
}

////////////////////////////////////////////////////////////
// TagFileBase implementation
//

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(TagFileBase,ITagFile);

////////////////////////////////////////

TagFileBase::~TagFileBase()
{
#line 104
   AssertMsg(block == NULL,"Tag file closing with a block still open");
   if (file != NULL)
      lg_fclose(file);
   file = NULL;
}


////////////////////////////////////////

STDMETHODIMP_(const TagVersion*) TagFileBase::GetVersion()
{
   return &MyVersion;
}

////////////////////////////////////////

STDMETHODIMP_(const TagFileTag*) TagFileBase::CurrentBlock()
{
   if (block == NULL)
      return NULL;
   return &block->key;
}

////////////////////////////////////////

STDMETHODIMP_(ulong) TagFileBase::BlockSize(const TagFileTag* tag)
{
   TagTableEntry* e = table.Search(tag);
   if (e == NULL)
      return kTagNoSuchBlock;
   return e->size;
}

////////////////////////////////////////

void TagFileBase::SetCurBlock(const TagFileTag* tag)
{
   if (tag == NULL)
      block = NULL;
   else
      block = table.Search(tag);
}

////////////////////////////////////////

int TagFileBase::PrepSeek(ulong& offset, TagFileSeekMode mode)
{
   if (block == NULL)
      return FALSE;

   // convert offset to absolute
   switch (mode)
   {
      case kTagSeekFromStart:
         offset += block->Start();
         break;
      case kTagSeekFromHere:
         offset += lg_ftell(file);
         break;
      case kTagSeekFromEnd:
         offset += block->End();
         break;
   }

   if (offset < block->Start())
      return block->offset - block->Start();
   if (offset > block->End())
      return offset - block->End();

   return 0;
}

////////////////////////////////////////

#define max(x,y) ((x) > (y) ? (x) : (y))
#define min(x,y) ((x) < (y) ? (x) : (y))


STDMETHODIMP TagFileBase::Seek(ulong offset, TagFileSeekMode mode)
{
   if (block == NULL)
   {
      Warning(("Seeking when no block is open\n"));
      return E_FAIL;
   }

   int check = PrepSeek(offset,mode);

   // if out of bounds, fail.
   if (check < 0 || (check > 0 && OpenMode() == kTagOpenRead))
   {
      Warning(("Seeking outside of tag file block\n"));
      return E_FAIL;
   }
   // @TBD: change logic so that SEEK_CUR is preserved for performance
   lg_fseek(file,offset,SEEK_SET);
   blockptr = offset - block->Start();

   if (OpenMode() == kTagOpenWrite)
   {
      block->size = max(block->size,blockptr);
   }

   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP_(ulong) TagFileBase::Tell()
{
   if (block == NULL)
      return 0;
   return blockptr;
}

//------------------------------------------------------------
// Iteration
//

class cTagIter : public ITagFileIter
{
   tHashSetHandle Idx;
   TagFileTable& Table;
   const TagTableEntry* Entry;

public:

   cTagIter(TagFileTable& t)
      :Entry(NULL),Table(t)
   {
   }

   virtual ~cTagIter() {};

   DECLARE_UNAGGREGATABLE();

   STDMETHOD(Start)() { Entry = Table.GetFirst(Idx); return S_OK; };
   STDMETHOD_(BOOL,Done)() { return Entry == NULL; };
   STDMETHOD(Next)() { Entry = Table.GetNext(Idx); return S_OK; };

   STDMETHOD_(const TagFileTag*,Tag)() { return (Entry) ? &Entry->key : NULL; } ;
};

////////////////////////////////////////

STDMETHODIMP_(ulong) TagFileBase::TellFromEnd()
{
   if (block == NULL)
      return 0;
   return block->size - blockptr;
}

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cTagIter,ITagFileIter);

STDMETHODIMP_(ITagFileIter*) TagFileBase::Iterate()
{
   return new cTagIter(table);
}

////////////////////////////////////////////////////////////
// TagFileWrite implementation
//

TagFileWrite::TagFileWrite(const char* filename)
   : TagFileBase(filename,"wb")
{
   if (file != NULL)
   {
      // write a place holder for header
      TagFileHeader header;
      WRITE(file,header);
   }
   else
   {
      Warning(("TagFileWrite: opening %s for writing failed\n",filename));
   }
}

////////////////////////////////////////

TagFileWrite::~TagFileWrite()
{
   if (file != NULL)
   {
      TagFileHeader header;

      lg_fseek(file,0,SEEK_END); // seek to end
      header.table = lg_ftell(file);

      table.Write(file); // write out the table

      lg_fseek(file,0,SEEK_SET); // seek to front

      // fill out header
      header.version = MyVersion;
      memcpy(header.deadbeef,Deadbeef,sizeof(header.deadbeef));

      // write out actual data in header
      WRITE(file,header);
   }
}


////////////////////////////////////////

STDMETHODIMP TagFileWrite::OpenBlock(const TagFileTag* tag, TagVersion* version)
{

   if (file == NULL)
      return E_FAIL;

   if (block != NULL)
   {
      Warning(("Opening Tag Block with a block already open\n"));
      return E_FAIL;
   }

#line 328
   AssertMsg1(table.Search(tag) == NULL,"TagFileWrite::NewBlock(): tag %s is already in use",tag->label);

   TagFileBlockHeader header;

   header.tag = *tag;
   header.version = *version;

   // put the block at the end of the file!
   lg_fseek(file,0,SEEK_END);

   // store offset and tag in table
   TagTableEntry* entry = new TagTableEntry(*tag,lg_ftell(file));
   void* result = table.Insert(entry);
   if (!result) return E_FAIL;

   SetCurBlock(tag);

   int len = lg_fwrite(&header,1,sizeof(header),file);
   if (len != sizeof(header))
   {
      Warning(("TagFileWrite::NewBlock(): wrote only %d out of %d bytes\n",len,sizeof(header)));
      return E_FAIL;
   }

   blockptr = 0;
   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP_(TagFileOpenMode) TagFileWrite::OpenMode()
{
   return kTagOpenWrite;
}

////////////////////////////////////////

STDMETHODIMP TagFileWrite::CloseBlock()
{
   if (block == NULL)
   {
      Warning(("TagFileWrite::CloseBlock() No Block to Close\n"));
      return E_FAIL;
   }
   SetCurBlock(NULL);
   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP_(long) TagFileWrite::Write(const char* buf, int buflen)
{
#line 388
   AssertMsg(block != NULL, "TagFileWrite::Write(): No block has been started\n");
   if (block == NULL) return -1;

   if (file == NULL)
      return -1;

   long len = lg_fwrite(buf,1,buflen,file);
   blockptr += len;

   block->size = max(blockptr,block->size);

   return len;
}

////////////////////////////////////////

STDMETHODIMP_(long) TagFileWrite::Read(char* , int )
{
   Warning(("Reading a TagFile opened for writing\n"));
   return -1;
}

////////////////////////////////////////

STDMETHODIMP TagFileWrite::Move(char* buf , int len)
{
   return Write(buf,len);
}

////////////////////////////////////////////////////////////
// TagFileRead Implementation
//

TagFileRead::TagFileRead(const char* filename)
   : TagFileBase(filename,"rb")
{
   if (file == NULL)
   {
      ConfigSpew("TagFileTrace",("TagFileRead: opening %s for reading failed\n",filename));
      return;
   }
   // read in header
   TagFileHeader header;
   READ(file,header);

   // verify header.

   if (memcmp(header.deadbeef,Deadbeef,sizeof(header.deadbeef)) != 0)
   {
      Warning(("Tag file %s is corrupt!\n",filename));
      lg_fclose(file);
      file = NULL;
      return;
   }

   // warn about version
   int delta = VersionNumsCompare(&MyVersion,&header.version);
   if (delta)
   {
#ifdef DBG_ON
      char buf[256];
      strcpy(buf,VersionNum2String(&MyVersion));
      Warning(("Tag file %s is not current version.\nOld: %s, New: %s\n",
               filename,VersionNum2String(&header.version),buf));
#endif
      // if it's NEWER, then discard
      if (delta < 0)
      {
         Warning(("Version in file is new.  Discarding tag\n"));
         lg_fclose(file);
         file = NULL;
         return;
      }
   }


   // read in tag table
   lg_fseek(file,header.table,SEEK_SET);
   table.Read(file);

}

////////////////////////////////////////

TagFileRead::~TagFileRead()
{

}

////////////////////////////////////////

STDMETHODIMP TagFileRead::OpenBlock(const TagFileTag* tag, TagVersion* version)
{
   if (file == NULL)
      return E_FAIL;

   SetCurBlock(tag);

   if (block == NULL)
   {
      ConfigSpew("tagfile_spew", ("Tag %s not in tag file\n", tag->label));
      return E_FAIL;
   }
   lg_fseek(file,block->offset,SEEK_SET);

   TagFileBlockHeader header;
   int len = lg_fread(&header,1,sizeof(header),file);
   if (len != sizeof(header))
   {
      Warning(("TagFileRead::OpenBlock(): read only %d of %d bytes\n",len,sizeof(header)));
      SetCurBlock(NULL);
      return E_FAIL;
   }

   // compare versons
   int delta = VersionNumsCompare(version,&header.version);
   if (delta != 0)
   {
#ifdef DBG_ON
      char buf[256];
      strcpy(buf,VersionNum2String(version));
      ConfigSpew("tagfile_spew",("Tag data %s is not current version.\nOld: %s, New: %s\n",
               tag->label,VersionNum2String(&header.version),buf));
#endif

      // if it's NEWER, then discard
      if (delta < 0)
      {
         ConfigSpew("tagfile_spew",("Version in file is newer.  Discarding tag\n"));
         SetCurBlock(NULL);
         return E_FAIL;
      }
   }

   *version = header.version;
   blockptr = 0;

   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP_(TagFileOpenMode) TagFileRead::OpenMode()
{
   return kTagOpenRead;
}

////////////////////////////////////////

STDMETHODIMP_(long) TagFileRead::Read(char* buf, int buflen)
{
   if (file == NULL)
      return -1;

   if (block == NULL)
   {
      Warning(("BufTagFileRead::Read() called before ::Seek()\n"));
      return -1;
   }

   ulong bytesleft = block->size - blockptr;
   if (buflen > bytesleft)
      buflen = max(bytesleft,0);

   int len = lg_fread(buf,1,buflen,file);
   blockptr += len;
   return len;
}

////////////////////////////////////////

STDMETHODIMP TagFileRead::CloseBlock()
{
   if (block == NULL)
   {
      Warning(("TagFileRead::CloseBlock() No Block to Close\n"));
      return E_FAIL;
   }
   SetCurBlock(NULL);
   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP TagFileRead::Move(char* buf, int buflen)
{
   return Read(buf,buflen);
}


//////////////////////////////////////////////////////////////
// TagFileOpen
//

ITagFile* TagFileBase::Open(const char* filename, TagFileOpenMode mode)
{
   TagFileBase* result = NULL;
   switch(mode)
   {
      case kTagOpenRead:
         result = new TagFileRead(filename);
         break;
      case kTagOpenWrite:
         result = new TagFileWrite(filename);
         break;
      default:
         return NULL;
   }
   if (result->file == NULL)
   {
      delete result;
      result = NULL;
   }
   return result;
}

////////////////////////////////////////

STDMETHODIMP_(long) TagFileRead::Write(const char* , int )
{
   Warning(("Writing to a tagfile opened for reading\n"));
   return -1;
}


ITagFile* TagFileOpen(const char* filename, TagFileOpenMode mode)
{
   return TagFileBase::Open(filename,mode);
}
