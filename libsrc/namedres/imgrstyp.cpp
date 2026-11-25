#include <windows.h>
#include <imgrstyp.h>
#include <resapi.h>
#include <resimage.h>
#include <resbastm.h>
#include <aggmemb.h>

struct sExtMap
{
	const char* ext;
	eImgKind kind;
};

static sExtMap extMaps[] =
{
  { ".pcx", kImgPCX },
  { ".tga", kImgTGA },
  { ".cel", kImgCEL },
  { ".gif", kImgGIF },
  { ".bmp", kImgBMP },
  { ".png", kImgPNG },
  { ".dds", kImgDDS },
  { NULL }
};

class cImageResourceType :
	public IResType
{
public:
	DECLARE_UNAGGREGATABLE();

	// Get the name of this type. This is an arbitrary static string,
	// which should ideally be unique.
	const char *STDMETHODCALLTYPE GetName();

	// Get the extensions that this type supports. This will call the callback
	// once for each supported extension.
	void STDMETHODCALLTYPE EnumerateExts(tResEnumExtsCallback callback, void *pClientData);

	// Return TRUE iff the given extension is legal for this type. Arguably
	// redundant with EnumerateExts, but it is useful for efficiency reasons
	// to have both.
	BOOL STDMETHODCALLTYPE IsLegalExt(const char *pExt);

	// Create a new resource of this type from the specified IStore, with
	// the given name. If ppResMem is non-null, it gives the memory
	// manager for this data. Returns NULL iff it can't create the IRes
	// for some reason. It should make sure that the resource knows its
	// name and storage, from the params.
	//
	// Note that this does not actually read any data in, or even open
	// up the stream in the IStore; it just sets things up so that later
	// operations can do so.
	IRes *STDMETHODCALLTYPE CreateRes(
		IStore *pStore,
		const char *pName,
		const char *pExt,
		IResMemOverride **ppResMem);
};

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cImageResourceType, IResType);

const char *STDMETHODCALLTYPE cImageResourceType::GetName()
{
	return RESTYPE_IMAGE;
}

void STDMETHODCALLTYPE cImageResourceType::EnumerateExts(tResEnumExtsCallback callback, void *pClientData)
{
	for (sExtMap* p = extMaps; p->ext; ++p)
		callback(p->ext, this, pClientData);
}

BOOL STDMETHODCALLTYPE cImageResourceType::IsLegalExt(const char *pExt)
{
	for (sExtMap* p = extMaps; p->ext; ++p)
		if (stricmp(pExt, p->ext) == 0)
			return TRUE;

	return FALSE;
}

IRes *STDMETHODCALLTYPE cImageResourceType::CreateRes(IStore *pStore, const char *pName, const char *pExt, IResMemOverride **ppResMem)
{
	eImgKind kind;
	for (sExtMap* p = extMaps; p->ext; ++p)
		if (stricmp(pExt, p->ext) == 0)
		{
			kind = p->kind;
			break;
		}

	return new cImageResource(pStore, pName, this, kind);
}

EXTERN IResType *MakeImageResourceType()
{
	return new cImageResourceType();
}
