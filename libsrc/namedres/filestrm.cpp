#include <filestrm.h>
#include <filepath.h>
#include <lgassert.h>
#include <str.h>
#include <comtools.h>
#include <resguid.h>

#include <io.h>
#include <cstdio>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cFileStream
//


STDMETHODIMP cFileStream::QueryInterface(REFIID riid, void **ppvObj)
{
	if (&riid != &IID_IStoreStream && &riid != &IID_IUnknown
		&& memcmp(&riid, &IID_IStoreStream, sizeof(GUID)) != 0
		&& memcmp(&riid, &IID_IUnknown, sizeof(GUID)) != 0)
	{
		*ppvObj = NULL;
		return E_NOINTERFACE;
	}
	*ppvObj = static_cast<IStoreStream *>(this);
	AddRef();
	return S_OK;
}

STDMETHODIMP_(ULONG) cFileStream::AddRef()
{
	return __m_ulRefs.AddRef();
}

STDMETHODIMP_(ULONG) cFileStream::Release()
{
	if (__m_ulRefs.Release())
		return __m_ulRefs;

	OnFinalRelease();
	return 0;
}

cFileStream::cFileStream(IStore* pStore)
	: __m_ulRefs(),
	m_pStorage(pStore),
	m_pFile(0),
	m_pFileSpec(0),
	m_nLastPos(0),
	m_nOpenCount(0)
{
	if (m_pStorage)
		m_pStorage->AddRef();
}

///////////////////////////////////////

cFileStream::~cFileStream()
{
	if (m_pFile)
	{
		char* pName;
		GetName(&pName);

#line 55
		CriticalMsg1("Destroying stream %s that was not fully closed!", pName);
#line 45
		Free(pName);
		fclose(m_pFile);
		m_pFile = NULL;
	}
	if (m_pStorage)
	{
		m_pStorage->Release();
		m_pStorage = NULL;
	}

	if (m_pFileSpec)
	{
		delete m_pFileSpec;
		m_pFileSpec = NULL;
	}
}

///////////////////////////////////////

void cFileStream::SetName(const char* pName)
{
	if (!pName)
		return;

	if (m_pFileSpec)
	{
		delete m_pFileSpec;
		m_pFileSpec = NULL;
	}

	if (!m_pStorage)
		m_pFileSpec = new cFileSpec(pName);
	else
	{
		cFilePath path(m_pStorage->GetFullPathName());

		m_pFileSpec = new cFileSpec(path, pName);
	}
}
///////////////////////////////////////
BOOL cFileStream::Open()
{
	if (m_pFile)
	{
		++m_nOpenCount;
		return TRUE;
	}

	if (!m_pFileSpec)
		return FALSE;

	char* pFileName;
	GetName(&pFileName);

	m_pFile = fopen(pFileName, "rb");
	Free(pFileName);
	if (!m_pFile)
		return FALSE;
	m_nLastPos = 0;
	++m_nOpenCount;
	return TRUE;
}

void cFileStream::Close()
{
	if (!m_pFile)
		return;
	--m_nOpenCount;

	if (m_nOpenCount == 0)
	{
		fclose(m_pFile);
		m_pFile = NULL;
	}
}

void cFileStream::GetName(char** ppName)
{
	cStr name;

	m_pFileSpec->GetNameString(name, kFullPathNameStyle);
	*ppName = name.Detach();
}

BOOL cFileStream::SetPos(long nPos)
{
	if (!m_pFile)
		return FALSE;

	m_nLastPos = nPos;
	return fseek(m_pFile, nPos, 0) == 0;
}

long cFileStream::GetPos()
{
	if (!m_pFile)
		return 0;

	return m_nLastPos;
}

long cFileStream::ReadAbs(long nStartPos, long nEndPos, char* pBuf)
{
	if (!m_pFile)
		return -1;

	if (m_nLastPos != nStartPos && fseek(m_pFile, nStartPos, 0))
		return -1;

	size_t pos = fread(pBuf, 1u, nEndPos - nStartPos + 1, m_pFile);
	m_nLastPos = nStartPos + pos;

	return pos;
}

///////////////////////////////////////

long cFileStream::GetSize()
{
	if (m_pFile)
		return filelength(fileno(m_pFile));

	if (m_pFileSpec)
	{
		char* pFileName;
		GetName(&pFileName);

		m_pFile = fopen(pFileName, "rb");

		Free(pFileName);

		if (!m_pFile)
			return -1;

		long nSize = filelength(fileno(m_pFile));
		fclose(m_pFile);

		return nSize;
	}

	return -1;
}

///////////////////////////////////////

long cFileStream::Read(long nNumBytes, char* pBuf)
{
	if (!m_pFile || nNumBytes < 1)
		return -1;

	size_t pos = fread(pBuf, 1, nNumBytes, m_pFile);
	m_nLastPos += pos;

	return pos;
}

///////////////////////////////////////

int16 cFileStream::Getc()
{
	if (!m_pFile)
		return -1;

	++m_nLastPos;

	return fgetc(m_pFile);
}

///////////////////////////////////////

void cFileStream::ReadBlocks(void* pBuf, long nSize, tStoreStreamBlockCallback callback, void* pCallbackData)
{
	int bDone = 0;
	int nBlockIx = 0;
	long nRead;

	if (!m_pFile)
		return;

	do
	{
		nRead = fread(pBuf, 1u, nSize, m_pFile);
		if (nRead < nSize)
			bDone = 1;

		if (callback)
		{
			nSize = callback(pBuf, nRead, nBlockIx, pCallbackData);
			if (nSize < 1)
				bDone = 1;
		}

		++nBlockIx;
	} while (!bDone);

	m_nLastPos += nRead;
}

///////////////////////////////////////

ulong cFileStream::LastModified()
{
	return m_pFileSpec->GetModificationTime();
}

