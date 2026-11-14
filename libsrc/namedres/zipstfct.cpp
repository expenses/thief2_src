#line 16
#include <zipstfct.h>
#include <storezip.h>
#include <storeapi.h>
#include <resguid.h>
#include <str.h>
#include <filespec.h>
#include <lgassert.h>
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDefaultStorageFactory
//

class cZipStorageFactory : public IStoreFactory
{
public:
	DECLARE_UNAGGREGATABLE();

	void STDMETHODCALLTYPE EnumerateTypes(tStoreEnumTypeCallback callback, void* pClientData);
	IStore* STDMETHODCALLTYPE CreateStore(IStore* pParent, const char* pName, const char* pExt);
};

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cZipStorageFactory, IStoreFactory);

void cZipStorageFactory::EnumerateTypes(tStoreEnumTypeCallback callback, void* pClientData)
{
	callback(".crf", this, pClientData);
	callback(".zip", this, pClientData);
}

///////////////////////////////////////

IStore* cZipStorageFactory::CreateStore(IStore* pParent, const char* pName, const char* pExt)
{


    AssertMsg(pParent, "cZipStorageFactory: no parent!");
	IStoreStream* pDataStream = pParent->OpenStream(pName, 0);

	if (!pDataStream)
		return NULL;

	cStr root;
	cFileSpec fileSpec(pName);
	fileSpec.GetFileRoot(root);

	cZipStorage* pStore = new cZipStorage(pParent, pDataStream, root);
	pDataStream->Release();

	IStoreHierarchy* pParentHier;
	if (FAILED(pParent->QueryInterface(IID_IStoreHierarchy, (void**)(&pParentHier))))
	{
#line 77
		CriticalMsg("Couldn't QI a StoreHierarchy!");
		pStore->Release();

		return NULL;
	}

	pParentHier->RegisterSubstorage(pStore, pStore->GetName());
	pParentHier->Release();

	return pStore;
}

///////////////////////////////////////

IStoreFactory* MakeZipStorageFactory()
{
	return new cZipStorageFactory;
}

