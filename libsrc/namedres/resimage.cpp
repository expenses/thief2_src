#line 46
#include <windows.h>
#include <resimage.h>
#include <resbase.h>
#include <tmgr.h>
#include <mode.h>
#include <grspoint.h>
struct grs_bitmap;
extern "C" int md_sizeof_mipmap_bits(grs_bitmap* mmap);
#include <mdutil.h>

#include <bmp.h>
#include <cel.h>
#include <gif.h>
#include <pcx.h>
#include <tga.h>
#include <bitmap.h>

extern ulong gResImageFlat16Format;
extern ushort gResImageChromaKey;

void ImageHackRemap16Bit(grs_bitmap* pBm, uint8 mip);
void ImageHackAutoSetTransparency(grs_bitmap* pBm, uint8 mip);
void FlagsXorMip(grs_bitmap* pBm, int flag);
void FlagsOrMip(grs_bitmap* pBm, int16 flag);

cImageResource::cImageResource(IStore* pStore, const char* pName, IResType* pType, eImgKind kind)
	: cResourceBase<IRes, &IID_IRes>(pStore, pName, pType), m_Kind(kind)
{
}

void* cImageResource::LoadData(ulong* pSize, ulong* pTimestamp, IResMemOverride* pResMem)
{
	if (!pResMem)
		return NULL;

	cAutoResThreadLock lock;

	grs_bitmap* bm = NULL;
	IStoreStream* pStream = OpenStream();
	if (!pStream)
	{
#ifndef SHIP
		CriticalMsg1("Unable to open stream: %s", GetName());
#endif
		return NULL;
	}

	switch (m_Kind)
	{
	case kImgBMP:
		bm = ResBmpReadImage(pStream, pResMem);
		break;
	case kImgCEL:
		bm = ResCelReadImage(pStream, pResMem);
		break;
	case kImgGIF:
		bm = ResGifReadImage(pStream, pResMem);
		break;
	case kImgPCX:
		bm = ResPcxReadImage(pStream, pResMem);
		break;
	case kImgTGA:
		bm = ResTgaReadImage(pStream, pResMem);
		break;
	default:
		break;
	}

	if (pTimestamp)
		*pTimestamp = pStream->LastModified();

	pStream->Close();
	pStream->Release();
	if (!bm)
		return NULL;

	ImageHackRemap16Bit(bm, 0);
	ImageHackAutoSetTransparency(bm, 0);
	if (pSize)
		*pSize = pResMem->GetSize(bm);

	return bm;
}

BOOL cImageResource::FreeData(void* pUntypedData, ulong nSize, IResMemOverride* pResMem)
{
	cAutoResThreadLock lock;
	grs_bitmap* pData = (grs_bitmap*)pUntypedData;
#line 145
	AssertMsg(pData, "FreeData: nothing to free!");

	if (!m_pAssignedData && (pData->flags & BMF_LOADED) && g_tmgr)
		g_tmgr->unload_texture(pData);

	pResMem->ResFree(pData);

	return TRUE;
}

int cImageResource::ExtractPartial(const long nStart, const long nEnd, void* pBuf)
{
	long nNumToRead;
	long nSize;
	bool bLocked = false;

	if (pBuf)
	{
		cAutoResThreadLock lock;

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
			nNumToRead = nSize - nStart;
		else
			nNumToRead = nEnd - nStart + 1;
		memmove(pBuf, (char*)pData + nStart, nNumToRead);

		if (bLocked)
			m_pResMan->UnlockResource(this);

		return nNumToRead;
	}
	return 0;
}

void cImageResource::ExtractBlocks(void* pBuf, const long nSize, tResBlockCallback callback, void* pCallbackData)
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


