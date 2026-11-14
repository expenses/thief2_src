#include <sndrstyp.h>
#include <resapi.h>
#include <resbase.h>
#include <aggmemb.h>

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
}

BOOL STDMETHODCALLTYPE cSoundResourceType::IsLegalExt(const char* pExt)
{
	return stricmp(pExt, ".wav") == 0 || stricmp(pExt, ".voc") == 0;
}

IRes* STDMETHODCALLTYPE cSoundResourceType::CreateRes(IStore* pStore, const char* pName, const char* pExt, IResMemOverride** ppResMem)
{
#ifdef DEBUG
	if (!IsLegalExt(pExt))
	{
		Warning(("cSoundResourceType called for illegal extension %s\n", pExt));
		return NULL;
	}
#endif
	return new cResourceBase<IRes, &IID_IRes>(pStore, pName, this);
}

IResType* MakeSoundResourceType()
{
	return new cSoundResourceType();
}
