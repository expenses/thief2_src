#pragma once

#include <storeapi.h>
#include <storbase.h>
#include <filepath.h>

enum eExistenceState
{
	kExists,
	kDoesntExist,
	kExistenceUnchecked
};

class cDirectoryStorage : public cStorageBase
{
public:
	cDirectoryStorage(const char* pRawName);
	~cDirectoryStorage();

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
	void STDMETHODCALLTYPE SetStoreManager(IStoreManager *pMan);
	void STDMETHODCALLTYPE SetParent(IStore* pNewParent);
	void STDMETHODCALLTYPE RegisterSubstorage(IStore* pSubstore, const char* pName);
	void STDMETHODCALLTYPE SetDataStream(IStoreStream*);
	void STDMETHODCALLTYPE DeclareContextRoot(int bIsRoot);
	void STDMETHODCALLTYPE Close();

private:
	void EnumerateLevelHelper(cFilePath* pPath, tStoreLevelEnumCallback callback, BOOL bAbsolute, BOOL bRecurse, void* pClientData);
	void EnumerateStreamHelper(cFilePath* pPath, tStoreLevelEnumCallback callback, BOOL bAbsolute, BOOL bRecurse, void* pClientData);
	void EnumerateStreamPath(cFilePath* pPath, tStoreLevelEnumCallback callback, BOOL bAbsolute, void* pClientData);

private:
	cFilePath* m_pFilePath;
	char* m_pName;
	IStore* m_pParent;
	cStorageHashByName* m_pSubstorageTable;
	cStreamHashByName* m_pStreamTable;
	eExistenceState m_Exists;
	IStoreManager* m_pStoreManager;
	unsigned int m_fFlags;
};
