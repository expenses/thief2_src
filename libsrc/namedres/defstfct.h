#pragma once

#include <comtools.h>
#include <storeapi.h>
#include <resguid.h>

class cDefaultStorageFactory : public IStoreFactory
{
public:
	DECLARE_UNAGGREGATABLE();

	void STDMETHODCALLTYPE EnumerateTypes(tStoreEnumTypeCallback callback, void* pClientData);
	IStore* STDMETHODCALLTYPE CreateStore(IStore* pParent, const char* pName, const char* pExt);
};
