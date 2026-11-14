#include <strrstyp.h>
#include <resapi.h>
#include <resstr.h>
#include <aggmemb.h>
#include <hashset.h>
#include <string.h>
#include <stdlib.h>

class cStringResourceType :
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

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cStringResourceType, IResType);

const char* STDMETHODCALLTYPE cStringResourceType::GetName()
{
	return RESTYPE_STRING;
}

void STDMETHODCALLTYPE cStringResourceType::EnumerateExts(tResEnumExtsCallback callback, void* pClientData)
{
	callback(".str", this, pClientData);
}

BOOL STDMETHODCALLTYPE cStringResourceType::IsLegalExt(const char* pExt)
{
	return stricmp(".str", pExt) == 0;
}

IRes* STDMETHODCALLTYPE cStringResourceType::CreateRes(IStore* pStore, const char* pName, const char* pExt, IResMemOverride** ppResMem)
{
	return new cStringResource(pStore, pName, this);
}

IResType* MakeStringResourceType()
{
	return new cStringResourceType;
}
