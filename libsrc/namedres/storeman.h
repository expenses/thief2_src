#pragma once

#include <comtools.h>
#include <storeapi.h>
#include <resguid.h>
#include <hashset.h>

class cFactoryHashByExt;

class cFactoryEntry
{
public:
	friend class cStorageManager;
	friend class cFactoryHashByExt;

	cFactoryEntry(IStoreFactory* pFactory, const char* pExt);
	~cFactoryEntry();

private:
	IStoreFactory* m_pFactory;
	char* m_pExt;
};

template cStrHashSet<cFactoryEntry*>;

class cStorageManager : public IStoreManager
{
public:
	DECLARE_UNAGGREGATABLE();

	cStorageManager();
	virtual ~cStorageManager();

	void STDMETHODCALLTYPE RegisterFactory(IStoreFactory* pFactory);
	IStore* STDMETHODCALLTYPE GetStore(const char* pPathName, int bCreate);
	IStore* STDMETHODCALLTYPE CreateSubstore(IStore* pParent, const char* pName);
	void STDMETHODCALLTYPE SetGlobalContext(ISearchPath* pPath);
	void STDMETHODCALLTYPE SetDefaultVariants(ISearchPath* pPath);
	ISearchPath* STDMETHODCALLTYPE NewSearchPath(const char* pNewPath);
	void STDMETHODCALLTYPE Close();
	int STDMETHODCALLTYPE HeteroStoreExists(IStore* pParentStore, const char* pSubStoreName, char* pNameBuffer);

	void InstallStorageType(const char* pExt, IStoreFactory* pFactory);

	friend void ResRefreshStorage();
	friend class cSearchPath;
private:
	IStore* m_pRootStore;
	IStoreFactory* m_pDefStoreFactory;
	ISearchPath* m_pGlobalContext;
	ISearchPath* m_pDefVariants;
	cFactoryHashByExt* m_pFactoryTable;
};
