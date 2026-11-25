#include <storeman.h>
#include <search.h>
#include <pathutil.h>
#include <storbase.h>
#include <hshsttem.h>

static int g_iSetupPath;
static int g_nSetupDepth;

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cSearchPath, ISearchPath);

///////////////////////////////////////

struct FindInfo
{
	IStore* pStore;
	const char* pName;
	uint fFlags;
	IStore* pFound;
};

///////////////////////////////////////

struct SearchCookie
{
	PathStorageNode* pNode;
	void* pData;
	char* pPattern;
	uint fFlags;
	char* pPath;
	cStreamHashByName* pStreamTable;
};

///////////////////////////////////////

struct PathIterateData
{
	cSearchPath* pPath;
	tSearchPathIterateCallback pRealCallback;
	void* pRealClientData;
};

///////////////////////////////////////

struct SubpathEnumerateData
{
	cSearchPath* pThisPath;
	const char* pRootPath;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cSearchPath
//

cSearchPath::cSearchPath(IStoreManager* pStoreMan)
	: m_bPathParsed(false),
	m_pContext(NULL),
	m_pVariants(NULL),
	m_pPathStorages(NULL),
	m_pLastNode(NULL)
{
	m_aPathStrings = new cDynArray<sSearchPathElement>();
	m_pStoreMan = pStoreMan;
	m_pStoreMan->AddRef();
}

///////////////////////////////////////

cSearchPath::~cSearchPath()
{
	Clear();

	if (m_pStoreMan)
		m_pStoreMan->Release();
	m_pStoreMan = NULL;

	if (m_aPathStrings)
	{
		delete m_aPathStrings;
		m_aPathStrings = NULL;
	}
}

///////////////////////////////////////

void cSearchPath::ClearStorages()
{
	PathStorageNode* pNext = NULL;
	for (PathStorageNode* pStoreNode = m_pPathStorages; pStoreNode != NULL; pStoreNode = pNext)
	{
		pNext = pStoreNode->pNext;
		pStoreNode->pStore->Release();
		delete pStoreNode;
	}

	m_pLastNode = NULL;
	m_pPathStorages = NULL;
	m_bPathParsed = FALSE;
}

///////////////////////////////////////

void cSearchPath::Clear()
{
	SafeRelease(m_pContext);

	SafeRelease(m_pVariants);





	for (int i = m_aPathStrings->Size() - 1; i >= 0; --i)
		operator delete((*m_aPathStrings)[i].pPath);

	delete m_aPathStrings;
	m_aPathStrings = new cDynArray<sSearchPathElement>();
}

///////////////////////////////////////

ISearchPath* cSearchPath::Copy()
{
	int n = m_aPathStrings->Size(); cSearchPath* pSearchPath = new cSearchPath(m_pStoreMan);

	if (m_pContext)
		pSearchPath->SetContext(m_pContext);

	if (m_pVariants)
		pSearchPath->SetVariants(m_pVariants);

	for (int i = 0; i < n; ++i)
	{
		switch ((*m_aPathStrings)[i].fRecurse)
		{
		case 0:
			pSearchPath->AddPath((*m_aPathStrings)[i].pPath);
			break;
		case 1:
			pSearchPath->AddPathTrees((*m_aPathStrings)[i].pPath, 0);
			break;
		case 2:
			pSearchPath->AddPathTrees((*m_aPathStrings)[i].pPath, 1);
			break;
		}
	}

	return pSearchPath;
}

///////////////////////////////////////

static void CleanStorePath(char* pDest, const char* pSrc)
{
	char* pEnd = pDest + MAX_PATH - 1;

	if (*pSrc)
	{
		if (pSrc[1] != ':' && pSrc[0] != '\\' && pSrc[0] != '/'
		    && (pSrc[0] != '.' || (pSrc[1] && pSrc[1] != '.' && pSrc[1] != '\\' && pSrc[1] != '/')))
		{
			*pDest++ = '.';
			*pDest++ = '\\';
		}

		while (*pSrc && pDest < pEnd)
		{
			if (*pSrc == '\\' || *pSrc == '/')
			{
				++pSrc;
				*pDest++ = '\\';
				if (!*pSrc)
					break;
				while (*pSrc == '\\' || *pSrc == '/')
				{
					if (!*++pSrc)
					{
						*pDest = '\0';
						return;
					}
				}
			}
			else
				*pDest++ = *pSrc++;
		}
	}

	*pDest = '\0';
}

static int FindPathElement(cDynArray<sSearchPathElement>& paths, const sSearchPathElement& elem)
{
	if (paths.AsPointer())
	{
		for (unsigned i = 0; i < paths.Size(); i++)
			if (paths[i].fRecurse == elem.fRecurse && !stricmp(elem.pPath, paths[i].pPath))
				return i;
	}
	return -1;
}

void cSearchPath::DoAddStore(const char* pStoreName, int fRecurse)
{
	if (!pStoreName)
		return;

	for (const char* p = pStoreName; *p; ++p)
	{
		if (!iswspace((uchar)*p))
		{
			char cleanPath[MAX_PATH];
			sSearchPathElement newEntry;

			CleanStorePath(cleanPath, pStoreName);
			GetNormalizedPath(cleanPath, &newEntry.pPath);
			newEntry.fRecurse = fRecurse;

			if (FindPathElement(*m_aPathStrings, newEntry) == -1)
			{
				m_aPathStrings->Append(newEntry);
				m_bPathParsed = FALSE;
			}
			return;
		}
	}
}

///////////////////////////////////////

void cSearchPath::DoAddPath(const char* pPath, int fRecurse)
{
	if (!pPath || !*pPath)
		return;

	char buff[512];
	const char* pdelim = NULL;

	for (const char* p = pPath; *p; p = pdelim + 1)
	{
		pdelim = strpbrk(p, ";+");
		if (!pdelim)
		{
			DoAddStore(p, fRecurse);
			return;
		}

		strncpy(buff, p, pdelim - p);
		buff[pdelim - p] = '\0';
		DoAddStore(buff, fRecurse);
	}
}

///////////////////////////////////////

void cSearchPath::Refresh()
{
}

///////////////////////////////////////

void cSearchPath::AddPath(const char* pPath)
{
	DoAddPath(pPath, 0);
}

///////////////////////////////////////

void cSearchPath::AddPathTrees(const char* pPath, BOOL bRecurse)
{
	DoAddPath(pPath, (bRecurse != FALSE) + 1);
}

///////////////////////////////////////

void cSearchPath::Ready()
{
	if (!m_bPathParsed)
		SetupStorages();
}

///////////////////////////////////////

void cSearchPath::SetContext(ISearchPath* pContext)
{
	if (m_pContext) m_pContext->Release();

	if (pContext)
	{
		m_pContext = pContext;
		m_pContext->AddRef();
	} else m_pContext = NULL;

	m_bPathParsed = FALSE;
}

///////////////////////////////////////

void cSearchPath::SetVariants(ISearchPath* pVariants)
{
	if (m_pVariants)
	{
		m_pVariants->Release();

	}

	if (pVariants)
	{
		m_pVariants = pVariants;
		m_pVariants->AddRef();
	} else m_pVariants = NULL;

	m_bPathParsed = FALSE;
}

///////////////////////////////////////

int StorageSetupCallback(IStore* /*pStore*/, const char* pSubstoreName, void* pClientData)
{
	char FullPath[512];

	SubpathEnumerateData* pData = ((SubpathEnumerateData*)(pClientData));

	strcpy(FullPath, pData->pRootPath);
	strcat(FullPath, pSubstoreName);

	pData->pThisPath->SetupSingleStore(FullPath, FALSE);

	return TRUE;
}

///////////////////////////////////////

void cSearchPath::SetupSingleStore(const char* pStorePath, int fRecurse)
{
	IStore* pStore = m_pStoreMan->GetStore(pStorePath, FALSE);
	if (pStore && pStore->Exists())
	{
		PathStorageNode* pNode = ((PathStorageNode*)(operator new(sizeof(PathStorageNode))));
		pNode->pStore = pStore;
		pNode->pNext = NULL;
		pNode->iPath = g_iSetupPath;
		if (m_pPathStorages)
			m_pLastNode->pNext = pNode;
		else
			m_pPathStorages = pNode;

		m_pLastNode = pNode;

		if (fRecurse)
		{
			SubpathEnumerateData pData;
			pData.pThisPath = this;
			pData.pRootPath = pStorePath;

			pStore->EnumerateLevel(StorageSetupCallback, FALSE, fRecurse == 2, &pData);
		}
	}
}

///////////////////////////////////////

void DispatchSetupStorage(ISearchPath* /*pSearchParh*/, const char* pStore, int fRecurse, void* pClientData)
{
	cSearchPath* pData = ((cSearchPath*)(pClientData));
	pData->SetupSingleStore(pStore, fRecurse);
}

///////////////////////////////////////

void cSearchPath::SetupStorages()
{
	ClearStorages();

	if (m_aPathStrings && m_aPathStrings->Size())
	{
		++g_nSetupDepth;
		Iterate(DispatchSetupStorage, TRUE, this);
		--g_nSetupDepth;
		m_bPathParsed = true;
	}
}

///////////////////////////////////////

void FindInStorage(ISearchPath* /*pPath*/, const char* pVariant, tSearchPathRecursion /*fRecurse*/, void* pClientData)
{
	FindInfo* pFindInfo = ((FindInfo*)(pClientData));

	if (pFindInfo->pFound)
		return;

	IStore* pStore = NULL;
	if (pVariant)
	{
		pStore = pFindInfo->pStore->GetSubstorage(pVariant, FALSE);
		if (!pStore)
			return;
	}
	else
	{
		pStore = pFindInfo->pStore;
		pStore->AddRef();
	}

	if (pFindInfo->fFlags & SEARCH_STORAGES)
	{
		pFindInfo->pFound = pStore->GetSubstorage(pFindInfo->pName, FALSE);
		if (pFindInfo->pFound)
		{
			if (!pFindInfo->pFound->Exists())
			{
				pFindInfo->pFound->Release();
				pFindInfo->pFound = NULL;
			}
		}
	}
	else if (pStore->StreamExists(pFindInfo->pName))
	{
		pFindInfo->pFound = pStore;
		pFindInfo->pFound->AddRef();
	}

	pStore->Release();
}

///////////////////////////////////////

IStore* cSearchPath::Find(const char* pName, uint fFlags, IStore** ppCanonStore, const char* pRawRelPath)
{
	IStore* found = NULL, * pStore = NULL;
	if (!pName || pName[0] == '\0')
	{
		Warning(("Find: empty name!\n"));
		return NULL;
	}

	if (!m_bPathParsed)
		SetupStorages();

	char* pRelPath;
	if (pRawRelPath)
		GetNormalizedPath(pRawRelPath, &pRelPath);
	else
		pRelPath = NULL;

	for (PathStorageNode* pNode = m_pPathStorages; !found && pNode; pNode = pNode->pNext)
	{
		if (pRelPath)
		{
			pStore = pNode->pStore->GetSubstorage(pRelPath, 0);
		}
		else
		{
			pStore = pNode->pStore;
			pStore->AddRef();
		}

		if (pStore)
		{
			FindInfo info;

			info.pName = pName;
			info.pStore = pStore;
			info.fFlags = fFlags;
			info.pFound = NULL;

			if (m_pVariants)
				m_pVariants->Iterate(FindInStorage, FALSE, &info);
			else
				FindInStorage(NULL, NULL, 0, &info);

			found = info.pFound;
			if (info.pFound && ppCanonStore)
			{
				*ppCanonStore = pStore;
				pStore->AddRef();
			}

			pStore->Release();
		}
	}

	if (pRelPath)
		operator delete(pRelPath);

	return found;
}

///////////////////////////////////////

void* cSearchPath::BeginContents(const char* pPattern, uint fFlags, const char* pRelPath)
{
	if (!m_bPathParsed)
		SetupStorages();

	if (!m_pPathStorages)
		return NULL;

	SearchCookie* pCookie = ((SearchCookie*)(operator new(sizeof(SearchCookie))));

	pCookie->pNode = m_pPathStorages;

	if (pRelPath)
	{
		pCookie->pPath = new char[strlen(pRelPath) + 1];
		strcpy(pCookie->pPath, pRelPath);
	}
	else
	{
		pCookie->pPath = NULL;
	}

	pCookie->pData = NULL;

	if (pPattern)
	{
		pCookie->pPattern = new char[strlen(pPattern) + 1];
		strcpy(pCookie->pPattern, pPattern);
	}
	else
	{
		pCookie->pPattern = NULL;
	}

	pCookie->fFlags = fFlags;
	pCookie->pStreamTable = new cStreamHashByName();

	return pCookie;
}

///////////////////////////////////////

BOOL cSearchPath::Next(void* pUntypedCookie, IStore** pFoundStore, char* pFoundName, IStore** ppFoundCanonStore)
{
	SearchCookie* pCookie = ((SearchCookie*)(pUntypedCookie));
	IStore* pStore;

	while (TRUE)
	{
		if (!pCookie->pNode)
			return FALSE;

		if (pCookie->pPath)
		{
			pStore = pCookie->pNode->pStore->GetSubstorage(pCookie->pPath, FALSE);
			if (!pStore)
			{
				pCookie->pNode = pCookie->pNode->pNext;
				continue;
			}
		}
		else
		{
			pStore = pCookie->pNode->pStore;
			pStore->AddRef();
		}

		if (!pCookie->pData)
		{
			pCookie->pData = pStore->BeginContents(pCookie->pPattern, pCookie->fFlags);
			if (!pCookie->pData)
			{
				pStore->Release();
				pCookie->pNode = pCookie->pNode->pNext;
				continue;
			}
		}

		while (pStore->Next(pCookie->pData, pFoundName))
		{
			if (!pCookie->pStreamTable->Search(pFoundName))
			{
				*pFoundStore = pStore;
				if (ppFoundCanonStore)
				{
					*ppFoundCanonStore = pStore;
					pStore->AddRef();
				}
				pCookie->pStreamTable->Insert(new cNamedStream(pFoundName, 1));
				return TRUE;
			}
		}

		pStore->EndContents(pCookie->pData);
		pStore->Release();
		pCookie->pData = NULL;
		pCookie->pNode = pCookie->pNode->pNext;
	}
}

///////////////////////////////////////

void cSearchPath::EndContents(void* pUntypedCookie)
{
	SearchCookie* pCookie = ((SearchCookie*)(pUntypedCookie));

	if (!pCookie)
		return;

	if (pCookie->pPattern)
		delete pCookie->pPattern;

	if (pCookie->pPath)
		delete pCookie->pPath;

	if (pCookie->pStreamTable)
	{
		pCookie->pStreamTable->DestroyAll();
		delete pCookie->pStreamTable;
	}

	operator delete(pCookie);
}

///////////////////////////////////////

void cSearchPath::DoIterate(const char* pContext, tSearchPathIterateCallback callback, void* pClientData)
{
	BOOL bGlobal = g_nSetupDepth && this == ((cStorageManager*)m_pStoreMan)->m_pGlobalContext;
	uint nPaths = m_aPathStrings->Size();
	for (uint i = 0; i < nPaths; ++i)
	{
		if (bGlobal)
			g_iSetupPath = i + 1;
		char fullpath[512];
		strcpy(fullpath, pContext);
		strcat(fullpath, (*m_aPathStrings)[i].pPath);
		callback(this, fullpath, (*m_aPathStrings)[i].fRecurse, pClientData);
	}

	if (bGlobal)
		g_iSetupPath = 0;
}

///////////////////////////////////////

void doDispatchIterate(ISearchPath* /*pPath*/, const char* pContextStore, tSearchPathRecursion /*fRecurse*/, void* pClientData)
{
	PathIterateData* pPathIterateData = ((PathIterateData*)(pClientData));
	pPathIterateData->pPath->DoIterate(pContextStore, pPathIterateData->pRealCallback, pPathIterateData->pRealClientData);
}

///////////////////////////////////////

void cSearchPath::Iterate(tSearchPathIterateCallback callback, BOOL bUseContext, void* pClientData)
{
	PathIterateData envelope;

	if (bUseContext && m_pContext)
	{
		envelope.pPath = this;
		envelope.pRealCallback = callback;
		envelope.pRealClientData = pClientData;

		m_pContext->Iterate(doDispatchIterate, TRUE, &envelope);
	}
	else
	{
		DoIterate("", callback, pClientData);
	}
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cFoundStream
//

cFoundStream::cFoundStream(IStore* pStore, const char* pName)
	: m_pStore(NULL), m_pName(NULL)
{
	if (pStore)
	{
		m_pStore = pStore;
		pStore->AddRef();
	}

	if (!pName)
		CriticalMsg("Creating a cFoundStream without a name!");

	m_pName = static_cast<char*>(operator new(strlen(pName) + 1));
	strcpy(m_pName, pName);
}

cFoundStream::~cFoundStream()
{
	if (m_pStore)
	{
		m_pStore->Release();
		m_pStore = NULL;
	}

	if (m_pName)
	{
		operator delete(m_pName);
		m_pName = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cFoundStreamsByName
//

cFoundStreamsByName::cFoundStreamsByName()
	: cStrIHashSet<cFoundStream*>(0x65)
{
}

cFoundStreamsByName::~cFoundStreamsByName()
{
	DestroyAll();
	cStrIHashSet<cFoundStream*>::~cStrIHashSet<cFoundStream*>();
}

tHashSetKey cFoundStreamsByName::GetKey(tHashSetNode p) const
{
	return (tHashSetKey)(((cFoundStream*)p)->m_pName);
}
