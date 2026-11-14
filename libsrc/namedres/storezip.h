#pragma once

#include <storbase.h>
#include <filepath.h>


struct sDirEndRecord
{
	uint32 Signature;
	uint16 DiskNumber;
	uint16 DirStartDisk;
	uint16 NumEntriesOnDisk;
	uint16 NumEntries;
	uint32 DirSize;
	uint32 DirOffset;
	uint16 CommentLen;
};

// static_assert(sizeof(sDirEndRecord) == 22, "Invalid sDirEndRecord size");

struct sDirFileRecord
{
	uint32 Signature;
	uint16 StoredVersion;
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
	uint16 CommentLen;
	uint16 DiskNumberStart;
	uint16 InternalAttributes;
	uint32 ExternalAttributes;
	uint32 HeaderOffset;
};

// static_assert(sizeof(sDirFileRecord) == 46, "Invalid sDirFileRecord size");

class cZipStorage;

class cZipSubstorage : public cStorageBase
{
public:
	friend class cZipStorage;

	cZipSubstorage(IStore* pParent, const char* pRawName);
	~cZipSubstorage();

	const char* STDMETHODCALLTYPE GetName();
	const char* STDMETHODCALLTYPE GetFullPathName();

	void STDMETHODCALLTYPE EnumerateLevel(tStoreLevelEnumCallback callback, BOOL bAbsolute, BOOL bRecurse, void* pClientData);
	void STDMETHODCALLTYPE EnumerateStreams(tStoreLevelEnumCallback callback, BOOL bAbsolute, BOOL bRecurse, void* pClientData);

	IStore* STDMETHODCALLTYPE GetSubstorage(const char* pSubPath, BOOL bCreate);
	IStore* STDMETHODCALLTYPE GetParent();

	void STDMETHODCALLTYPE Refresh(BOOL bRecurse);
	BOOL STDMETHODCALLTYPE Exists();

	BOOL STDMETHODCALLTYPE StreamExists(const char* pName);
	IStoreStream* STDMETHODCALLTYPE OpenStream(const char* pName, uint);

	void* STDMETHODCALLTYPE BeginContents(const char* pPattern, uint fFlags);
	BOOL STDMETHODCALLTYPE Next(void* pCookie, char* foundName);
	void STDMETHODCALLTYPE EndContents(void* pCookie);

	void STDMETHODCALLTYPE GetCanonPath(char** ppCanonPath);
	void STDMETHODCALLTYPE SetStoreManager(IStoreManager* pMan);
	void STDMETHODCALLTYPE SetParent(IStore* pNewParent);
	void STDMETHODCALLTYPE RegisterSubstorage(IStore* pSubstore, const char* pName);
	void STDMETHODCALLTYPE SetDataStream(IStoreStream*);
	void STDMETHODCALLTYPE DeclareContextRoot(int bIsRoot);
	void STDMETHODCALLTYPE Close();

	int EnumerateLevelHelper(const char* pSubpath, IStore* pSubstore, tStoreLevelEnumCallback, BOOL bAbsolute, void* pClientData);
	int EnumerateStreamsHelper(const char* pSubpath, IStore* pSubstore, tStoreLevelEnumCallback, BOOL bAbsolute, void* pClientData);

protected:
	void SetTopZip(cZipStorage* pStorage);
	cZipSubstorage* MakeSubstorage(const char* pSubPath);
	void AddStream(const char* pName, sDirFileRecord* pDirRecord);

protected:
	cFilePath* m_pFilePath;
	char* m_pName;
	IStore* m_pParent;
	cZipStorage* m_pZipStorage;
	cStorageHashByName* m_pSubstorageTable;
	cStreamHashByName* m_pStreamTable;
	IStoreManager* m_pStoreManager;
	uint m_fFlags;
};

class cZipStorage : public cZipSubstorage
{
public:
	cZipStorage(IStore* pParent, IStoreStream* pZipStream, const char* pName);
	~cZipStorage();

	IStoreStream* ReadyStreamAt(long nPos);
	ulong LastModified();

private:
	IStoreStream* m_pZipStream;
};