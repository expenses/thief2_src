
#include <windows.h>
#include <palrstyp.h>
#include <resapi.h>
#include <respall.h>
#include <resbastm.h>
#include <aggmemb.h>

struct sExtMap
{
	const char* ext;
	ePalKind kind;
};

static sExtMap extMaps[] =
{
  { ".pcx", kPalPCX },
  { ".tga", kPalTGA },
  { ".cel", kPalCEL },
  { ".gif", kPalGIF },
  { ".bmp", kPalBMP },
  { NULL }
}; 

class cPaletteResourceType :
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

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cPaletteResourceType, IResType);

const char* STDMETHODCALLTYPE cPaletteResourceType::GetName()
{
	return RESTYPE_PALETTE;
}

void STDMETHODCALLTYPE cPaletteResourceType::EnumerateExts(tResEnumExtsCallback callback, void* pClientData)
{
	for (sExtMap* p = extMaps; p->ext; ++p)
		callback(p->ext, this, pClientData);
}

BOOL STDMETHODCALLTYPE cPaletteResourceType::IsLegalExt(const char* pExt)
{
	for (sExtMap* p = extMaps; p->ext; ++p)
		if (stricmp(pExt, p->ext) == 0)
			return TRUE;

	return FALSE;
}

IRes* STDMETHODCALLTYPE cPaletteResourceType::CreateRes(IStore* pStore, const char* pName, const char* pExt, IResMemOverride** ppResMem)
{
	ePalKind kind;
	for (sExtMap* p = extMaps; p->ext; ++p)
		if (stricmp(pExt, p->ext) == 0)
		{
			kind = p->kind;
			break;
		}

	return new cPaletteResource(pStore, pName, this, kind);
}

IResType* MakePaletteResourceType()
{
	return new cPaletteResourceType;
}
