#include <storeman.h>
#include <storedir.h>
#include <search.h>
#include <defstfct.h>
#include <filespec.h>
#include <lgassert.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cFactoryEntry
//

class cFactoryHashByExt;

cFactoryEntry::cFactoryEntry(IStoreFactory* pFactory, const char* pExt)
	: m_pExt(NULL)
{
#line 30
	AssertMsg(pExt, "Creating storage factory entry with no extension!");
	AssertMsg(pFactory, "Missing storage factory!");

	m_pFactory = pFactory;
	m_pFactory->AddRef();

	m_pExt = new char[strlen(pExt) + 1];
	strcpy(m_pExt, pExt);
}

cFactoryEntry::~cFactoryEntry()
{
	if (m_pFactory)
	{
		m_pFactory->Release();
		m_pFactory = NULL;
	}

	if (m_pExt)
	{
		delete [] m_pExt;
		m_pExt = NULL;
	}
}

///////////////////////////////////////

class cFactoryHashByExt : public cStrHashSet<cFactoryEntry*>
{
public:
	cFactoryHashByExt();
	~cFactoryHashByExt();

	tHashSetKey GetKey(tHashSetNode node) const;
};

cFactoryHashByExt::cFactoryHashByExt()
{
}

cFactoryHashByExt::~cFactoryHashByExt()
{
	DestroyAll();
}

tHashSetKey cFactoryHashByExt::GetKey(tHashSetNode node) const
{
	return reinterpret_cast<tHashSetKey>(((cFactoryEntry*)(node))->m_pExt);
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cStorageManager
//

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cStorageManager, IStoreManager);

cStorageManager::cStorageManager() :
	m_pRootStore(NULL),
	m_pDefStoreFactory(NULL),
	m_pGlobalContext(NULL),
	m_pDefVariants(NULL),
	m_pFactoryTable(NULL)
{
	m_pRootStore = new cDirectoryStorage(NULL);

	IStoreHierarchy* pStoreHier;
	if (FAILED(m_pRootStore->QueryInterface(IID_IStoreHierarchy, ((void**)(&pStoreHier)))))
#line 86
		CriticalMsg("Root storage has no IStoreHierarchy!");

	pStoreHier->SetStoreManager(this);
	pStoreHier->Release();
	m_pFactoryTable = new cFactoryHashByExt();
	m_pDefStoreFactory = new cDefaultStorageFactory();

	RegisterFactory(m_pDefStoreFactory);
}

///////////////////////////////////////

cStorageManager::~cStorageManager()
{
	if (m_pRootStore)
	{
		Warning(("Storage Manager being deleted without being closed!\n"));
		Close();
	}

	if (m_pFactoryTable)
	{
		delete m_pFactoryTable;
		m_pFactoryTable = NULL;
	}

	if (m_pDefStoreFactory)
	{
		m_pDefStoreFactory->Release();
		m_pDefStoreFactory = NULL;
	}
}

void cStorageManager::InstallStorageType(const char* pExt, IStoreFactory* pFactory)
{
	cFactoryEntry* pEntry = m_pFactoryTable->Search(pExt);
	if (pEntry)
	{
		m_pFactoryTable->Remove(pEntry);
		delete pEntry;
	}

	m_pFactoryTable->Insert(new cFactoryEntry(pFactory, pExt));
}

///////////////////////////////////////

void StorageTypeEnumerator(const char* pExt, IStoreFactory* pFactory, void* pClientData)
{
	((cStorageManager*)(pClientData))->InstallStorageType(pExt, pFactory);
}

///////////////////////////////////////

void cStorageManager::RegisterFactory(IStoreFactory* pFactory)
{
	pFactory->EnumerateTypes(StorageTypeEnumerator, this);
}

///////////////////////////////////////

IStore* cStorageManager::GetStore(const char* pPathName, int bCreate)
{
#line 160
	AssertMsg(m_pRootStore, "Storage Manager doesn't have a root store!");

	return m_pRootStore->GetSubstorage(pPathName, bCreate);
}

///////////////////////////////////////

IStore* cStorageManager::CreateSubstore(IStore* pParent, const char* pName)
{
	cStr ext;
	cFileSpec fileSpec(pName);
	fileSpec.GetFileExtension(ext);

	cFactoryEntry* pFactoryEntry = m_pFactoryTable->Search(ext);

	IStore* pStorage = NULL;
	if (!pFactoryEntry)
		pStorage = m_pDefStoreFactory->CreateStore(pParent, pName, ext);
	else
		pStorage = pFactoryEntry->m_pFactory->CreateStore(pParent, pName, ext);

	if (pStorage)
	{
		IStoreHierarchy* pHier;
		if (FAILED(pStorage->QueryInterface(IID_IStoreHierarchy, ((void**)(&pHier)))))
			Warning(("Storage without an IStoreHierarchy!"));
		else
		{
			pHier->SetStoreManager(this);
			pHier->Release();
		}
	}

	return pStorage;
}

///////////////////////////////////////

void SetContextRoot(ISearchPath*, const char* pRoot, tSearchPathRecursion, void* pClientData)
{
	cStorageManager* pStoreMan = ((cStorageManager*)(pClientData));

	IStore* pStore = pStoreMan->GetStore(pRoot, FALSE);
	if (!pStore)
		return;

	IStoreHierarchy* pStoreHier;
	if (SUCCEEDED(pStore->QueryInterface(IID_IStoreHierarchy, ((void**)(&pStoreHier)))))
	{
		pStoreHier->DeclareContextRoot(TRUE);
		pStoreHier->Release();
	}

	pStore->Release();
}

///////////////////////////////////////

void cStorageManager::SetGlobalContext(ISearchPath* pPath)
{
	if (m_pGlobalContext)
		m_pGlobalContext->Release();

	m_pGlobalContext = pPath;
	if (pPath)
	{
		pPath->AddRef();
		pPath->Iterate(SetContextRoot, TRUE, this);
	}
}

///////////////////////////////////////


void cStorageManager::SetDefaultVariants(ISearchPath* pPath)
{
	if (m_pDefVariants)
		m_pDefVariants->Release();

	m_pDefVariants = pPath;
	if (m_pDefVariants)
		m_pDefVariants->AddRef();
}

///////////////////////////////////////

ISearchPath* cStorageManager::NewSearchPath(const char* pNewPath)
{
	cSearchPath* pNewSearchPath = new cSearchPath(this);

	if (m_pGlobalContext)
		pNewSearchPath->SetContext(m_pGlobalContext);

	if (m_pDefVariants)
		pNewSearchPath->SetVariants(m_pDefVariants);

	if (pNewPath)
		pNewSearchPath->AddPath(pNewPath);

	return pNewSearchPath;
}

///////////////////////////////////////

void cStorageManager::Close()
{
#line 298
	AssertMsg(m_pRootStore, "Trying to Close without a valid Root Storage!");

	IStoreHierarchy* pStoreHier;
	if (FAILED(m_pRootStore->QueryInterface(IID_IStoreHierarchy, ((void**)(&pStoreHier)))))

		CriticalMsg("Root storage has no IStoreHierarchy!");

	pStoreHier->Close();
	pStoreHier->Release();

#line 243
	SafeRelease(m_pRootStore);

	SafeRelease(m_pGlobalContext);





	SafeRelease(m_pDefVariants);




}

///////////////////////////////////////

int cStorageManager::HeteroStoreExists(IStore* pParentStore, const char* pSubStoreName, char* pNameBuffer)
{
	size_t nLen = strlen(pSubStoreName);

	tHashSetHandle hsh;
	for (cFactoryEntry* pEntry = m_pFactoryTable->GetFirst(hsh); pEntry != NULL; pEntry = m_pFactoryTable->GetNext(hsh))
	{
		strcpy(pNameBuffer, pSubStoreName);
		if (pNameBuffer[nLen - 1] == '\\')
			pNameBuffer[nLen - 1] = '\0';

		strcat(pNameBuffer, pEntry->m_pExt);
		if (pParentStore->StreamExists(pNameBuffer))
			return TRUE;
	}

	return FALSE;
}

///////////////////////////////////////
