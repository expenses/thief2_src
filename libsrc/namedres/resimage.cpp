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
#include <imgpng.h>
#include <imgdds.h>
#include <bitmap.h>
#include <resmtl.h>
#include <mprintf.h>

extern ulong gResImageFlat16Format;
extern ushort gResImageChromaKey;

void ImageHackRemap16Bit(grs_bitmap* pBm, uint8 mip);
void ImageHackAutoSetTransparency(grs_bitmap* pBm, uint8 mip);
void FlagsXorMip(grs_bitmap* pBm, int flag);
void FlagsOrMip(grs_bitmap* pBm, int16 flag);

int g_ResDXTMode;
int g_bForce32BitTextures;
sMaterial *g_pResImageMaterial;

cImageResource::cImageResource(IStore* pStore, const char* pName, IResType* pType, eImgKind kind)
	: cResourceBase<IRes, &IID_IRes>(pStore, pName, pType), m_Kind(kind), m_pMaterial(NULL)
{
}

grs_bitmap* ResReadImage(eImgKind kind, IStoreStream* pStream, IResMemOverride* pResMem)
{
	grs_bitmap* bm = NULL;
	switch (kind)
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
	case kImgPNG:
		bm = ResPngReadImage(pStream, pResMem);
		break;
	case kImgDDS:
		bm = ResDdsReadImage(pStream, pResMem);
		break;
	default:
		return NULL;
	}
	if (bm && (bm->type == BMT_FLAT8 || !(bm->flags & BMF_RGB_MASK)))
		bm->flags |= BMF_MIPMAP;
	return bm;
}

void* cImageResource::LoadData(ulong* pSize, ulong* pTimestamp, IResMemOverride* pResMem)
{
	if (!pResMem)
		return NULL;

	cAutoResThreadLock lock;

	grs_bitmap* bm = NULL;
	IStoreStream* pStream = OpenStream();
	if (!pStream)
		return NULL;

	if (g_bMaterialsEnabled && !m_pMaterial)
		m_pMaterial = MtlLoadForRes(this);

	g_pResImageMaterial = m_pMaterial;
	bm = ResReadImage(m_Kind, pStream, pResMem);
	g_pResImageMaterial = NULL;

	if (pTimestamp)
		*pTimestamp = pStream->LastModified();

	pStream->Close();
	pStream->Release();
	if (!bm)
		return NULL;

	sTexBitmap *pTex = (sTexBitmap *)bm;
	sMaterial *pMtl = m_pMaterial;
	if (pMtl)
	{
		pTex->pMtl = pMtl;
		pMtl->pBitmap = bm;
		if (abs(pMtl->tileFactor[0]) > 1 || abs(pMtl->tileFactor[1]) > 1)
		{
			if (!(pMtl->flags & kMtlTerrainScale))
			{
				pMtl->flags |= kMtlTerrainScale;
				pMtl->terrainScale[0] = pTex->scaleW;
				pMtl->terrainScale[1] = pTex->scaleH;
			}
			for (int i = 0; i < 2; i++)
			{
				if (pMtl->tileFactor[i] < 1)
					pMtl->terrainScale[i] = -(pMtl->terrainScale[i] / pMtl->tileFactor[i]);
				else
					pMtl->terrainScale[i] *= pMtl->tileFactor[i];
				if (pMtl->terrainScale[i] == 0)
					pMtl->terrainScale[i] = 1;
			}
			if (pMtl->terrainScale[0] > 4096 || pMtl->terrainScale[1] > 4096)
			{
				mprintf("WARNING: material for texture '%s' has a tile_factor that results in terrain scale %dx%d (max is 4096x4096)\n",
				        GetName(), pMtl->terrainScale[0], pMtl->terrainScale[1]);
				if (pMtl->terrainScale[0] > 4096)
					pMtl->terrainScale[0] = 4096;
				if (pMtl->terrainScale[1] > 4096)
					pMtl->terrainScale[1] = 4096;
			}
			pMtl->flags |= 0x10000;
			for (int j = 0; j < 2; j++)
				pMtl->tileScale[j] = (pMtl->tileFactor[j] < 1) ? (float)-pMtl->tileFactor[j] : 1.0f / pMtl->tileFactor[j];
		}
		if (pMtl->flags & kMtlTerrainScale)
		{
			pTex->scaleW = (short)pMtl->terrainScale[0];
			pTex->scaleH = (short)pMtl->terrainScale[1];
			pTex->scaleHLog = 0;
			pTex->scaleWLog = 0;
			for (int n = 2; n <= pMtl->terrainScale[0]; n *= 2)
				pTex->scaleWLog++;
			for (int m = 2; m <= pMtl->terrainScale[1]; m *= 2)
				pTex->scaleHLog++;
		}
	}

	ImageHackRemap16Bit(bm, pTex->bTerrain != 0);
	ImageHackAutoSetTransparency(bm, pTex->bTerrain != 0);
	MtlTexEdgePad(pTex);
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

	if (m_pMaterial)
	{
		MtlMaterialFree(m_pMaterial);
		delete m_pMaterial;
		m_pMaterial = NULL;
	}
	if (!m_pAssignedData)
	{
		if ((pData->flags & BMF_LOADED) && g_tmgr)
			g_tmgr->unload_texture(pData);
		gr_close_bitmap(pData);
	}

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


