#include "storedir.h"

#include <pathutil.h>
#include <filespec.h>
#include <filepath.h>
#include <filestrm.h>
#include <hshsttem.h>

///////////////////////////////////////

struct sDirState
{
	BOOL bStart;
	uint fFlags;
	char* pPattern;
	sFindContext FC;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDirectoryStorage
//

cDirectoryStorage::cDirectoryStorage(const char* pRawName)
	:m_pFilePath(NULL),
	m_pName(NULL),
	m_pParent(NULL),
	m_pSubstorageTable(NULL),
	m_pStreamTable(NULL),
	m_Exists(kExistenceUnchecked),
	m_pStoreManager(NULL),
	m_fFlags(0)
{
	if (pRawName)
	{
		char* pName;
		GetNormalizedPath(pRawName, &pName);
		m_pName = (char*)operator new(strlen(pName) + 1);
		strcpy(m_pName, pName);
		operator delete(pName);
	}
	m_pSubstorageTable = new cStorageHashByName();
	m_pStreamTable = new cStreamHashByName();
}

///////////////////////////////////////

cDirectoryStorage::~cDirectoryStorage()
{
	if (m_pName)
	{
		operator delete(m_pName);
		m_pName = NULL;
	}

	if (m_pFilePath)
	{
		delete m_pFilePath;
		m_pFilePath = NULL;
	}

	if (m_pParent)
	{
		m_pParent->Release();
		m_pParent = NULL;
	}

	if (m_pSubstorageTable)
	{
		delete m_pSubstorageTable;
		m_pSubstorageTable = NULL;
	}

	if (m_pStreamTable)
	{
		tHashSetHandle h;
		for (cNamedStream* pEntry = m_pStreamTable->GetFirst(h); pEntry != NULL; pEntry = m_pStreamTable->GetNext(h))
		{
			m_pStreamTable->Remove(pEntry);
			delete pEntry;
		}

		delete m_pStreamTable;
		m_pStreamTable = NULL;
	}

	if (m_pStoreManager)
	{
		m_pStoreManager->Release();
		m_pStoreManager = NULL;
	}
}

///////////////////////////////////////

const char* cDirectoryStorage::GetName()
{
	return m_pName;
}

///////////////////////////////////////

const char* cDirectoryStorage::GetFullPathName()
{
	if (m_pFilePath)
		return m_pFilePath->GetPathName();

	return NULL;
}

///////////////////////////////////////

void cDirectoryStorage::EnumerateLevelHelper(cFilePath* pPath, tStoreLevelEnumCallback callback, BOOL bAbsolute, BOOL bRecurse, void* pClientData)
{
	sFindContext FC;
	cFilePath foundPath;
	cStr relativePath;
	const char* pName = NULL;

	for (BOOL bContinue = pPath->FindFirst(foundPath, FC); bContinue; bContinue = pPath->FindNext(foundPath, FC))
	{
		if (bAbsolute)
		{
			foundPath.MakeFullPath();
			pName = foundPath.GetPathName();
		}
		else
		{
			if (m_pFilePath->ComputeAnchoredPath(foundPath, relativePath))
			{
				pName = relativePath;
			}
			else
			{
				foundPath.MakeFullPath();
				pName = foundPath.GetPathName();
			}
		}

		if (callback(this, pName, pClientData) && bRecurse)
			EnumerateLevelHelper(&foundPath, callback, bAbsolute, bRecurse, pClientData);
	}

	pPath->FindDone(FC);
}

///////////////////////////////////////

void cDirectoryStorage::EnumerateLevel(tStoreLevelEnumCallback callback, BOOL bAbsolute, BOOL bRecurse, void* pClientData)
{
	if (m_pFilePath)
		EnumerateLevelHelper(m_pFilePath, callback, bAbsolute, bRecurse, pClientData);
}

///////////////////////////////////////

void cDirectoryStorage::EnumerateStreamPath(cFilePath* pPath, tStoreLevelEnumCallback callback, BOOL bAbsolute, void* pClientData)
{
	sFindContext FC;
	cFileSpec foundFile;
	cStr name;

	for (BOOL bContinue = pPath->FindFirst(foundFile, FC); bContinue; bContinue = pPath->FindNext(foundFile, FC))
	{
		if (bAbsolute)
		{
			foundFile.MakeFullPath();
			foundFile.GetNameString(name, kFullPathNameStyle);
		}
		else
		{
			if (foundFile.MakeAnchoredPath(*m_pFilePath))
				foundFile.GetNameString(name, kAnchorRelativeNameStyle);
			else
				foundFile.GetNameString(name, kFullPathNameStyle);
		}

		callback(this, name, pClientData);
	}

	pPath->FindDone(FC);
}

///////////////////////////////////////

void cDirectoryStorage::EnumerateStreamHelper(cFilePath* pPath, tStoreLevelEnumCallback callback, BOOL bAbsolute, BOOL bRecurse, void* pClientData)
{
	sFindContext FC;
	cFilePath foundPath;
	cStr name;

	EnumerateStreamPath(pPath, callback, bAbsolute, pClientData);
	if (bRecurse)
	{
		for (BOOL bContinue = pPath->FindFirst(foundPath, FC); bContinue; bContinue = pPath->FindNext(foundPath, FC))
			EnumerateStreamHelper(&foundPath, callback, bAbsolute, bRecurse, pClientData);

		pPath->FindDone(FC);
	}
}

///////////////////////////////////////

void cDirectoryStorage::EnumerateStreams(tStoreLevelEnumCallback callback, BOOL bAbsolute, BOOL bRecurse, void* pClientData)
{
	if (m_pFilePath)
		EnumerateStreamHelper(m_pFilePath, callback, bAbsolute, bRecurse, pClientData);
}

///////////////////////////////////////

IStore* cDirectoryStorage::GetSubstorage(const char* pSubPath, BOOL bCreate)
{
	if (!pSubPath || !strlen(pSubPath))
		return NULL;

	char topPathLevel[32];
	GetPathTop(pSubPath, topPathLevel, &pSubPath);

	IStore* pSubstorage = NULL;

	cNamedStorage* pSubEntry = m_pSubstorageTable->Search(topPathLevel);
	if (pSubEntry)
	{
		pSubstorage = pSubEntry->m_pStore;
		if (!pSubstorage)
			return NULL;
		pSubstorage->AddRef();
	}
	else if (!strcmp(topPathLevel, "..\\") && m_pParent)
	{
		pSubstorage = m_pParent;
		pSubstorage->AddRef();
	}
	else if (!strcmp(topPathLevel, ".\\") && m_pParent)
	{
		pSubstorage = this;
		AddRef();
	}
	else
	{
		cFilePath AnchoredSubpath;
		bool isDriveLetter = false;
		if (m_pFilePath)
		{
			cFilePath SubstoragePath(topPathLevel);
			AnchoredSubpath = *m_pFilePath;
			if (!AnchoredSubpath.AddRelativePath(SubstoragePath))
				goto fail;
		}
		else if (topPathLevel[1] == ':')
			isDriveLetter = true;
		else
			AnchoredSubpath.FromText(topPathLevel);

		if (isDriveLetter || AnchoredSubpath.PathExists())
		{
			cDirectoryStorage* pNewStore = new cDirectoryStorage(topPathLevel);
			pNewStore->SetParent(this);
			pNewStore->SetStoreManager(m_pStoreManager);

			pSubstorage = pNewStore;
			pSubEntry = new cNamedStorage(pSubstorage);

			m_pSubstorageTable->Insert(pSubEntry);
		}
		else
		{
			char pNameBuffer[32];
			if (m_pStoreManager->HeteroStoreExists(this, topPathLevel, pNameBuffer))
			{
				pSubstorage = m_pStoreManager->CreateSubstore(this, pNameBuffer);
				if (pSubstorage)
				{
					RegisterSubstorage(pSubstorage, topPathLevel);
				}
				else
				{
					pSubEntry = new cNamedStorage(topPathLevel);
					m_pSubstorageTable->Insert(pSubEntry);

					return NULL;
				}
			}
			else
			{
				if (bCreate)
				{
					cDirectoryStorage* dirStorage = new cDirectoryStorage(topPathLevel);
					dirStorage->SetParent(this);
					pSubstorage = dirStorage;

					pSubEntry = new cNamedStorage(pSubstorage);
					m_pSubstorageTable->Insert(pSubEntry);
				}
				else
				{
					pSubEntry = new cNamedStorage(topPathLevel);
					m_pSubstorageTable->Insert(pSubEntry);
fail:
					return NULL;
				}
			}
		}
	}

	if (!strlen(pSubPath))
		return pSubstorage;

	IStore* result = pSubstorage->GetSubstorage(pSubPath, bCreate);
	pSubstorage->Release();
	return result;
}

///////////////////////////////////////

IStore* cDirectoryStorage::GetParent()
{
	if (m_pParent)
		m_pParent->AddRef();

	return m_pParent;
}

///////////////////////////////////////

void cDirectoryStorage::Refresh(BOOL bRecurse)
{
	tHashSetHandle hs;

	for (cNamedStream* pStream = m_pStreamTable->GetFirst(hs); pStream != NULL; pStream = m_pStreamTable->GetNext(hs))
	{
		m_pStreamTable->Remove(pStream);
		if (pStream)
			delete pStream;
	}

	for (cNamedStorage* pSubstorage = m_pSubstorageTable->GetFirst(hs); pSubstorage != NULL; pSubstorage = m_pSubstorageTable->GetNext(hs))
	{
		if (bRecurse && pSubstorage->m_pStore)
			pSubstorage->m_pStore->Refresh(bRecurse);

		m_pSubstorageTable->Remove(pSubstorage);
		if (pSubstorage)
			delete pSubstorage;
	}

	m_Exists = kExistenceUnchecked;
}

///////////////////////////////////////

BOOL cDirectoryStorage::Exists()
{
	if (m_Exists == kExistenceUnchecked)
	{
		if (!m_pFilePath)
			return TRUE;

		if (m_pParent->Exists())
		{
			if (m_pFilePath->PathExists())
				m_Exists = kExists;
			else
				m_Exists = m_pName[1] == ':' ? kExists : kDoesntExist;
		}
		else
		{
			m_Exists = kDoesntExist;
		}
	}

	return m_Exists == kExists;
}

///////////////////////////////////////

BOOL cDirectoryStorage::StreamExists(const char* pName)
{
	if (!m_pFilePath || !pName)
		return FALSE;

	cNamedStream* pEntry = m_pStreamTable->Search(pName);
	if (pEntry)
		return pEntry->m_bExists;

	BOOL bExists = FALSE;
	if (m_pFilePath)
	{
		cFileSpec fileSpec(*m_pFilePath, pName );
		bExists = fileSpec.FileExists();
	}
	else
	{
		cFileSpec fileSpec(pName);
		bExists = fileSpec.FileExists();
	}

	pEntry = new cNamedStream(pName, bExists );
	m_pStreamTable->Insert(pEntry);

	return bExists;
}

///////////////////////////////////////

IStoreStream* cDirectoryStorage::OpenStream(const char* pName, uint)
{
	if (!pName)
		return NULL;

	if (!StreamExists(pName))
		return NULL;

	cFileStream* fileStream = new cFileStream(this);
	fileStream->SetName(pName);
	if (!fileStream->Open())
	{ fileStream->Release(); return NULL; }

	return fileStream;

}

///////////////////////////////////////

void cDirectoryStorage::GetCanonPath(char** ppCanonPath)
{
	if (m_pParent && !(m_fFlags & STORAGE_CONTEXT_ROOT))
	{
		char* pCanonPath;
		m_pParent->GetCanonPath(&pCanonPath);
		const char* pName = GetName();

		*ppCanonPath = static_cast<char*>(Realloc(pCanonPath, strlen(pCanonPath) + strlen(pName) + 1));
		strcat(*ppCanonPath, pName);
	}
	else
	{
		*ppCanonPath = static_cast<char*>(Malloc(1));
		**ppCanonPath = '\0';
	}
}

void* cDirectoryStorage::BeginContents(const char* pPattern, uint fFlags)
{
	sDirState* pState = new sDirState();
	pState->bStart = TRUE;
	pState->fFlags = fFlags;

	if (pPattern)
	{
		pState->pPattern = static_cast<char*>(operator new(strlen(pPattern) + 1));
		strcpy(pState->pPattern, pPattern);
	} else pState->pPattern = NULL;

	return pState;
}

///////////////////////////////////////

int cDirectoryStorage::Next(void* pCookie, char* foundName)
{
	sDirState* pState = ((sDirState*)(pCookie));
	BOOL bContinue = FALSE;

	cFilePath FoundPath;
	cFileSpec FoundFile;

	if (pState->bStart)
	{
		if (pState->fFlags & SEARCH_STORAGES)
			bContinue = m_pFilePath->FindFirst(FoundPath, pState->FC);
		else
			bContinue = m_pFilePath->FindFirst(FoundFile, pState->FC, pState->pPattern);

		pState->bStart = FALSE;
	}
	else
	{
		if (pState->fFlags & SEARCH_STORAGES)
			bContinue = m_pFilePath->FindNext(FoundPath, pState->FC);
		else
			bContinue = m_pFilePath->FindNext(FoundFile, pState->FC);
	}

	if (bContinue)
	{
		const char* pName = NULL;
		if (pState->fFlags & SEARCH_STORAGES)
			pName = FoundPath.GetPathName();
		else
			pName = FoundFile.GetFileName();

		strcpy(foundName, pName);
	}

	return bContinue;
}

///////////////////////////////////////

void cDirectoryStorage::EndContents(void* pCookie)
{
	sDirState* pState = ((sDirState*)(pCookie));
	if (pState->pPattern)
		operator delete(pState->pPattern);

	delete pState;
}

///////////////////////////////////////

void cDirectoryStorage::RegisterSubstorage(IStore* pSubstore, const char* pName)
{
	cNamedStorage* pOldSubstore = m_pSubstorageTable->Search(pName);
	if (pOldSubstore)
	{
		if (pSubstore == pOldSubstore->m_pStore)
			return;

		m_pSubstorageTable->Remove(pOldSubstore);
		delete pOldSubstore;
	}

	cNamedStorage* pStoreEntry = new cNamedStorage(pSubstore);
	m_pSubstorageTable->Insert(pStoreEntry);

	IStoreHierarchy* pSubstoreHier;
	if (SUCCEEDED(pSubstore->QueryInterface(IID_IStoreHierarchy, ((void**)(&pSubstoreHier)))))
	{
		pSubstoreHier->SetParent(this);
		pSubstoreHier->Release();
	}
}

///////////////////////////////////////

void cDirectoryStorage::SetParent(IStore* pNewParent)
{
	if (m_pParent)
		m_pParent->Release();

	m_pParent = pNewParent;
	if (!m_pParent)
		return;

	m_pParent->AddRef();

	if (!m_pName)
		return;

	const char* pParentName = m_pParent->GetFullPathName();
	cFilePath localPath(m_pName);

	if (pParentName)
	{
		m_pFilePath = new cFilePath(pParentName);
		m_pFilePath->AddRelativePath(localPath);
	}
	else
	{
		m_pFilePath = new cFilePath(localPath);
	}

	m_pFilePath->MakeFullPath();
}

///////////////////////////////////////

void cDirectoryStorage::SetStoreManager(IStoreManager* pMan)
{
	if (m_pStoreManager)
		m_pStoreManager->Release();

	m_pStoreManager = pMan;
	if (m_pStoreManager)
		m_pStoreManager->AddRef();
}

///////////////////////////////////////

void cDirectoryStorage::SetDataStream(IStoreStream*)
{
#line 776
	CriticalMsg("cDirectoryStorage::SetDataStream called!");
}

///////////////////////////////////////

void cDirectoryStorage::DeclareContextRoot(int bIsRoot)
{
	if (bIsRoot)
		m_fFlags |= STORAGE_CONTEXT_ROOT;
	else
		m_fFlags &= ~STORAGE_CONTEXT_ROOT;
}

///////////////////////////////////////

void cDirectoryStorage::Close()
{
	if (!m_pSubstorageTable)
		return;

	tHashSetHandle h;
	for (cNamedStorage* pEntry = m_pSubstorageTable->GetFirst(h); pEntry != NULL; pEntry = m_pSubstorageTable->GetNext(h))
	{
		m_pSubstorageTable->Remove(pEntry);

		IStoreHierarchy* pHier;
		if (pEntry->m_pStore && SUCCEEDED(pEntry->m_pStore->QueryInterface(IID_IStoreHierarchy, ((void**)(&pHier)))))
		{
			pHier->Close();
			pHier->Release();
		}
		if (pEntry)
			delete pEntry;
	}
}
