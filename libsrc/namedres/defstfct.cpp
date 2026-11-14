#include <defstfct.h>
#include <storedir.h>
#include <lgassert.h>

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cDefaultStorageFactory, IStoreFactory);

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDefaultStorageFactory
//

void cDefaultStorageFactory::EnumerateTypes(tStoreEnumTypeCallback callback, void* pClientData)
{
}

///////////////////////////////////////

IStore* cDefaultStorageFactory::CreateStore(IStore* pParent, const char* pName, const char* pExt)
{
	BOOL needsData;
	IStore* pStore;
	IStoreHierarchy* pHier;

	if (strcmp(pExt, "zip") == 0)
		needsData = TRUE;
	else
	{
		pStore = new cDirectoryStorage(pName);
		needsData = FALSE;
	}

	if (pParent)
	{
		IStoreHierarchy* pParentHier;
		if (FAILED(pParent->QueryInterface(IID_IStoreHierarchy, ((void**)(&pParentHier)))))
		{
#line 51
			CriticalMsg("Couldn't QI a StoreHierarchy!");
			pStore->Release();
			return NULL;
		}
		pParentHier->RegisterSubstorage(pStore, pStore->GetName());
		pParentHier->Release();
		if (needsData)
		{
			IStoreStream* pDataStream = pParent->OpenStream(pName, 0);
			if (!pDataStream)
			{
				pStore->Release();
				return NULL;
			}
			if (FAILED(pStore->QueryInterface(IID_IStoreHierarchy, ((void**)(&pHier)))))
			{

				CriticalMsg("Couldn't QI a StoreHierarchy!");
				pStore->Release();
				return NULL;
			}

			pHier->SetDataStream(pDataStream);
			pDataStream->Release();
			pHier->Release();
		}
	}

	return pStore;
}
