#line 35
#include <windows.h>
#include <respall.h>
#include <resbase.h>
#include <tmgr.h>
#include <mode.h>
#include <mdutil.h>

#include <bmp.h>
#include <cel.h>
#include <gif.h>
#include <pcx.h>
#include <tga.h>
#include <bitmap.h>

cPaletteResource::cPaletteResource(IStore* pStore, const char* pName, IResType* pType, ePalKind kind)
	: cResourceBase<IRes, &IID_IRes>(pStore, pName, pType), m_Kind(kind) {}

void* cPaletteResource::LoadData(ulong* pSize, ulong* pTimestamp, IResMemOverride* pResMem)
{
	if (!pResMem)
		return NULL;

	cAutoResThreadLock lock;

	IStoreStream* pStream = OpenStream();
	if (!pStream)
	{
		AssertMsg1(FALSE, "Unable to open stream: %s", GetName());
		return NULL;
	}

	void* pData;
	switch (m_Kind)
	{
	case kPalBMP:
		pData = ResBmpReadPalette(pStream, pResMem);
		break;
	case kPalCEL:
		pData = ResCelReadPalette(pStream, pResMem);
		break;
	case kPalGIF:
		pData = ResGifReadPalette(pStream, pResMem);
		break;
	case kPalPCX:
		pData = ResPcxReadPalette(pStream, pResMem);
		break;
	case kPalTGA:
		pData = ResTgaReadPalette(pStream, pResMem);
		break;
	default:
		break;
	}

	if (pTimestamp)
		*pTimestamp = pStream->LastModified();

	pStream->Close();
	pStream->Release();

	if (pSize)
		*pSize = pResMem->GetSize(pData);

	return pData;
}

int cPaletteResource::ExtractPartial(const long nStart, const long nEnd, void* pBuf)
{
	long nNumToRead = 0;
	bool bLocked = false;

	if (pBuf)
	{
		cAutoResThreadLock lock;
		long nSize;

		void* pData = m_pResMan->FindResource(this, &nSize);
		if (!pData || !nSize)
		{
			pData = m_pResMan->LockResource(this);
			if (pData)
			{
				bLocked = true;
				nSize = m_pResMan->GetResourceSize(this);
			}
			else
				return 0;
		}

		if (nEnd >= nSize)
		{
			nNumToRead = nSize - nStart;

		}
		else
		{
			nNumToRead = nEnd - nStart + 1;

		}
		memmove(pBuf, (char*)pData + nStart, nNumToRead);
		if (bLocked)
			m_pResMan->UnlockResource(this);

		return nNumToRead;
	}
	return 0;
}

void cPaletteResource::ExtractBlocks(void* pBuf, const long nSize, tResBlockCallback callback, void* pCallbackData)
{
	bool bLocked = false;
	long nIx = 0;
	long nRemain;

	cAutoResThreadLock lock;

	void* pData = m_pResMan->FindResource(this, &nRemain);

	if (!pData || !nSize)
	{
		pData = m_pResMan->LockResource(this);
		if (!pData)
			return;

		bLocked = true;

		nRemain = m_pResMan->GetResourceSize(this);
	}

	while (nRemain > 0)
	{
		long nMove = (nRemain < nSize) ? nRemain : nSize;
		memmove(pBuf, pData, nMove);

		callback(this, pBuf, nMove, nIx++, pCallbackData);

		pData = static_cast<char*>(pData) + nMove;
		nRemain -= nMove;
	}

	if (bLocked)
		m_pResMan->UnlockResource(this);
}


