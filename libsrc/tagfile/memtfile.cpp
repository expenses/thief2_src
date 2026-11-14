

#include <lg.h>
#include <vernum.h>
#include <tagfile.h>
#include <memtfile.h>
#include <memtfil_.h>

#include <config.h>
#include <cfgdbg.h>
#include <tagguid.h>

// implement hash sets
#include <hshsttem.h>

// Must be last header
#include <dbmem.h>

//
// Memory Tag File Iterator
//

class cMemFileIter : public cCTUnaggregated<ITagFileIter, &IID_ITagFileIter, kCTU_Default>
{
public:
	cMemFileIter(cMemTagTable* table) : mpTable(table), mpEntry(NULL), mHandle() { }
	~cMemFileIter() { mpEntry = NULL; }

	STDMETHOD(Start)()
	{
		mpEntry = mpTable->GetFirst(mHandle);
		return S_OK;
	}

	STDMETHOD_(BOOL, Done)()
	{
		return mpEntry == NULL;
	}

	STDMETHOD(Next)()
	{
		if (mpEntry)
			mpEntry = mpTable->GetNext(mHandle);
		return S_OK;
	}

	STDMETHOD_(const TagFileTag*, Tag)(THIS)
	{
		if (mpEntry)
			return &mpEntry->tag;
		else
			return NULL;
	}

protected:
	tHashSetHandle mHandle;
	cMemTagTable* mpTable;
	const sMemTagEntry* mpEntry;
};

//
// Memory Tag File Table
//

cMemTagTable::~cMemTagTable()
{
	tHashSetHandle handle;
	for (sMemTagEntry* e = GetFirst(handle); e != NULL; e = GetNext(handle))
	{
		if (e->buf)
#line 25
			Free(e->buf);
		delete e;
	}
}

sMemTagEntry* cMemTagTable::AddTag(const TagFileTag* tag, const TagVersion* version)
{
	sMemTagEntry* e = new sMemTagEntry;
	e->tag = *tag;
	e->version = *version;
	e->buf_size = kDefaultBufSize;
	e->buf = (uchar*)Malloc(e->buf_size);
	e->block_size = 0;
	Insert(e);
	return e;
}

//
// Tag File Base
//
TagVersion cMemFile::Version = { 1, 0 };
STDMETHODIMP_(ulong) cMemFile::BlockSize(const TagFileTag* tag)
{
	sMemTagEntry* entry = mpTable->Search(tag->label);
	if (entry)
		return entry->block_size;
	else
		return 0;
}

STDMETHODIMP_(const TagFileTag*) cMemFile::CurrentBlock()
{
	if (mpCurBlock)
		return &mpCurBlock->tag;
	else
		return NULL;
}

STDMETHODIMP_(HRESULT) cMemFile::Seek(ulong offset, TagFileSeekMode from)
{
	if (!mpCurBlock)
		return E_FAIL;

	ulong origin = 0;
	if (from == kTagSeekFromHere)
		origin = mCursor;
	else if (from == kTagSeekFromEnd)
		origin = mpCurBlock->block_size;
	mCursor = offset + origin;
	return S_OK;
}

STDMETHODIMP_(ulong) cMemFile::Tell()
{
	return mCursor;
}

STDMETHODIMP_(ulong) cMemFile::TellFromEnd()
{
	return mpCurBlock->block_size - mCursor;
}

STDMETHODIMP_(ITagFileIter*) cMemFile::Iterate()
{
	return new cMemFileIter(mpTable);
}

//
// Read/write tag file class
//

STDMETHODIMP_(HRESULT) cMemFileReadWrite::OpenBlock(const TagFileTag* tag, TagVersion* version)
{
	sMemTagEntry* entry = mpTable->Search(tag->label);

	if (mMode == kTagOpenWrite)
	{
		if (entry)
			return E_FAIL;
		entry = mpTable->AddTag(tag, version);
	}
	else
	{
		if (entry == NULL)
			return E_FAIL;
		*version = entry->version;
	}

	mpCurBlock = entry;
	mCursor = 0;

	return S_OK;
}

STDMETHODIMP_(HRESULT) cMemFileReadWrite::CloseBlock()
{
	if (!mpCurBlock)
		return E_FAIL;
	mpCurBlock = NULL;
	return S_OK;
}

STDMETHODIMP_(long) cMemFileReadWrite::Read(char* buf, int buflen)
{
#line 166
	Assert_(mMode == kTagOpenRead);

	if (!mpCurBlock)
		return 0;
	if (mCursor + buflen > mpCurBlock->block_size)
		buflen = mpCurBlock->block_size - mCursor;
	memcpy(buf, &mpCurBlock->buf[mCursor], buflen);
	mCursor += buflen;
	return buflen;
}
STDMETHODIMP_(long) cMemFileReadWrite::Write(const char* buf, int buflen)
{

	Assert_(mMode == kTagOpenWrite);

	if (!mpCurBlock)
		return 0;

	if (mCursor + buflen > mpCurBlock->buf_size)
	{
		while (mCursor + buflen > mpCurBlock->buf_size)
			mpCurBlock->buf_size *= 2;

		mpCurBlock->buf = reinterpret_cast<uchar*>(Realloc(mpCurBlock->buf, mpCurBlock->buf_size));
	}

	memcpy(&mpCurBlock->buf[mCursor], buf, buflen);

	mCursor += buflen;

	if (mCursor > mpCurBlock->block_size)
		mpCurBlock->block_size = mCursor;

	return buflen;
}

STDMETHODIMP_(long) cMemFileReadWrite::Move(char* buf, int buflen)
{
	if (mMode == kTagOpenWrite)
		return Write(buf, buflen);
	else
		return Read(buf, buflen);
}

class cMemFileFactory : public cTagFileFactory
{
public:
	cMemFileFactory() : mTable() { }

	ITagFile* Open(const char* filename, TagFileOpenMode mode)
	{
		return new cMemFileReadWrite(&mTable, mode);
	}

protected:
	cMemTagTable mTable;
};

cTagFileFactory* CreateMemoryTagFileFactory()
{
	return new cMemFileFactory();
}
