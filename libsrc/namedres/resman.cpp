#include <storeapi.h>
F_DECLARE_INTERFACE(IResManHelper);
F_DECLARE_INTERFACE(IResMem);
#include <resman.h>
#include <storeman.h>


#ifndef SHIP
BOOL g_fResPrintAccesses = FALSE;
BOOL g_fResPrintDrops = FALSE;
#endif

#include <resthred.h>
#include <filespec.h>
#include <str.h>
#include <mprintf.h>
#include <appagg.h>
#include <memall.h>
#include <binrstyp.h>
#include <zipstfct.h>
#include <pathutil.h>

#ifdef SHIP
#define ResStat(x)
#else
#define ResStat(x) x
#endif


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cResMan
//

cResMan::cResMan(IUnknown* pOuter)
	:
	m_pStoreMan(NULL),
	m_pDefSearchPath(NULL),
	m_ResTypeHash(),
	m_ResTypeByNameHash(),
	m_ResTable(),
	m_pSharedCache(NULL),
	m_pCache(NULL),
	m_bPagingEnabled(1),
	m_ppResFactories(NULL),
	m_nResFactories(0),
	m_FreshStamp(1),
#ifndef SHIP
	m_pResStats(NULL),
#endif
	m_fDidInit(0),
	m_DefResMem()
{
#line 457
	MI_INIT_AGGREGATION_4(pOuter, IResMan, IResStats, IResMem, IResManHelper, kPriorityLibrary, NULL);
}

///////////////////////////////////////

cResMan::~cResMan()
{
}

///////////////////////////////////////

HRESULT LGAPI ResCacheCallback(const sCacheMsg* pMsg)
{
	void* pClientContext = pMsg->pClientContext;
	int nMessage = pMsg->message;

	if (nMessage >= 0)
	{
		if (nMessage > 1)
		{
		}
		else
		{
			((cResMan*)pClientContext)->FreeData(((cResourceTypeData*)(pMsg->itemId)), TRUE);
			return 0;
		}
	}
	else
	{
	}
	return 1;
}

///////////////////////////////////////

HRESULT cResMan::Init()
{
	if (m_fDidInit)
		return S_FALSE;

	m_pSharedCache = AppGetObj(ISharedCache);
#line 121
	AssertMsg(m_pSharedCache, "cResMan: no shared cache is available!");

	sCacheClientDesc cacheDesc;
	cacheDesc.pID = &IID_IResMan;
	cacheDesc.pContext = this;
	cacheDesc.pfnCallback = ResCacheCallback;
	cacheDesc.nMaxBytes = -1;
	cacheDesc.nMaxItems = -1;
	cacheDesc.flags = 0;

	m_pSharedCache->AddClient(&cacheDesc, &m_pCache);
	AssertMsg(m_pCache, "cResMan: couldn't get a resource cache!");

	IResType* pDefResType = MakeBinaryResourceType();
	RegisterResType(pDefResType);

	pDefResType->Release();

	m_pStoreMan = new cStorageManager();

	IStoreFactory* pZipStore = MakeZipStorageFactory();
	RegisterStoreFactory(pZipStore);

	pZipStore->Release();

	ResStat(m_pResStats = new cResStats());
	m_fDidInit = true;

	return S_OK;
}

///////////////////////////////////////

void cResMan::CleanResources()
{
#ifndef SHIP
	tHashSetHandle NameHandle, TypeHandle;

	for (cResourceName* pNameData = m_ResTable.GetFirst(NameHandle); pNameData != NULL; pNameData = m_ResTable.GetNext(NameHandle))
	{
		if (!(pNameData->m_fFlags & 1))
		{
			for (cResourceData* pData = pNameData->m_pFirstStream; pData != NULL; pData = pData->m_pNext)
			{
				for (cResourceTypeData* pTypeData = pData->m_ResourceTypeHash.GetFirst(TypeHandle);
					pTypeData != NULL; pTypeData = pData->m_ResourceTypeHash.GetNext(TypeHandle))
				{
					int refs;
					if (pTypeData->m_pRes)
					{
						refs = pTypeData->m_pRes->AddRef() - 1;
						pTypeData->m_pRes->Release();
					}
					else
					{
						refs = -1;
					}

					Warning(("Leftover resource %s found at cleanup with lock count of %d, %d references.\n", pTypeData->GetName(), pTypeData->m_nUserLockCount, refs));
				}
			}
		}
	}
#endif
}

///////////////////////////////////////

HRESULT cResMan::End()
{
	if (!m_fDidInit)
		return E_FAIL;

	m_pSharedCache->FlushAll();

	SafeRelease(m_pCache);

	SafeRelease(m_pSharedCache);

	CleanResources();

#ifndef SHIP
	if (m_pResStats)
	{
		delete m_pResStats;
		m_pResStats = NULL;
	}
#endif

	m_ResTable.DestroyAll();
	m_ResTypeHash.DestroyAll();
	m_ResTypeByNameHash.DestroyAll();

	if (m_ppResFactories)
	{
		for (int i = 0; i < m_nResFactories; ++i)
			m_ppResFactories[i]->Release();
		Free(m_ppResFactories);
		m_ppResFactories = NULL;
	}

	m_nResFactories = 0;

	SafeRelease(m_pDefSearchPath);

	m_pStoreMan->Close();
	SafeRelease(m_pStoreMan);

	m_fDidInit = false;

	return S_OK;
}

///////////////////////////////////////

ISearchPath* cResMan::NewSearchPath(const char* pNewPath)
{
	return m_pStoreMan->NewSearchPath(pNewPath);
}

///////////////////////////////////////

void cResMan::SetDefaultPath(ISearchPath* pPath)
{
	cAutoResThreadLock lock;

	if (m_pDefSearchPath)
		m_pDefSearchPath->Release();

	m_pDefSearchPath = pPath;
	if (pPath)
		pPath->AddRef();
}

///////////////////////////////////////

void cResMan::SetGlobalContext(ISearchPath* pPath)
{
	m_pStoreMan->SetGlobalContext(pPath);
}

///////////////////////////////////////

void cResMan::SetDefaultVariants(ISearchPath* pPath)
{
	m_pStoreMan->SetDefaultVariants(pPath);
}

///////////////////////////////////////

void ResTypeEnumerator(const char* pExt, IResType* pType, void* pClientData)
{
	cResMan* pResMan = (cResMan*)pClientData;
	pResMan->InstallResourceType(pExt, pType);
}

///////////////////////////////////////

void ResTypeRemover(const char* pExt, IResType* pType, void* pClientData)
{
	cResMan* pResMan = (cResMan*)pClientData;
	pResMan->RemoveResourceType(pExt, pType);
}

///////////////////////////////////////

BOOL cResMan::RegisterResType(IResType* pType)
{
	cAutoResThreadLock lock;

	if (!pType)
		return FALSE;

	pType->EnumerateExts(ResTypeEnumerator, this);
	if (!m_ppResFactories)
	{
		m_ppResFactories = ((IResType**)(Malloc(sizeof(IResType*))));
		*m_ppResFactories = pType;
		m_nResFactories = 1;
	}
	else
	{
		++m_nResFactories;
		m_ppResFactories = ((IResType**)Realloc(m_ppResFactories, sizeof(IResType*) * m_nResFactories));
		m_ppResFactories[m_nResFactories - 1] = pType;
	}

	pType->AddRef();

	cNamedResType* pTypeEntry = m_ResTypeByNameHash.Search(pType->GetName());
	if (pTypeEntry)
	{
		m_ResTypeByNameHash.Remove(pTypeEntry);
		delete pTypeEntry;
	}

	pTypeEntry = new cNamedResType(pType);
	m_ResTypeByNameHash.Insert(pTypeEntry);

	return TRUE;
}

///////////////////////////////////////

BOOL cResMan::RegisterStoreFactory(IStoreFactory* pStoreFactory)
{
	cAutoResThreadLock lock;

	if (pStoreFactory == NULL || m_pStoreMan == NULL)
	{
		return FALSE;
	}

	m_pStoreMan->RegisterFactory(pStoreFactory);

	return TRUE;
}

///////////////////////////////////////

void cResMan::UnregisterResType(IResType* pType)
{
	cAutoResThreadLock lock;

	if (!pType)
		return;

	pType->EnumerateExts(ResTypeRemover, this);

	if (m_ppResFactories)
	{
		int i;
		for (i = 0; i < m_nResFactories; ++i)
			if (m_ppResFactories[i] == pType)
				break;

		if (i < m_nResFactories)
		{
			m_ppResFactories[i]->Release();
			if (i < m_nResFactories - 1)
				memmove(&m_ppResFactories[i], &m_ppResFactories[i + 1], sizeof(IResType*) * (m_nResFactories - (i + 1)));

			if (!--m_nResFactories)
			{
				Free(m_ppResFactories);
				m_ppResFactories = NULL;
			}
		}
	}

	cNamedResType* pTypeEntry = m_ResTypeByNameHash.Search(pType->GetName());
	if (pTypeEntry && pTypeEntry->m_pType == pType)
	{
		m_ResTypeByNameHash.Remove(pTypeEntry);
		delete pTypeEntry;
	}
}

///////////////////////////////////////

IStore* cResMan::GetStore(const char* pPathName)
{
	if (pPathName == NULL)
	{
		return NULL;
	}

	cAutoResThreadLock lock;

	return m_pStoreMan->GetStore(pPathName, 0);
}

///////////////////////////////////////

void cResMan::MarkForRefresh(IRes* pRes)
{
	cAutoResThreadLock lock;

	if (!pRes) return;

	cResourceTypeData* pData = GetResourceTypeData(pRes);
	if (pData)
		pData->m_Freshed = FALSE;
}

///////////////////////////////////////

void cResMan::GlobalRefresh()
{
	++m_FreshStamp;
}

///////////////////////////////////////

struct sFoundTypedStream
{
	const char* pName;
	const char* pRelPath;
	ISearchPath* pPath;
	char pFoundName[32];
	IStore* pFoundCanonStore;
	IStore* pStore;
};

///////////////////////////////////////

void TryNameWithExt(const char* pExt, IResType*, void* pClientData)
{
	sFoundTypedStream* foundTyped = (sFoundTypedStream*)pClientData;

	if (foundTyped->pStore)
		return;

	strcpy(foundTyped->pFoundName, foundTyped->pName);
	strcat(foundTyped->pFoundName, pExt);

	foundTyped->pStore = foundTyped->pPath->Find(foundTyped->pFoundName, 0, &foundTyped->pFoundCanonStore, foundTyped->pRelPath);
}

///////////////////////////////////////

IRes* cResMan::DoBind(const char* pName, const char* pTypeName, ISearchPath* pPath, const char* pRelPath, uint fBindFlags)
{
	if (!pName || pName[0] == '\0')
	{
		Warning(("Can't Bind Resource with empty name!\n"));
		return NULL;
	}

	if (!pTypeName || pTypeName[0] == '\0')
	{
		Warning(("Can't Bind Resource without a type!\n"));
		return NULL;
	}

	IResType* pType = GetResType(pTypeName);
	if (!pType)
	{
		Warning(("Bind: unknown type specified!\n"));
		return NULL;
	}

	if (!pPath)
	{
		pPath = m_pDefSearchPath;
		if (!pPath)
		{
			Warning(("Bind: no path specified.\n"));
			return NULL;
		}
	}

	char pFullname[32];
	IStore* pStore, * pCanonStore;

	cStr resExt;
	cFileSpec fileSpec(pName);
	fileSpec.GetFileExtension(resExt);

	if (resExt.IsEmpty())
	{
		sFoundTypedStream results;
		results.pName = pName;
		results.pPath = pPath;
		results.pRelPath = pRelPath;
		results.pFoundCanonStore = NULL;
		results.pStore = NULL;

		pType->EnumerateExts(TryNameWithExt, &results);
		pStore = results.pStore;
		if (results.pStore)
		{
			strcpy(pFullname, results.pFoundName);
			pCanonStore = results.pFoundCanonStore;
		}
	}
	else
	{
		if (!pType->IsLegalExt(resExt))
		{
			const char* pszExt = (const char*)resExt;
			Warning(("Illegal extension %s given to bind type %s.\n", pszExt, pTypeName));
			return NULL;
		}

		pStore = pPath->Find(pName, 0, &pCanonStore, pRelPath);
		strcpy(pFullname, pName);
	}

	if (!pStore)
	{
		return NULL;
	}

	IRes* pRes = BindSpecific(pFullname, pTypeName, pStore, pCanonStore, fBindFlags);

	if (pCanonStore)
		pCanonStore->Release();
	pCanonStore = NULL;

	pStore->Release();

	return pRes;
}

///////////////////////////////////////

IRes* STDMETHODCALLTYPE cResMan::Bind(const char* pRelativePathname, const char* pTypeName, ISearchPath* pPath, const char* pExpRelPath, const uint fBindFlags)
{
	char temp[1024];
	strncpy(temp, pRelativePathname, 1023);
	temp[1023] = '\0';

	const char* pRelPath;
	char OldSlash[4];
	BOOL bCombined;
	const char* pName;

	MungePaths(temp, pExpRelPath, (char**)(&pRelPath), (char**)(&pName), OldSlash, &bCombined);
	IRes* pRes = DoBind(pName, pTypeName, pPath, pRelPath, fBindFlags);
	RestorePath((char*)(pRelPath), (char*)(pName), OldSlash[0], bCombined);

	return pRes;
}

///////////////////////////////////////

void STDMETHODCALLTYPE cResMan::BindAll(const char* pPattern, const char* pTypeName, ISearchPath* pPath, void(*callback)(IRes*, IStore*, void*), void* pClientData, const char* pRelPath, const uint fBindFlags)
{
	if (!callback)
		return;

	if (!pPath)
	{
		pPath = m_pDefSearchPath;
		if (!pPath)
		{
			Warning(("BindAll called without a search path!\n"));
			return;
		}
	}

	if (!pTypeName || pTypeName[0] == '\0')
	{
		Warning(("Can't Bind Resource without a type!\n"));
		return;
	}

	IResType* pType = NULL;
	if (pTypeName)
	{
		pType = GetResType(pTypeName);
		if (pType == NULL)
			return;
	}

	IStoreStream* pCookie = (IStoreStream*)pPath->BeginContents(pPattern, 0, pRelPath);
	if (!pCookie)
		return;

	char pResName[32];
	IRes* pRes;
	IStore* pStore, * pCanonStore;

	while (pPath->Next(pCookie, &pStore, pResName, &pCanonStore))
	{
		if (pType)
		{
			cStr resExt;
			cFileSpec fileSpec(pResName);
			fileSpec.GetFileExtension(resExt);

			if (!pType->IsLegalExt(resExt))
			{
				pCanonStore->Release();
				pStore->Release();
				continue;
			}
		}

		pRes = GetResource(pResName, pTypeName, pStore);
		if (!pRes)
			pRes = BindSpecific(pResName, pTypeName, pStore, pCanonStore, fBindFlags);

		if (pRes)
		{
			callback(pRes, pStore, pClientData);
			pRes->Release();
		}

		pCanonStore->Release();
		pStore->Release();
	}

	pPath->EndContents(pCookie);
}

///////////////////////////////////////

IRes* STDMETHODCALLTYPE cResMan::BindSpecific(const char* pName, const char* pTypeName, IStore* pStore, IStore* pCanonStore, const uint fBindFlags)
{
	cAutoResThreadLock lock;
	IRes* pResource;

#line 735
	AssertMsg(pTypeName && *pTypeName != '\0' && pName && *pName != '\0' && pStore != NULL, "Missing arguments for BindSpecific!");

	pResource = NULL;

	if (VerifyStorage(pStore))
	{
		pResource = GetResource(pName, pTypeName, pStore);
		if (pResource)
			return pResource;

		if (!pStore->StreamExists(pName))
			return NULL;

		cAnsiStr resExt;
		cFileSpec fileSpec(pName);
		fileSpec.GetFileExtension(resExt);

		pResource = CreateResource(pStore, pName, resExt, pTypeName, fBindFlags);
		if (pResource)
		{
			IResControl* pResControl;
			if (FAILED(pResource->QueryInterface(IID_IResControl, (void**)&pResControl)))
			{
#line 770
				CriticalMsg("BindSpecific: resource lacks IResControl!");
				pResource->Release();
				return NULL;
			}

			pResControl->AllowStorageReset(!(fBindFlags & 1));
			pResControl->SetCanonStore(pCanonStore);
			pResControl->Release();
		}

		ResStat(m_pResStats->LogStatRes(pResource, ResourceBindings));
	}

	return pResource;
}

///////////////////////////////////////

IRes* STDMETHODCALLTYPE cResMan::Retype(IRes* pOldRes, const char* pTypeName, const uint fBindFlags)
{
	if (!pOldRes || !pTypeName)
		return NULL;

	IStore* pStore = pOldRes->GetStore();
	AssertMsg(pStore, "Resource without a storage!");

	char* pStreamName;
	pOldRes->GetStreamName(0, &pStreamName);
	if (pStreamName)
	{
		IStore* pCanonStore = NULL;
		pCanonStore = pOldRes->GetCanonStore();
		IRes* pNewRes = BindSpecific(pStreamName, pTypeName, pStore, pCanonStore, fBindFlags);

		pCanonStore->Release();
		operator delete(pStreamName);
		pStore->Release();

		return pNewRes;
	}
	return NULL;
}

///////////////////////////////////////

IRes* cResMan::WalkNameChain(cResourceData* pData, const char*, const char* pTypeName, const char* pCanonPath)
{
#line 834
	AssertMsg(pTypeName != NULL, "No type given to WalkNameChain!");

	while (pData)
	{
		cResourceTypeData* pTypeData = pData->m_ResourceTypeHash.Search(pTypeName);
		if (pTypeData)
		{
			if (pCanonPath)
			{
				char* pResCanonPath;
				pTypeData->m_pRes->GetCanonPath(&pResCanonPath);
				if (strcmpi(pCanonPath, pResCanonPath) == 0)
				{
					pTypeData->m_pRes->AddRef();
					operator delete(pResCanonPath);
					return pTypeData->m_pRes;
				}

				operator delete(pResCanonPath);
			}
			else
			{
				pTypeData->m_pRes->AddRef();
				return pTypeData->m_pRes;
			}
		}

		pData = pData->m_pNext;
	}

	return NULL;
}

///////////////////////////////////////

IRes* cResMan::DoLookup(const char* pName, const char* pTypeName, const char* pRawCanonPath)
{
	if (!pTypeName || pTypeName[0] == '\0')
	{
		Warning(("Can't Lookup Resource without a type!\n"));
		return NULL;
	}

	IRes* pRes = NULL;

	if (!pName || pName[0] == '\0')
	{
		Warning(("Lookup called with empty name.\n"));
		return NULL;
	}

	cResourceName* pNameData = m_ResTable.Search(pName);
	if (!pNameData)
		return NULL;

	char* pCanonPath = NULL;
	if (pRawCanonPath)
		GetNormalizedPath(pRawCanonPath, &pCanonPath);

	if (pNameData->m_fFlags & 1)
	{
		for (int i = 0; i < pNameData->m_ppFullNames->Size(); ++i)
		{
			cResourceData* pData = m_ResTable.FindResData((*pNameData->m_ppFullNames)[i], NULL, 0);
			if (pData)
			{
				pRes = WalkNameChain(pData, pName, pTypeName, pCanonPath);
				if (pRes)
				{
					if (pCanonPath)
						operator delete(pCanonPath);
					return pRes;
				}
			}
			else
				Warning(("Lookup: odd -- no entry found for %s.\n", (*pNameData->m_ppFullNames)[i]));
		}

		if (pCanonPath)
			operator delete(pCanonPath);
		return NULL;
	}
	else
	{
		pRes = WalkNameChain(pNameData->m_pFirstStream, pName, pTypeName, pCanonPath);
		if (pCanonPath)
			operator delete(pCanonPath);
		return pRes;
	}
}

///////////////////////////////////////

IRes* cResMan::Lookup(const char* pRelativePathname, const char* pTypeName, const char* pCanonPath)
{
	char temp[1024];
	strncpy(temp, pRelativePathname, 1023);
	temp[1023] = '\0';

	char* pRelPath, * pName, OldSlash[4];
	BOOL bComb;

	MungePaths(temp, pCanonPath, &pRelPath, &pName, OldSlash, &bComb);
	IRes* pRes = DoLookup(pName, pTypeName, pRelPath);
	RestorePath(pRelPath, pName, OldSlash[0], bComb);

	return pRes;
}

///////////////////////////////////////

void cResMan::UnregisterResource(IRes* pResource, ulong ManData)
{
	cAutoResThreadLock lock;

	if (!pResource || !ManData || ManData == -1)
	{
		Warning(("UnregisterResource called for a bogus resource!\n"));
		return;
	}

	cResourceTypeData* pTypeData = (cResourceTypeData*)ManData;
	cResourceData* pResData = pTypeData->m_pResourceData;
	pResData->m_ResourceTypeHash.Remove(pTypeData);
	delete pTypeData;
}

///////////////////////////////////////

void cResMan::CacheAdd(cResourceTypeData* pData)
{
	if (m_bPagingEnabled)
		m_pCache->Add((tCacheItemID)pData, pData->m_pRes, pData->m_nSize);
}

///////////////////////////////////////

BOOL cResMan::CacheRemove(cResourceTypeData* pData)
{
	if (!m_bPagingEnabled)
		return FALSE;

	void* pDummy;
	if (m_pCache->Remove((tCacheItemID)pData, &pDummy) != S_OK)
	{
		Warning(("Tried to remove an item expected but not in the cache (%s)\n", pData->GetName()));
		return FALSE;
	}

	return TRUE;
}

///////////////////////////////////////

void* cResMan::DoLockResource(IRes* pResource, cResourceTypeData* pData)
{
	BOOL bSuccess;
	cResourceTypeData* pFoundData;
	int i;
	char** ppProxiedTypes;
	IResControl* pResControl;
	int nNumTypes;
	int bProxied;

	m_ResManARQ.ClearPreload(pData);
	++pData->m_nUserLockCount;
	if (pData->m_nUserLockCount == 1)
	{
		if (pData->m_Freshed < m_FreshStamp)
		{
			Warning(("Refresh requested for %s; NYI...\n", pData->GetName()));
			pData->m_Freshed = m_FreshStamp;
		}

		if (pData->m_nInternalLockCount == 0)
		{
			nNumTypes = 0;
			bProxied = 0;
			ppProxiedTypes = 0;

			if (pResource->QueryInterface(IID_IResControl, ((void**)(&pResControl))) < 0)
			{
#line 1271
				CriticalMsg1("No IResControl for resource %s!", pResource->GetName());
				return NULL;
			}



			AssertMsg1(pData->m_pData == 0, "Unexpectedly overwriting data for %s", pResource->GetName());

			ppProxiedTypes = pResControl->GetTranslatableTypes(&nNumTypes);
			if (nNumTypes)
			{
				i = 0;
				while (bProxied == 0 && i < nNumTypes)
				{
					pFoundData = pData->m_pResourceData->m_ResourceTypeHash.Search(ppProxiedTypes[i]);

					if (pFoundData && pFoundData->m_pData)
						bProxied = 1;
					else
						++i;
				}

				if (bProxied)
				{
					IResMemOverride* pResMem;
					BOOL bDidAllocate;
					void* pNewData;
					ulong nNewSize;

					nNewSize = 0;

					if (pData->m_pResMem == 0)
						pResMem = &m_DefResMem;
					else
						pResMem = pData->m_pResMem;

#ifndef SHIP
					if (g_fResPrintAccesses)
						mprintf("cResMan::DoLockResource(): Loading resource %s (proxied)\n", pData->GetName());
#endif

					pNewData = pResControl->LoadTranslation(pFoundData->m_pData, pFoundData->m_nSize, ppProxiedTypes[i], &bDidAllocate, &nNewSize, pResMem);

					if (pNewData == 0)
					{
						Warning(("Proxy load failed. Attempting fresh load.\n"));
						bProxied = 0;
					}
					else
					{
						pData->m_pData = pNewData;
						pData->m_nSize = nNewSize;
						if (!bDidAllocate)
						{
							pData->m_pProxiedRes = pFoundData;
							DoLockResource(pResource, pFoundData);
						}

						ResStat(m_pResStats->LogStatRes(pResource, ProxiedLoad));
					}
				}
			}

			if (!bProxied)
			{
#ifndef SHIP
				if (g_fResPrintAccesses)
					mprintf("cResMan::DoLockResource(): Loading resource %s\n", pData->GetName());
#endif

				if (pData->m_pResMem == 0)
					pData->m_pData = pResControl->LoadData(&pData->m_nSize, 0, &m_DefResMem);
				else
					pData->m_pData = pResControl->LoadData(&pData->m_nSize, 0, pData->m_pResMem);

#line 1368
				AssertMsg1(pData->m_pData, "Failed to load resource data %s", pData->GetName());

				ResStat(m_pResStats->LogStatRes(pResource, NewLoad));
				ResStat(m_pResStats->LogStatRes(pResource, MemAlloced));
				pResource->AddRef();
			}
			pResControl->Release();
		}
		else
		{
			bSuccess = CacheRemove(pData);
#line 1387
			AssertMsg1(bSuccess, "Res tried to take %s back from cache, it wasnt there?", pData->GetName());

			pData->m_nInternalLockCount = 0;
			if (bSuccess == 0)
			{
				ResStat(m_pResStats->LogStatRes(pResource, LoadFailed));
				ResStat(m_pResStats->LogStatRes(pResource, LockFailed));
			}
			else
			{
				ResStat(m_pResStats->LogStatRes(pResource, NewLoadLRU));
			}
		}
	}
	else
	{
		ResStat(m_pResStats->LogStatRes(pResource, AlreadyLoaded));
	}

	return pData->m_pData;
}

///////////////////////////////////////
void* cResMan::LockResource(IRes* pResource)
{
	cAutoResThreadLock lock;
	ResStat(m_pResStats->LogStatRes(pResource, LockRequests));
	if (!pResource)
		return NULL;
	cResourceTypeData* pData = GetResourceTypeData(pResource);
	if (!pData)
	{

		CriticalMsg("Resource Lock failed. No private data");
		return NULL;
	}

#line 999
	if (!DoLockResource(pResource, pData))
#line 1425
		CriticalMsg("Resource Lock failed -- unable to lock data.");

	return pData->m_pData;
}

///////////////////////////////////////

void* cResMan::ExtractResource(IRes* pRes, void* pBuf)
{
	cAutoResThreadLock lock;

	if (!pRes || !pBuf)
		return NULL;

	IResControl* pResControl;
	if (FAILED(pRes->QueryInterface(IID_IResControl, (void**)&pResControl)))
	{
#line 1449
		CriticalMsg("Resource without an IResControl!");
		return NULL;
	}

	IStoreStream* pStream = pResControl->OpenStream();

	AssertMsg1(pStream, "Unable to open stream: %s", pRes->GetName());

	pResControl->Release();

	if (!pStream)
		return NULL;

	long nSize = pStream->GetSize();
	pStream->Read(nSize, static_cast<char*>(pBuf));
	pStream->Close();
	pStream->Release();

	return pBuf;
}

///////////////////////////////////////

void* cResMan::FindResource(IRes* pResource, long* pSize)
{
	cAutoResThreadLock lock;

	if (pSize)
		*pSize = 0;

	cResourceTypeData* pData = GetResourceTypeData(pResource);


	AssertMsg(pData, "FindResource -- no private resource data!");

	if ((!pData->m_nUserLockCount && !pData->m_nInternalLockCount)
		|| pData->m_Freshed < m_FreshStamp)
		return NULL;

	if (pSize)
		*pSize = pData->m_nSize;

	if (!pData->m_nUserLockCount && pData->m_nInternalLockCount)
	{
		if (pData->m_pResMem)
			pData->m_pResMem->GetSize(pData->m_pData);
		else
			MSize(pData->m_pData);

		m_pCache->Touch((tCacheItemID)pData);
	}

	return pData->m_pData;
}

///////////////////////////////////////

void cResMan::DoUnlockResource(cResourceTypeData* pData)
{
#line 1515
	AssertMsg1(pData->m_nUserLockCount, "Lock count 0 during Unlock of %s", pData->GetName());

	if (!pData->m_nUserLockCount)
	{
	}
	else
	{
		--pData->m_nUserLockCount;
		if (!pData->m_nUserLockCount)
		{
			if (pData->m_pProxiedRes)
			{
				DoUnlockResource(pData->m_pProxiedRes);
				pData->m_pData = NULL;
				pData->m_nSize = 0;
				pData->m_pProxiedRes = NULL;
			}
			else
			{
#line 1540
				AssertMsg1(!pData->m_nInternalLockCount, "Internal Lock Count non-zero during Unlock of %s", pData->GetName());

				if (pData->m_nInternalLockCount)
				{
				}
				else
				{
					pData->m_nInternalLockCount = 1;
					CacheAdd(pData);
				}
			}
		}
	}
}

///////////////////////////////////////
void cResMan::UnlockResource(IRes* pResource)
{
	cAutoResThreadLock lock;
	cResourceTypeData* pData = GetResourceTypeData(pResource);
	if (pData == NULL)
	{

		CriticalMsg("Unable to unlock resource");
		return;
	}

	DoUnlockResource(pData);
}

///////////////////////////////////////

uint cResMan::GetResourceLockCount(IRes* pResource)
{
	cAutoResThreadLock lock;

	cResourceTypeData* pData = GetResourceTypeData(pResource);
	if (pData == NULL)
	{



		CriticalMsg("Unable to get lock count");
		return 0;
	}

	return pData->m_nUserLockCount;
}

///////////////////////////////////////

BOOL cResMan::DropResourceData(cResourceTypeData* pData)
{
	if (pData->m_nUserLockCount)
		return FALSE;

	if (pData->m_nInternalLockCount)
	{
		CacheRemove(pData);
		FreeData(pData, FALSE);
	}

	return TRUE;
}

///////////////////////////////////////

int cResMan::DropResource(IRes* pResource)
{
	cAutoResThreadLock lock;

	cResourceTypeData* pData = GetResourceTypeData(pResource);
	return DropResourceData(pData);
}

///////////////////////////////////////

long cResMan::GetResourceSize(IRes* pRes)
{
	if (!pRes)
		return 0;

	cAutoResThreadLock lock;

	cResourceTypeData* pData = GetResourceTypeData(pRes);
	if (pData->m_nSize)
		return pData->m_nSize;

	if (pData->m_nUserLockCount == 0 && pData->m_nInternalLockCount == 0)
	{
		IResControl* pResControl;
		if (FAILED(pRes->QueryInterface(IID_IResControl, (void**)&pResControl)))
		{
#line 1656
			CriticalMsg("Resource without an IResControl!");
			return 0;
		}

		IStoreStream* pStream = pResControl->OpenStream();

		AssertMsg1(pStream, "Unable to open stream: %s", pRes->GetName());

		pResControl->Release();

		if (!pStream)
			return 0;

		long nSize = pStream->GetSize();
		pStream->Close();
		pStream->Release();

		return nSize;
	}

	return 0;
}

///////////////////////////////////////

int cResMan::AsyncLock(IRes* pResource, int nPriority)
{
	return m_ResManARQ.Lock(pResource, nPriority);
}

///////////////////////////////////////

int cResMan::AsyncExtract(IRes* pResource, int nPriority, void* pBuf, long bufSize)
{
	return m_ResManARQ.Extract(pResource, nPriority, pBuf, bufSize);
}

///////////////////////////////////////

int cResMan::AsyncPreload(IRes* pResource)
{
	return m_ResManARQ.Preload(pResource);
}

///////////////////////////////////////

int cResMan::IsAsyncFulfilled(IRes* pResource)
{
	return m_ResManARQ.IsFulfilled(pResource);
}

///////////////////////////////////////

long cResMan::AsyncKill(IRes* pResource)
{
	return m_ResManARQ.Kill(pResource);
}

///////////////////////////////////////

long cResMan::GetAsyncResult(IRes* pResource, void** ppResult)
{
	return m_ResManARQ.GetResult(pResource, ppResult);
}

///////////////////////////////////////

void cResMan::EnablePaging(BOOL bEnable)
{
	m_bPagingEnabled = bEnable;
}

///////////////////////////////////////

void cResMan::Compact()
{
	m_pSharedCache->Flush(&IID_IResMan);
}

///////////////////////////////////////

void STDMETHODCALLTYPE cResMan::SetMode(const eResStatMode mode, BOOL bTurnOn)
{
	ResStat(m_pResStats->SetMode(mode, bTurnOn));
}

///////////////////////////////////////

void cResMan::Dump(const char* pFile)
{
	ResStat(m_pResStats->Dump(pFile));
}

///////////////////////////////////////

void cResMan::DumpSnapshot(const char* pFile)
{
	FILE* fp = NULL;
#ifndef SHIP
	if (pFile)
		fp = fopen(pFile, "a+");
	DoDumpSnapshot(fp);

	if (fp)
		fclose(fp);
#endif
}

///////////////////////////////////////

void cResMan::DoDumpSnapshot(FILE* fp)
{
	if (fp)
	{
		fprintf(fp, "Current Resources in Use:\n");
		fprintf(fp, "Canonical Path                 Type\tSize\tExtLock\tIntLock\tRefCnt\n");
	}
	else
	{
		mprintf("Current Resources in Use:\n");
		mprintf("Canonical Path                 Type\tSize\tExtLock\tIntLock\tRefCnt\n");
	}

	tHashSetHandle nameHandle;
	for (cResourceName* pNameEntry = m_ResTable.GetFirst(nameHandle); pNameEntry != NULL; pNameEntry = m_ResTable.GetNext(nameHandle))
	{
		if (!(pNameEntry->m_fFlags & 1))
		{
			for (cResourceData* pResData = pNameEntry->m_pFirstStream; pResData != NULL; pResData = pResData->m_pNext)
			{
				tHashSetHandle h;
				for (cResourceTypeData* pResEntry = pResData->m_ResourceTypeHash.GetFirst(h);
					pResEntry != NULL; pResEntry = pResData->m_ResourceTypeHash.GetNext(h))
				{
					char* pCanonPathName;
					pResEntry->m_pRes->GetCanonPathName(&pCanonPathName);

					char padded_name[33];
					strncpy(padded_name, pCanonPathName, 0x1Du);
					padded_name[29] = '\0';

					char i;
					for (i = strlen(padded_name); i < 30; ++i)
						padded_name[i] = ' ';
					padded_name[i] = '\0';

					ulong refcnt = pResEntry->m_pRes->AddRef() - 1;
					pResEntry->m_pRes->Release();

					if (fp)
						fprintf(fp, "%s%s\t%ld\t%d\t%d\t%d\n", padded_name, pResEntry->m_pType->GetName(), pResEntry->m_nSize, pResEntry->m_nUserLockCount, pResEntry->m_nInternalLockCount, refcnt);
					else
						mprintf("%s%s\t%ld\t%d\t%d\t%d\n", padded_name, pResEntry->m_pType->GetName(), pResEntry->m_nSize, pResEntry->m_nUserLockCount, pResEntry->m_nInternalLockCount, refcnt);

					operator delete(pCanonPathName);
				}
			}
		}
	}
}

///////////////////////////////////////

tResult LGAPI _Res2Create(REFIID, IResMan** ppResMan, IUnknown* pOuter)
{
	cResMan* pResMan = new cResMan(pOuter);

	if (ppResMan)
		*ppResMan = pResMan;

	return pResMan != NULL ? S_OK : E_FAIL;
}

IStoreStream *ResOpenStoreStream(IStore *pStore, const char *pName)
{
	if (!pStore || !pStore->Exists())
		return NULL;
	cAutoResThreadLock lock;
	return pStore->OpenStream(pName, 0);
}

void ResRefreshStorage()
{
	AutoAppIPtr(ResMan);
	cStorageManager *pStoreMan = (cStorageManager *)((cResMan *)(IResMan *)pResMan)->m_pStoreMan;
	if (pStoreMan->m_pRootStore)
		pStoreMan->m_pRootStore->Refresh(2);
}

IStoreStream *ResFindStoreStream(const char *pName, const char *pRelPath, IStore **ppStore)
{
	if (!*pName)
		return NULL;

	ISearchPath *pPath;
	{
		AutoAppIPtr(ResMan);
		pPath = ((cResMan *)(IResMan *)pResMan)->m_pDefSearchPath;
	}
	if (!pPath)
		return NULL;

	IStore *pCanonStore = NULL;
	IStore *pStore = pPath->Find(pName, 0, &pCanonStore, pRelPath);
	if (!pStore)
		return NULL;

	IStoreStream *pStream = NULL;
	if (pStore->Exists())
	{
		cAutoResThreadLock lock;
		pStream = pStore->OpenStream(pName, 0);
		if (pStream && ppStore)
		{
			*ppStore = pStore;
			pStore->AddRef();
		}
	}
	SafeRelease(pCanonStore);
	pStore->Release();
	return pStream;
}

IStoreStream *ResOpenConfigFile(const char *pPathName, const char *pRelPath, IStore **ppStore)
{
	if (!pPathName)
		return NULL;

	char temp[1024];
	char *pRel;
	char *pName;
	char OldSlash;
	int bCombined;
	strncpy(temp, pPathName, sizeof(temp) - 1);
	temp[sizeof(temp) - 1] = '\0';
	cResMan::MungePaths(temp, pRelPath, &pRel, &pName, &OldSlash, &bCombined);
	IStoreStream *pStream = pName ? ResFindStoreStream(pName, pRel, ppStore) : NULL;
	cResMan::RestorePath(pRel, pName, OldSlash, bCombined);
	return pStream;
}
