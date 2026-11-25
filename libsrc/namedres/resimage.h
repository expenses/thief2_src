#pragma once

#include <resbase.h>

enum eImgKind
{
	kImgBMP,
	kImgCEL,
	kImgGIF,
	kImgPCX,
	kImgTGA,
	kImgPNG,
	kImgDDS
};

struct grs_bitmap;
grs_bitmap* ResReadImage(eImgKind kind, IStoreStream* pStream, IResMemOverride* pResMem);

class cImageResource : public cResourceBase<IRes, &IID_IRes>
{
public:
	cImageResource(IStore* pStore,
		const char* pName,
		IResType* pType,
		eImgKind kind);

	void* STDMETHODCALLTYPE LoadData(ulong* pSize,
		ulong* pTimestamp,
		IResMemOverride* pResMem);

	BOOL STDMETHODCALLTYPE FreeData(void* pData,
		ulong nSize,
		IResMemOverride* pResMem);

	int STDMETHODCALLTYPE ExtractPartial(const long nStart,
		const long nEnd,
		void* pBuf);

	void STDMETHODCALLTYPE ExtractBlocks(void* pBuf,
		const long nSize,
		tResBlockCallback,
		void* pCallbackData);

protected:
	eImgKind m_Kind;
	struct sMaterial *m_pMaterial;
};
