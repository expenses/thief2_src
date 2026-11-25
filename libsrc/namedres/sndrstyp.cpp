#include <string.h>
#include <reslock.h>
#include <sndrstyp.h>
#include <storeapi.h>
#include <resapi.h>
#include <resbase.h>
#include <reshelp.h>
#include <aggmemb.h>
#include <config.h>
#include <lgsound.h>
#include <imaadpcm.h>
#include <oggdec.h>

enum eSoundResFormat
{
	kSoundResWav,
	kSoundResVoc,
	kSoundResOgg,
	kSoundResMp3,
};

enum eSndDecompressOnLoad
{
	kSndDecompressNone,
	kSndDecompressOgg,
	kSndDecompressAll,
};

static int g_SndDecompressOnLoad = kSndDecompressOgg;

class cSoundResource : public cResourceBase<IRes, &IID_IRes>
{
public:
	cSoundResource(IStore *pStore, const char *pName, IResType *pType, const char *pExt)
		: cResourceBase<IRes, &IID_IRes>(pStore, pName, pType)
	{
		m_Format = -1;
		m_Header.tag[0] = 0;
		m_Header.tag[1] = 0;
		m_Header.tag[2] = 0;
		m_Header.tag[3] = 0;
		if (stricmp(pExt, ".wav") == 0)
			m_Format = kSoundResWav;
		else if (stricmp(pExt, ".voc") == 0)
			m_Format = kSoundResVoc;
		else if (stricmp(pExt, ".ogg") == 0)
			m_Format = kSoundResOgg;
		else if (stricmp(pExt, ".mp3") == 0)
			m_Format = kSoundResMp3;
	}

	STDMETHOD_(int, ExtractPartial)(const long nStart, const long nEnd, void *pBuf);
	STDMETHOD_(void *, LoadData)(ulong *pSize, ulong *pTimestamp, IResMemOverride *pResMem);

private:
	int m_Format;
	sDrkSHeader m_Header;
};

#define DrkSTag (*(uint32 *)"DrkS")

class cSoundResourceType :
	public IResType
{
public:
	DECLARE_UNAGGREGATABLE();

	// Get the name of this type. This is an arbitrary static string,
	// which should ideally be unique.
	const char* STDMETHODCALLTYPE GetName();

	// Get the extensions that this type supports. This will call the callback
	// once for each supported extension.
	void STDMETHODCALLTYPE EnumerateExts(tResEnumExtsCallback callback, void* pClientData);

	// Return TRUE iff the given extension is legal for this type. Arguably
	// redundant with EnumerateExts, but it is useful for efficiency reasons
	// to have both.
	BOOL STDMETHODCALLTYPE IsLegalExt(const char* pExt);

	// Create a new resource of this type from the specified IStore, with
	// the given name. If ppResMem is non-null, it gives the memory
	// manager for this data. Returns NULL iff it can't create the IRes
	// for some reason. It should make sure that the resource knows its
	// name and storage, from the params.
	//
	// Note that this does not actually read any data in, or even open
	// up the stream in the IStore; it just sets things up so that later
	// operations can do so.
	IRes* STDMETHODCALLTYPE CreateRes(
		IStore* pStore,
		const char* pName,
		const char* pExt,
		IResMemOverride** ppResMem);
};

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cSoundResourceType, IResType);

const char* STDMETHODCALLTYPE cSoundResourceType::GetName()
{
	return RESTYPE_SOUND;
}

void STDMETHODCALLTYPE cSoundResourceType::EnumerateExts(tResEnumExtsCallback callback, void* pClientData)
{
	callback(".wav", this, pClientData);
	callback(".voc", this, pClientData);
	callback(".ogg", this, pClientData);
}

BOOL STDMETHODCALLTYPE cSoundResourceType::IsLegalExt(const char* pExt)
{
	return stricmp(pExt, ".wav") == 0 || stricmp(pExt, ".voc") == 0 || stricmp(pExt, ".ogg") == 0;
}

IRes* STDMETHODCALLTYPE cSoundResourceType::CreateRes(IStore* pStore, const char* pName, const char* pExt, IResMemOverride** ppResMem)
{
	if (!IsLegalExt(pExt))
		return NULL;
	return new cSoundResource(pStore, pName, this, pExt);
}

STDMETHODIMP_(int) cSoundResource::ExtractPartial(const long nStart, const long nEnd, void *pBuf)
{
	long nSize;
	char buf[256];
	void *pData;
	ulong nSamples;
	int nHeaderBytes, nRead;
	long nFirst, nLast;

	nRead = 0;
	if (!pBuf || nEnd <= nStart)
		return 0;

	ResManLock();
	pData = m_pResMan->FindResource(this, &nSize);
	if (pData && nSize)
	{
		int n = (nEnd >= nSize) ? nSize - nStart : nEnd - nStart + 1;
		memcpy(pBuf, (char *)pData + nStart, n);
		ResManUnlock();
		return n;
	}

	if (m_Format == kSoundResOgg || m_Format == kSoundResMp3)
	{
		char *p = (char *)Lock();
		if (!p)
		{
			ResManUnlock();
			return 0;
		}
		nSize = GetSize();
		int n = (nEnd >= nSize) ? nSize - nStart : nEnd - nStart + 1;
		memcpy(pBuf, p + nStart, n);
		Unlock();
		ResManUnlock();
		return n;
	}

	if (!m_pStream)
	{
		m_pStream = OpenStream();
		if (!m_pStream)
		{
			ResManUnlock();
			return 0;
		}
	}

	nFirst = nStart - sizeof(sDrkSHeader);
	nLast = nEnd - sizeof(sDrkSHeader);
	nHeaderBytes = 0;
	if (nFirst < 0)
	{
		if (!*(uint32 *)m_Header.tag)
		{
			if (m_pStream->GetPos())
				m_pStream->SetPos(0);
			long nLen = m_pStream->Read(sizeof(buf), buf);
			memset(&m_Header.attribs, 0, sizeof(m_Header) - sizeof(m_Header.tag));
			*(uint32 *)m_Header.tag = DrkSTag;
			if (SndCrackRezHeader(buf, nLen, &pData, &m_Header.dataLen, &nSamples, &m_Header.attribs))
				*(uint32 *)m_Header.tag = 0;
			else
				m_Header.dataOffset = (char *)pData - buf + sizeof(sDrkSHeader);
			if (!*(uint32 *)m_Header.tag)
			{
				ResManUnlock();
				return 0;
			}
		}
		if (nLast > 0)
		{
			nHeaderBytes = -nFirst;
			memcpy(pBuf, (char *)(&m_Header + 1) + nFirst, nHeaderBytes);
			pBuf = (char *)pBuf + nHeaderBytes;
			nFirst = 0;
		}
		else
			memcpy(pBuf, (char *)(&m_Header + 1) + nFirst, nLast - nFirst);
	}

	if (nLast > 0)
	{
		if (nFirst != m_pStream->GetPos())
			m_pStream->SetPos(nFirst);
		nRead = m_pStream->Read(nLast - nFirst + 1, (char *)pBuf);
	}
	ResManUnlock();
	return nHeaderBytes + nRead;
}

STDMETHODIMP_(void *) cSoundResource::LoadData(ulong *pSize, ulong *pTimestamp, IResMemOverride *pResMem)
{
	IStoreStream *pStream;
	sDrkSHeader *pHdr;
	void *pRaw;
	ulong nSamples;
	long nSize;

	if (!pResMem)
		return NULL;

	ResManLock();
	pStream = OpenStream();
	if (!pStream)
	{
		ResManUnlock();
		return NULL;
	}

	nSize = pStream->GetSize() + sizeof(sDrkSHeader);
	if (pSize)
		*pSize = nSize;
	if (pTimestamp)
		*pTimestamp = pStream->LastModified();

	pHdr = (sDrkSHeader *)pResMem->ResMalloc(nSize);
	pStream->Read(nSize - sizeof(sDrkSHeader), (char *)(pHdr + 1));
	pStream->Close();
	pStream->Release();

	memset(&pHdr->attribs, 0, sizeof(*pHdr) - sizeof(pHdr->tag));
	*(uint32 *)pHdr->tag = DrkSTag;
	if (SndCrackRezHeader(pHdr + 1, nSize - sizeof(sDrkSHeader), &pRaw, &pHdr->dataLen, &nSamples, &pHdr->attribs))
		pHdr->dataOffset = 0;
	else
		pHdr->dataOffset = (char *)pRaw - (char *)pHdr;

	sSndAttribs *pAttribs = &pHdr->attribs;
	uint32 nNumSamples = pAttribs->numSamples;
	if (nNumSamples
	    && ((g_SndDecompressOnLoad >= kSndDecompressAll && pAttribs->dataType >= kSndDataIMAADPCM)
	        || (g_SndDecompressOnLoad >= kSndDecompressOgg && pAttribs->dataType > kSndDataIMAADPCM)))
	{
		sDrkSHeader *pOld = pHdr;
		char *pIn = (char *)pOld + pOld->dataOffset;
		long nNewSize = 2 * pAttribs->nChannels * nNumSamples + sizeof(sDrkSHeader);

		pHdr = (sDrkSHeader *)pResMem->ResMalloc(nNewSize);
		if (pSize)
			*pSize = nNewSize;
		*pHdr = *pOld;
		pHdr->attribs.dataType = kSndDataPCM;
		pHdr->attribs.bitsPerSample = 16;
		pHdr->attribs.bytesPerBlock = 2 * pAttribs->nChannels;
		pHdr->dataLen = nNewSize - sizeof(sDrkSHeader);
		pHdr->dataOffset = sizeof(sDrkSHeader);
		pHdr->attribs.samplesPerBlock = 1;

		short *pOut = (short *)(pHdr + 1);
		switch (pAttribs->dataType)
		{
			case kSndDataIMAADPCM:
			{
				IMA_ADPCM Decoder;
				BOOL bStereo = (pAttribs->nChannels == 2);
				uint32 nLeft = pOld->attribs.numSamples;
				uint32 nInBlock = 0;
				while (nLeft)
				{
					BOOL bHeader = FALSE;
					if (!nInBlock)
					{
						nInBlock = pOld->attribs.samplesPerBlock;
						bHeader = TRUE;
					}
					uint32 n = (nLeft < nInBlock) ? nLeft : nInBlock;
					if (bHeader)
						pIn += Decoder.DecompressBlock(bStereo, pIn, pOut, n);
					else
						pIn += Decoder.Decompress(pIn, pOut, n);
					nInBlock -= n;
					nLeft -= n;
					pOut += pAttribs->nChannels * n;
				}
				break;
			}
			case kSndDataOgg:
			{
				cOggDecoder Decoder;
				Decoder.DecompressBlock(pAttribs->nChannels == 2, (char *)pOld, pOut, nNumSamples);
				break;
			}
		}
		pResMem->ResFree(pOld);
	}

	m_Header = *pHdr;
	ResManUnlock();
	return pHdr;
}

IResType* MakeSoundResourceType()
{
	config_get_int("snd_decompress_on_load", &g_SndDecompressOnLoad);
	return new cSoundResourceType();
}
