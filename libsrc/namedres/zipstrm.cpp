// $Source: x:/prj/tech/libsrc/res/RCS/pkzip.cpp $
// $Author: TOML $
// $Date: 1997/01/17 11:04:24 $
// $Revision: 1.6 $
//

#include <windows.h>
#include <stddef.h>
#include <stdlib.h>
#include <zipstrm.h>
#include <zlib.h>
#include <dynfunc.h>

#define EXP_BUFFER_SIZE         12596
#define CMP_ABORT               4
#define ZIP_STORED              0
#define ZIP_DCL_IMPLODED        10

///////////////////////////////////////////////////////////////////////////////
//
// Dynamic loading
//

static void PkFail()
{
#line 63
    CriticalMsg("Failed to locate and load implode.dll!");
    exit(1);
}

typedef unsigned int (*tPkReadFunc)(char *buf, unsigned int *size, void *param);
typedef void         (*tPkWriteFunc)(char *buf, unsigned int *size, void *param);

DeclDynFunc(unsigned int, PkExplode, (tPkReadFunc, tPkWriteFunc, char *, void *));

ImplDynFunc(PkExplode, "implode.dll", "explode", PkFail);

#define DynPkExplode (DynFunc(PkExplode).GetProcAddress())

///////////////////////////////////////////////////////////////////////////////
//
// Expand stream-to-memory
//

const unsigned kReadBufSize = 0x10000;
const unsigned kMinReadSize = 1;

struct sPkExplodeInfo
{
    IStoreStream * pSourceStream;

    const BYTE * pSource;
    const BYTE * pSourceLimit;
    BYTE *       pDest;
    const BYTE * pDestLimit;
    ulong        skip;

    BYTE *       pReadBuf;
    BOOL         fComplete;
};

///////////////////////////////////////

unsigned int PkExplodeReader(char *buf, unsigned int *size, void *param)
{
    // @TBD (toml 01-13-97): We're buffering... is it good?
    sPkExplodeInfo * const pExplodeInfo = (sPkExplodeInfo *) param;

    if (!pExplodeInfo || pExplodeInfo->fComplete)
        return 0;

    if (pExplodeInfo->pSource >= pExplodeInfo->pSourceLimit)
    {
        // Worst case is 0% compression (degenerate cases are eliminated by implosion)
        // the most we'll ever have to read is the size of the uncompressed data
        unsigned targetReadSize, actualReadSize;

        targetReadSize = (pExplodeInfo->skip) ? pExplodeInfo->skip :
                                                pExplodeInfo->pDestLimit - pExplodeInfo->pDest;

        if (targetReadSize > kReadBufSize)
            targetReadSize = kReadBufSize;
        else if (targetReadSize < kMinReadSize)
            targetReadSize = kMinReadSize;

        actualReadSize = pExplodeInfo->pSourceStream->Read(targetReadSize, (char *) pExplodeInfo->pReadBuf);

        pExplodeInfo->pSource         = pExplodeInfo->pReadBuf;
        pExplodeInfo->pSourceLimit    = pExplodeInfo->pSource + actualReadSize;
    }

    const unsigned readLimit = pExplodeInfo->pSourceLimit - pExplodeInfo->pSource;
    if (readLimit)
    {
        const unsigned readSize = (readLimit > *size) ? *size : readLimit;

        memcpy(buf, pExplodeInfo->pSource, readSize);
        pExplodeInfo->pSource += readSize;
        *size = readSize;     // Example code shows setting size.  Is it really needed ? (toml 01-13-97)
        return readSize;
    }
    return 0;
}

///////////////////////////////////////

void PkExplodeWriter(char *buf, unsigned int *size, void *param)
{
    sPkExplodeInfo * const pExplodeInfo = (sPkExplodeInfo *) param;

    unsigned actualSize = *size;

    // If we have more to skip
    if (pExplodeInfo->skip)
    {
        if (pExplodeInfo->skip > actualSize)
        {
            // And it's greater than the present amount, skip and return
            pExplodeInfo->skip -= actualSize;
            return;
        }
        else
        {
            // Otherwise, skip the remaining amount
            actualSize -= pExplodeInfo->skip;
            buf += pExplodeInfo->skip;
            pExplodeInfo->skip = 0;
        }
    }

    // If we're writing past the desired amount, set write to the limit
    if ((pExplodeInfo->pDest + actualSize) > pExplodeInfo->pDestLimit)
        actualSize = pExplodeInfo->pDestLimit - pExplodeInfo->pDest;

    memcpy(pExplodeInfo->pDest, buf, actualSize);
    pExplodeInfo->pDest += actualSize;

    // If we're past the desired amount, skip the rest...
    if (pExplodeInfo->pDest > pExplodeInfo->pDestLimit)
        pExplodeInfo->fComplete = TRUE;
}

///////////////////////////////////////

const int kPkExplodeMaxDest = 0x8000000L;

long PkExplodeStreamToMem(IStoreStream * pSourceStream, void * pDest,
                          long skip, long destMax)
{
    static char * pPkBuffer = NULL;

    if (!pPkBuffer)
#line 99
        pPkBuffer = (char *) operator new(EXP_BUFFER_SIZE + kReadBufSize);

    char *          pWorkBuf = (char *) pPkBuffer;
    sPkExplodeInfo  explodeInfo;

    if (!destMax)
        destMax = kPkExplodeMaxDest;

    explodeInfo.pSourceStream = pSourceStream;
    explodeInfo.pReadBuf     = (BYTE *) pPkBuffer + EXP_BUFFER_SIZE;
    explodeInfo.pSource      = explodeInfo.pReadBuf;
    explodeInfo.pSourceLimit = explodeInfo.pReadBuf;
    explodeInfo.pDest        = (BYTE *) pDest;
    explodeInfo.pDestLimit   = explodeInfo.pDest + destMax;
    explodeInfo.skip         = skip;
    explodeInfo.fComplete    = FALSE;

    unsigned result = DynPkExplode(PkExplodeReader,
                                   PkExplodeWriter,
                                   pWorkBuf,
                                   &explodeInfo);

    if ((result == 0 ||
         (result == CMP_ABORT && explodeInfo.fComplete)) &&
        explodeInfo.pDest <= explodeInfo.pDestLimit)
    {
        return (explodeInfo.pDest - (BYTE *) pDest);
    }

#line 249
    CriticalMsg1("Expansion failed (%d)!", result);
    return 0;
}

///////////////////////////////////////

void* ZAllocInterface(void*, unsigned int items, unsigned int size)
{
#line 239
	return malloc(size * items);
}

///////////////////////////////////////

void ZFreeInterface(void*, void* addr)
{
	free(addr);
}

///////////////////////////////////////

int ZInflateStreamToMem(IStoreStream* pStream, int nStreamSize, void* pData, int nSize)
{
	const long BufferSize = 0x10000;
	if (!nStreamSize)
		return 0;

	char* inputBuffer = ((char*)(operator new(BufferSize)));
	int fFlush = Z_NO_FLUSH;

	long actualReadSize = pStream->Read((nStreamSize < BufferSize ? nStreamSize : BufferSize), inputBuffer);
	if (!actualReadSize)
	{
#ifndef SHIP
#line 324
		CriticalMsg("Inflating empty file!");
#endif
		operator delete(inputBuffer);
		return 0;
	}

	long totalReadSize = actualReadSize;
	if (actualReadSize == nStreamSize)
		fFlush = Z_FINISH;

	z_stream_s zBlock;
	zBlock.avail_in = actualReadSize;
	zBlock.next_in = ((Bytef*)(inputBuffer));
	zBlock.next_out = ((Bytef*)(pData));
	zBlock.avail_out = nSize;
	zBlock.zalloc = ZAllocInterface;
	zBlock.zfree = ZFreeInterface;
	zBlock.opaque = 0;

	int ret;
	ret = inflateInit2(&zBlock, -MAX_WBITS);
#line 349
	AssertMsg1(ret == Z_OK, "zlib inflateInit failed with %d\n", ret);

	do
	{
		if (!zBlock.avail_in && totalReadSize < nStreamSize)
		{
			long nRead = pStream->Read((nStreamSize - totalReadSize < ((long)(BufferSize)) ? nStreamSize - totalReadSize : ((long)(BufferSize))), inputBuffer);
			zBlock.next_in = ((Bytef*)(inputBuffer));
			zBlock.avail_in = nRead;
			totalReadSize += nRead;
		}
		ret = inflate(&zBlock, fFlush);
		if (ret == Z_STREAM_END)
		{
			long actualRead = nSize - zBlock.avail_out;
			inflateEnd(&zBlock);



			operator delete(inputBuffer);
			return actualRead;
		}
		if (ret == Z_OK)
		{
#line 369
			AssertMsg(zBlock.avail_out, "zlib inflate: buffer full before end!");
		}
		else
		{
#line 372
			CriticalMsg1("zlib inflate returned %d!\n", ret);
#line 312
			operator delete(inputBuffer);
			return -1;
		}
	} while (1);
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cZipStream
//

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cZipStream, IStoreStream);

cZipStream::cZipStream(IStore* pStore, cZipStorage* pMaster, cNamedZipStream* pInfo)
	: m_pStorage(pStore),
	m_pMaster(pMaster),
	m_pInfo(pInfo),
	m_pFileSpec(NULL),
	m_nLastPos(0),
	m_pData(0),
	m_nOpenCount(0)
{
#line 403
	AssertMsg(m_pStorage, "Creating Zip stream without a storage.");
	AssertMsg(m_pMaster, "Creating Zip stream without a master.");
	AssertMsg(m_pInfo, "Creating Zip stream without info.");

	m_pStorage->AddRef();
	m_pMaster->AddRef();
}

///////////////////////////////////////

cZipStream::~cZipStream()
{
	if (m_nOpenCount > 0)
	{
		char* pName;
		GetName(&pName);
		Warning(("cZipStream: %s deleted without being fully Closed!\n", pName));
		operator delete(pName);
		operator delete(m_pData);
		m_nOpenCount = 0;
		m_pData = NULL;
	}

	if (m_pStorage)
	{
		m_pStorage->Release();
		m_pStorage = NULL;
	}
	if (m_pMaster)
	{
		m_pMaster->Release();
		m_pMaster = NULL;
	}
	if (m_pFileSpec)
	{


		delete m_pFileSpec;
		m_pFileSpec = NULL;
	}
}
///////////////////////////////////////
void cZipStream::SetName(const char* pName)
{

#line 446
	AssertMsg(pName, "Setting Zip stream name to null");


	if (m_pFileSpec)
	{
		delete m_pFileSpec;
		m_pFileSpec = NULL;
	}

	if (!m_pStorage)
	{
		m_pFileSpec = new cFileSpec(pName);
	}
	else
	{
		cFilePath storePath(m_pStorage->GetFullPathName());
		m_pFileSpec = new cFileSpec(storePath, pName);
	}
}

///////////////////////////////////////

BOOL cZipStream::Open()
{
	if (m_pInfo->m_nCompressionMethod == ZIP_STORED)
	{
		if (m_nOpenCount > 0)
		{
			++m_nOpenCount;
			return TRUE;
		}

		m_nLastPos = 0;
		++m_nOpenCount;
		return TRUE;
	}

	if (m_pData)
	{
		++m_nOpenCount;
		return TRUE;
	}

	m_pData = ((char*)(operator new(m_pInfo->m_nUncompressedSize)));
	if (!m_pData)
		return FALSE;

	IStoreStream* pRealStream = m_pMaster->ReadyStreamAt(m_pInfo->m_nHeaderOffset);

#line 507
	AssertMsg(pRealStream, "Opening zip stream with no real stream!");


#ifndef SHIP
	long nRealSize;
#endif
	if (m_pInfo->m_nCompressionMethod == Z_DEFLATED)
	{
#ifndef SHIP
		nRealSize =
#endif
		ZInflateStreamToMem(
			pRealStream,
			m_pInfo->m_nCompressedSize,
			m_pData,
			m_pInfo->m_nUncompressedSize);
	}
	else if (m_pInfo->m_nCompressionMethod == ZIP_DCL_IMPLODED)
	{
#ifndef SHIP
		nRealSize =
#endif
		PkExplodeStreamToMem(pRealStream, m_pData, 0, m_pInfo->m_nUncompressedSize);
	}


#line 527
	AssertMsg2(nRealSize == m_pInfo->m_nUncompressedSize, "Zip stream: expected size of %ld, got %ld", m_pInfo->m_nUncompressedSize, nRealSize);

	pRealStream->Release();
	m_nLastPos = 0;
	++m_nOpenCount;
	return TRUE;
}
///////////////////////////////////////
void cZipStream::Close()
{

	AssertMsg(m_nOpenCount, "cZipStream closed more than opened");


	if (m_pInfo->m_nCompressionMethod)
	{
		--m_nOpenCount;
		if (!m_nOpenCount)
		{
			operator delete(m_pData);
			m_pData = NULL;
		}
	}
	else
	{
		--m_nOpenCount;
	}
}

///////////////////////////////////////

void cZipStream::GetName(char** ppName)
{
	cStr name;
	m_pFileSpec->GetNameString(name);
	*ppName = name.Detach();;
}

///////////////////////////////////////

BOOL cZipStream::SetPos(long nPos)
{
	if (!m_nOpenCount)
		return FALSE;
	if (nPos >= m_pInfo->m_nUncompressedSize)
		return FALSE;

	m_nLastPos = nPos;
	return TRUE;
}

///////////////////////////////////////

long cZipStream::GetPos()
{
	if (m_nOpenCount)
		return m_nLastPos;

	return -1;
}

///////////////////////////////////////

long cZipStream::ReadAbs(long nStartPos, long nEndPos, char* pBuf)
{
	if (!m_nOpenCount)
		return -1;

	if (!SetPos(nStartPos))
		return -1;

	return Read(nEndPos - nStartPos, pBuf);
}

///////////////////////////////////////

long cZipStream::GetSize()
{
	return m_pInfo->m_nUncompressedSize;
}

///////////////////////////////////////

long cZipStream::Read(long nNumBytes, char* pBuf)
{
	if (!m_nOpenCount)
		return -1;

	if (nNumBytes < 1)
		return -1;

	long nBytesLeft = m_pInfo->m_nUncompressedSize - m_nLastPos;
	if (nBytesLeft <= 0)
		return -1;

	if (nNumBytes > nBytesLeft)
		nNumBytes = m_pInfo->m_nUncompressedSize - m_nLastPos;

	if (!m_pInfo->m_nCompressionMethod)
	{
		IStoreStream* pRealStream = m_pMaster->ReadyStreamAt(m_pInfo->m_nHeaderOffset);

		AssertMsg(pRealStream, "Opening zip stream with no real stream!");

		pRealStream->SetPos(m_nLastPos + pRealStream->GetPos());
		long size = pRealStream->Read(nNumBytes, pBuf);
		pRealStream->Release();
#line 631
		AssertMsg(size == nNumBytes, "Read failed to get the right number of bytes.");
	}
	else
	{
		if (!m_pData)
			return -1;

		memmove(pBuf, &m_pData[m_nLastPos], nNumBytes);
	}

	m_nLastPos += nNumBytes;
	return nNumBytes;
}

///////////////////////////////////////

short cZipStream::Getc()
{
	if (!m_pInfo->m_nCompressionMethod)
	{
		unsigned char c;
		if (m_nOpenCount && Read(1, (char *)&c) == 1)
			return c;
	}
	else if (m_pData && m_nOpenCount && m_nLastPos < m_pInfo->m_nUncompressedSize)
	{
		unsigned char c = m_pData[m_nLastPos++];
		return c;
	}

	return -1;
}

///////////////////////////////////////

void cZipStream::ReadBlocks(void* pBuf, long nSize, tStoreStreamBlockCallback callback, void* pCallbackData)
{
#ifndef SHIP
#line 682
	CriticalMsg("cZipStream::ReadBlocks Not implemented.");
#endif

	int nBlockIx = 0;
	char* p = &m_pData[m_nLastPos];
	char* pEnd = &m_pData[m_pInfo->m_nUncompressedSize];

	if (!m_pData || !m_nOpenCount || !callback)
		return;

	while (p < pEnd)
	{
		long nRead;
		if ((p + nSize) >= pEnd)
			nRead = pEnd - p;
		else
			nRead = nSize;
		memmove(pBuf, p, nRead);
		p += nRead;

		nSize = callback(pBuf, nRead, nBlockIx, pCallbackData);
		if (nSize < 1)
			break;

		++nBlockIx;
	}

	m_nLastPos = p - m_pData;
}

///////////////////////////////////////

ulong cZipStream::LastModified()
{
	return m_pMaster->LastModified();
}

///////////////////////////////////////

