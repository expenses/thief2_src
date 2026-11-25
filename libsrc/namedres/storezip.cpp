#include <storezip.h>
#include <pathutil.h>
#include <filespec.h>
#include <zipstrm.h>

///////////////////////////////////////

struct sEnumLevelEnvelope
{
	void* pRealClientData;
	tStoreLevelEnumCallback pCallback;
	cZipSubstorage* pStore;
	BOOL bAbsolute;
};

///////////////////////////////////////

struct sZipState
{
	BOOL bStart;
	uint fFlags;
	char* pPattern;
	tHashSetHandle hsh;
};

///////////////////////////////////////

struct sLocalFileHeader
{
	uint32 Signature;
	uint16 ExtractVersion;
	uint16 Flags;
	uint16 CompressionMethod;
	uint16 ModTime;
	uint16 ModDate;
	uint32 CRC32;
	uint32 CompressedSize;
	uint32 UncompressedSize;
	uint16 FilenameLen;
	uint16 ExtraLen;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cNamedZipStream
//

cNamedZipStream::cNamedZipStream(const char* pName, uint32 nCompressedSize,
	uint32 nUncompressedSize, uint32 nHeaderOffset, uint16 nCompressionMethod)
	: cNamedStream(pName, TRUE),
	m_nCompressedSize(nCompressedSize),
	m_nUncompressedSize(nUncompressedSize),
	m_nHeaderOffset(nHeaderOffset),
	m_nCompressionMethod(nCompressionMethod)
{
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cZipSubstorage
//

cZipSubstorage::cZipSubstorage(IStore* pParent, const char* pRawName)
	: m_pFilePath(NULL),
	m_pName(NULL),
	m_pParent(NULL),
	m_pSubstorageTable(NULL),
	m_pStreamTable(NULL),
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

	SetParent(pParent);
}

///////////////////////////////////////

cZipSubstorage::~cZipSubstorage()
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

const char* cZipSubstorage::GetName()
{
	return m_pName;
}

///////////////////////////////////////

const char* cZipSubstorage::GetFullPathName()
{
	if (m_pFilePath)
		return m_pFilePath->GetPathName();

	return NULL;
}

///////////////////////////////////////

int cZipSubstorage::EnumerateLevelHelper(const char* pSubpath, IStore* pSubstore, tStoreLevelEnumCallback callback, BOOL bAbsolute, void* pClientData)
{
	cFilePath topPath(pSubstore->GetName() );
	cFilePath subPath(pSubpath);
	topPath.AddRelativePath(subPath);

	const char* pName = NULL;
	if (bAbsolute)
	{
		cFilePath myPath(*m_pFilePath );
		myPath.AddRelativePath(topPath);
		myPath.MakeFullPath();

		pName = myPath.GetPathName();
	}
	else
	{
		pName = topPath.GetPathName();
	}

	return callback(this, pName, pClientData);
}

///////////////////////////////////////

int EnumerateLevelCallback(IStore* pSubstore, const char* pSubpath, void* pClientData)
{
	sEnumLevelEnvelope* envelope = ((sEnumLevelEnvelope*)(pClientData));
	cZipSubstorage* pStore = envelope->pStore;
	return pStore->EnumerateLevelHelper(pSubpath, pSubstore, envelope->pCallback, envelope->bAbsolute, envelope->pRealClientData);
}

///////////////////////////////////////

void cZipSubstorage::EnumerateLevel(tStoreLevelEnumCallback callback, BOOL bAbsolute, BOOL bRecurse, void* pClientData)
{
	tHashSetHandle hsh;

	for (cNamedStorage* pEntry = m_pSubstorageTable->GetFirst(hsh); pEntry != NULL; pEntry = m_pSubstorageTable->GetNext(hsh))
	{
		const char* pName = NULL;
		if (bAbsolute)
		{
			cFilePath fullPath(*m_pFilePath );
			cFilePath subPath(pEntry->m_pName);
			fullPath.AddRelativePath(subPath);
			fullPath.MakeFullPath();

			pName = fullPath.GetPathName();
		}
		else
		{
			pName = pEntry->m_pName;
		}

		if (callback(this, pName, pClientData) && bRecurse)
		{
			sEnumLevelEnvelope envelope;
			envelope.pRealClientData = pClientData;
			envelope.pCallback = callback;
			envelope.pStore = this;
			envelope.bAbsolute = bAbsolute;

			pEntry->m_pStore->EnumerateLevel(EnumerateLevelCallback, FALSE, TRUE, &envelope);
		}
	}
}

///////////////////////////////////////

int cZipSubstorage::EnumerateStreamsHelper(const char* pSubpath, IStore* pSubstore, tStoreLevelEnumCallback callback, BOOL bAbsolute, void* pClientData)
{
	cStr name;

	cFilePath topPath(pSubstore->GetName() );
	cFileSpec fileSpec(topPath,pSubpath );

	if (bAbsolute)
		fileSpec.GetNameString(name, *m_pFilePath);
	else
		fileSpec.GetNameString(name, kAnchorRelativeNameStyle);

	return callback(this, name, pClientData);
}

///////////////////////////////////////

int EnumerateStreamsCallback(IStore* pSubstore, const char* pSubpath, void* pClientData)
{
	sEnumLevelEnvelope* envelope = ((sEnumLevelEnvelope*)(pClientData));
	cZipSubstorage* pStore = envelope->pStore;
	return pStore->EnumerateStreamsHelper(pSubpath, pSubstore, envelope->pCallback, envelope->bAbsolute, envelope->pRealClientData);
}

///////////////////////////////////////

void cZipSubstorage::EnumerateStreams(tStoreLevelEnumCallback callback, BOOL bAbsolute, BOOL bRecurse, void* pClientData)
{
	tHashSetHandle hsh; cStr name;

	for (cNamedStream* pStreamEntry = m_pStreamTable->GetFirst(hsh); pStreamEntry != NULL; pStreamEntry = m_pStreamTable->GetNext(hsh))
	{

		const char* pName;
		if (bAbsolute)
		{
			cFileSpec fileSpec(pStreamEntry->m_pName );

			if (!fileSpec.GetFullPath(name, *m_pFilePath))
#line 324
				CriticalMsg("cZipSubstorage::EnumerateStreams -- no fullpath");

			pName = name;
		}
		else
		{
			pName = pStreamEntry->m_pName;
		}

		callback(this, pName, pClientData);
	}

	if (bRecurse)
	{
		for (cNamedStorage* pStoreEntry = m_pSubstorageTable->GetFirst(hsh); pStoreEntry != NULL; pStoreEntry = m_pSubstorageTable->GetNext(hsh))
		{
			sEnumLevelEnvelope envelope;
			envelope.pRealClientData = pClientData;
			envelope.pCallback = callback;
			envelope.pStore = this;
			envelope.bAbsolute = bAbsolute;

			pStoreEntry->m_pStore->EnumerateStreams(EnumerateStreamsCallback, FALSE, TRUE, &envelope);
		}
	}
}

///////////////////////////////////////

void cZipSubstorage::Refresh(BOOL bRecurse)
{
}

///////////////////////////////////////

BOOL cZipSubstorage::Exists()
{
	return TRUE;
}

///////////////////////////////////////

IStore* cZipSubstorage::GetSubstorage(const char* pSubPath, BOOL bCreate)
{
	if (!pSubPath || !strlen(pSubPath))
		return NULL;

	char topPathLevel[32];
	const char* pathRest;
	GetPathTop(pSubPath, topPathLevel, &pathRest);

	IStore* pSubstorage = NULL;

	cNamedStorage* pSubEntry = m_pSubstorageTable->Search(topPathLevel);
	if (pSubEntry)
	{
		pSubstorage = pSubEntry->m_pStore;
#line 393
		AssertMsg(pSubstorage, "NULL substorage in Zip file?!?");
		pSubstorage->AddRef();
	}
	else
	{

		if (strcmp(topPathLevel, "..\\") == 0)
		{
			if (!m_pParent)
				return NULL;

			pSubstorage = m_pParent;
			pSubstorage->AddRef();
		}
		else if (strcmp(topPathLevel, ".\\") == 0)
		{
			pSubstorage = this;
			AddRef();
		}
		else
		{
			char nameBuffer[32];
			if (!m_pStoreManager->HeteroStoreExists(this, topPathLevel, nameBuffer))
				return 0;

			pSubstorage = m_pStoreManager->CreateSubstore(this, nameBuffer);
			if (!pSubstorage)
				return NULL;

			pSubEntry = new cNamedStorage(pSubstorage);
			m_pSubstorageTable->Insert(pSubEntry);
		}
	}

	if (!strlen(pathRest))
		return pSubstorage;

	IStore* pResult = pSubstorage->GetSubstorage(pathRest, bCreate);
	pSubstorage->Release();
	return pResult;
}

///////////////////////////////////////

void cZipSubstorage::SetTopZip(cZipStorage* pStorage)
{
#line 457
	AssertMsg(pStorage, "cZipSubstorage::SetTopZip -- empty pStorage!");

	m_pZipStorage = pStorage;
}

///////////////////////////////////////

cZipSubstorage* cZipSubstorage::MakeSubstorage(const char* pSubPath)
{
#ifndef SHIP
	if (!pSubPath)
#line 474
		CriticalMsg("cZipStorage::MakeSubstorage -- null subpath");
	if (strlen(pSubPath) == 0)
		CriticalMsg("cZipStorage::MakeSubstorage -- empty subpath");
#endif
	char topPathLevel[32];
	const char* pathRest;
	GetPathTop(pSubPath, topPathLevel, &pathRest);

	cZipSubstorage* pSubstorage = NULL;

	cNamedStorage* pSubEntry = m_pSubstorageTable->Search(topPathLevel);
	if (pSubEntry)
	{
		pSubstorage = (cZipSubstorage*) pSubEntry->m_pStore;
	}
	else
	{
#line 964
		pSubstorage = new cZipSubstorage(this, topPathLevel );
		pSubstorage->SetTopZip(m_pZipStorage);

		pSubstorage->SetStoreManager(m_pStoreManager);

		cNamedStorage* pNamedStorage = new cNamedStorage(pSubstorage);

		pSubstorage->Release();

		m_pSubstorageTable->Insert(pNamedStorage);
	}

	if (!strlen(pathRest))
	{
		pSubstorage->AddRef();
		return pSubstorage;
	}

	return pSubstorage->MakeSubstorage(pathRest);
}

///////////////////////////////////////

IStore* cZipSubstorage::GetParent()
{
#line 519
    AssertMsg(m_pParent, "ZipSubstorage with no parent!");

	m_pParent->AddRef();
	return m_pParent;
}

///////////////////////////////////////

BOOL cZipSubstorage::StreamExists(const char* pName)
{
#line 552
	AssertMsg(pName, "cZipSubstorage::StreamExists -- null name!");

	return m_pStreamTable->Search(pName) != NULL;
}

///////////////////////////////////////

void cZipSubstorage::AddStream(const char* pName, sDirFileRecord* pDirRecord)
{
#line 567
	AssertMsg(pName,
		"cZipSubstorage::AddStream -- null name!");
	AssertMsg(m_pStreamTable->Search(pName) == NULL,
		"cZipSubstorage::AddStream -- adding duplicate stream!");

	m_pStreamTable->Insert(new cNamedZipStream(pName,
			pDirRecord->CompressedSize,
			pDirRecord->UncompressedSize,
			pDirRecord->HeaderOffset,
			pDirRecord->CompressionMethod ));
}

///////////////////////////////////////

IStoreStream* cZipSubstorage::OpenStream(const char* pName, uint)
{
#line 592
	AssertMsg(pName, "cZipSubstorage::OpenStream -- null pName!");
	cNamedStream* pStreamEntry = m_pStreamTable->Search(pName);

	if (!pStreamEntry)
		return NULL;

	IStoreStream* pStream = new cZipStream(this, m_pZipStorage, (cNamedZipStream*)(pStreamEntry)); // TODO
	pStream->SetName(pName);
	if (!pStream->Open())
	{ pStream->Release(); return NULL; }

	return pStream;
}

///////////////////////////////////////

void* cZipSubstorage::BeginContents(const char* pPattern, uint fFlags)
{
	sZipState* pState = ((sZipState*)(operator new(sizeof(sZipState))));
	pState->bStart = TRUE;
	pState->fFlags = fFlags;

	if (pPattern)
	{
		pState->pPattern = static_cast<char*>(operator new(strlen(pPattern) + 1));
		strcpy(pState->pPattern, pPattern);
	}
	else
	{
		pState->pPattern = NULL;
	}
	return pState;
}

///////////////////////////////////////

BOOL cZipSubstorage::Next(void* pUntypedCookie, char* foundName)
{
	const char* pName = NULL;
	sZipState* pCookie = (sZipState*)(pUntypedCookie);
	if (pCookie->bStart)
	{
		if (pCookie->fFlags & SEARCH_STORAGES)
		{
			cNamedStorage* pStoreEntry = m_pSubstorageTable->GetFirst(pCookie->hsh);
			if (pStoreEntry)
				pName = pStoreEntry->m_pName;
		}
		else
		{
			cNamedStream* pStreamEntry = m_pStreamTable->GetFirst(pCookie->hsh);
			if (pStreamEntry)
				pName = pStreamEntry->m_pName;
		}

		pCookie->bStart = FALSE;
	}
	else
	{
		if (pCookie->fFlags & SEARCH_STORAGES)
		{
			cNamedStorage* pStoreEntry = m_pSubstorageTable->GetNext(pCookie->hsh);
			if (pStoreEntry)
				pName = pStoreEntry->m_pName;
		}
		else
		{
			cNamedStream* pStreamEntry = m_pStreamTable->GetNext(pCookie->hsh);
			if (pStreamEntry)
				pName = pStreamEntry->m_pName;
		}
	}

	if (pName)
	{
		strcpy(foundName, pName);
		return TRUE;
	}

	return FALSE;
}

///////////////////////////////////////

void cZipSubstorage::EndContents(void* pUntypedCookie)
{
	sZipState* pCookie = (sZipState*)(pUntypedCookie);
	if (pCookie->pPattern)
		operator delete(pCookie->pPattern);

	operator delete(pCookie);
}

///////////////////////////////////////

void cZipSubstorage::DeclareContextRoot(int bIsRoot)
{
	if (bIsRoot)
		m_fFlags |= STORAGE_CONTEXT_ROOT;
	else
		m_fFlags &= ~STORAGE_CONTEXT_ROOT;
}

///////////////////////////////////////

void cZipSubstorage::GetCanonPath(char** ppCanonPath)
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
		*ppCanonPath = static_cast<char*>(operator new(1));
		**ppCanonPath = '\0';
	}
}

///////////////////////////////////////

void cZipSubstorage::RegisterSubstorage(IStore* pSubstore, const char* pName)
{
	cNamedStorage* pOldSubstore = m_pSubstorageTable->Search(pName);
#line 738
	AssertMsg(!pOldSubstore, "cZipSubstorage::RegisterSubstorage -- substituting...");

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

void cZipSubstorage::SetParent(IStore* pNewParent)
{
	if (m_pParent)
	{
		if (m_pParent == pNewParent)
			return;

		m_pParent->Release();
		if (m_pFilePath)
		{
			delete m_pFilePath;
			m_pFilePath = NULL;
		}
	}

	m_pParent = pNewParent;
	if (!m_pParent)
		return;

	m_pParent->AddRef();

	if (m_pName)
	{
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
}

///////////////////////////////////////

void cZipSubstorage::SetStoreManager(IStoreManager* pMan)
{
	if (m_pStoreManager)
		m_pStoreManager->Release();
	m_pStoreManager = pMan;

	m_pStoreManager = pMan;
	if (m_pStoreManager)
		m_pStoreManager->AddRef();

	if (!m_pSubstorageTable)
		return;

	tHashSetHandle h;
	for (cNamedStorage* pEntry = m_pSubstorageTable->GetFirst(h); pEntry != NULL; pEntry = m_pSubstorageTable->GetNext(h))
	{
		IStoreHierarchy* pHier;
		if (SUCCEEDED(pEntry->m_pStore->QueryInterface(IID_IStoreHierarchy, ((void**)(&pHier)))))
		{
			pHier->SetStoreManager(pMan);
			pHier->Release();
		}
	}
}

///////////////////////////////////////

void cZipSubstorage::SetDataStream(IStoreStream*)
{
#line 840
	CriticalMsg("cZipSubstorage::SetDataStream called!");
}

///////////////////////////////////////

void cZipSubstorage::Close()
{
	if (!m_pSubstorageTable)
		return;

	tHashSetHandle h;
	for (cNamedStorage* pEntry = m_pSubstorageTable->GetFirst(h); pEntry != NULL; pEntry = m_pSubstorageTable->GetNext(h))
	{
		m_pSubstorageTable->Remove(pEntry);

		IStoreHierarchy* pHier;
		if (SUCCEEDED(pEntry->m_pStore->QueryInterface(IID_IStoreHierarchy, ((void**)(&pHier)))))
		{
			pHier->Close();
			pHier->Release();
		}

		delete pEntry;
	}
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cZipStorage
//

cZipStorage::cZipStorage(IStore* pParent, IStoreStream* pZipStream, const char* pName)
	: cZipSubstorage(pParent, pName)
{
#ifndef SHIP
#line 1015
	if (!pParent)
#line 904
		CriticalMsg("Zip file created without a parent storage!");
	if (!pZipStream)
#line 905
		CriticalMsg("Zip file created without an input stream!");
	if (!pName)
#line 906
		CriticalMsg("Zip file created without a name!");
#endif
	m_pZipStream = pZipStream;
	m_pZipStream->AddRef();
	m_pZipStorage = this;

	uint ZipSize = m_pZipStream->GetSize();
	uint EndRecStart = ZipSize - 22;
#line 923
	AssertMsg(EndRecStart, "Zip file too small!");

	if (!m_pZipStream->SetPos(EndRecStart)) {
	    #line 925
	    CriticalMsg("Couldn't go to the start of zipfile end record");
	}
		
	sDirEndRecord EndRecord;

	m_pZipStream->Read(sizeof(sDirEndRecord), ((char*)(&EndRecord)));
	m_pZipStream->SetPos(EndRecord.DirOffset);

	for (int nRecordsLeft = EndRecord.NumEntries; nRecordsLeft; --nRecordsLeft)
	{
		sDirFileRecord dirRecord;
		m_pZipStream->Read(sizeof(sDirFileRecord), ((char*)(&dirRecord)));
#line 950
		AssertMsg(dirRecord.Signature == 0x02014b50, "Zip directory record with bad signature!"); // Central directory file header signature

		char* pFilename = static_cast<char*>(operator new(dirRecord.FilenameLen + 1));
		m_pZipStream->Read(dirRecord.FilenameLen, pFilename);
		pFilename[dirRecord.FilenameLen] = '\0';

		m_pZipStream->SetPos(dirRecord.ExtraLen + dirRecord.CommentLen + m_pZipStream->GetPos());

		if (dirRecord.ExternalAttributes & 0x10)
		{
			cZipSubstorage* pSubstore = MakeSubstorage(pFilename);
			pSubstore->Release();
		}
		else
		{
			char pPath[512];
			char pFileRoot[32];
			if (PathAndName(pFilename, pPath, pFileRoot))
			{
				cZipSubstorage* pSubstore = MakeSubstorage(pPath);





				AssertMsg1(pSubstore, "Couldn't create zip substorage %s!", pPath);
				pSubstore->AddStream(pFileRoot, &dirRecord);
				pSubstore->Release();
			}
			else
			{
				AddStream(pFileRoot, &dirRecord);
			}
		}


		operator delete(pFilename);
	}
}

///////////////////////////////////////

cZipStorage::~cZipStorage()
{
	m_pZipStream->Close();
	m_pZipStream->Release();
	m_pZipStream = NULL;
}

// static_assert(sizeof(sLocalFileHeader) == 30)

IStoreStream* cZipStorage::ReadyStreamAt(long nPos)
{
	sLocalFileHeader header;

	m_pZipStream->SetPos(nPos);
	m_pZipStream->Read(sizeof(sLocalFileHeader), ((char*)(&header)));
#line 1015
	AssertMsg(header.Signature == 0x04034b50, "Zip file header with bad signature!"); // Local file header signature ("PK\3\4")
	m_pZipStream->SetPos(header.FilenameLen + header.ExtraLen + m_pZipStream->GetPos());
	m_pZipStream->AddRef();

	return m_pZipStream;
}

///////////////////////////////////////

ulong cZipStorage::LastModified()
{
	return m_pZipStream->LastModified();
}
