#pragma once

#include <comtools.h>
#include <dynarray.h>
#include <hashset.h>
#include <storeapi.h>
#include <resguid.h>

struct sSearchPathElement
{
	char* pPath;
	int fRecurse;
};

struct PathStorageNode
{
	IStore* pStore;
	PathStorageNode* pNext;
};

class cFoundStream
{
public:
	friend class cFoundStreamsByName;

	cFoundStream(IStore* pStore, const char* pName);
	~cFoundStream();

private:
	IStore* m_pStore;
	char* m_pName;
};

template cStrIHashSet<cFoundStream*>;

class cFoundStreamsByName : public cStrIHashSet<cFoundStream*>
{
public:
	cFoundStreamsByName();
	~cFoundStreamsByName();
	tHashSetKey GetKey(tHashSetNode p) const;
};

class cSearchPath : public ISearchPath
{
public:
	DECLARE_UNAGGREGATABLE();

	cSearchPath(IStoreManager* pStoreMan);
	virtual ~cSearchPath();

	void STDMETHODCALLTYPE Clear();
	ISearchPath* STDMETHODCALLTYPE Copy();
	void STDMETHODCALLTYPE AddPath(const char* pPath);
	void STDMETHODCALLTYPE AddPathTrees(const char* pPath, BOOL bRecurse);
	void STDMETHODCALLTYPE Ready();
	void STDMETHODCALLTYPE Refresh();
	void STDMETHODCALLTYPE SetContext(ISearchPath* pContext);
	void STDMETHODCALLTYPE SetVariants(ISearchPath* pVariants);
	IStore* STDMETHODCALLTYPE Find(const char* pName, uint fFlags, IStore** ppCanonStore, const char* pRelPath);
	void* STDMETHODCALLTYPE BeginContents(const char* pPattern, uint fFlags, const char* pRelPath);
	BOOL STDMETHODCALLTYPE Next(void* pCookie, IStore** pFoundStore, char* pFoundName, IStore** ppFoundCanonStore);
	void STDMETHODCALLTYPE EndContents(void* pCookie);
	void STDMETHODCALLTYPE Iterate(tSearchPathIterateCallback callback, BOOL bUseContext, void* pClientData);

	void DoIterate(const char* pContext, tSearchPathIterateCallback callback, void* pClientData);
	void SetupSingleStore(const char* pStorePath, int fRecurse);

private:
	void ClearStorages();
	void SetupStorages();
	void DoAddStore(const char* pStoreName, int fRecurse);
	void DoAddPath(const char* pPath, int fRecurse);

private:
	IStoreManager* m_pStoreMan;
	unsigned char m_bPathParsed;
	cDynArray<sSearchPathElement>* m_aPathStrings;
	ISearchPath* m_pContext;
	ISearchPath* m_pVariants;
	PathStorageNode* m_pPathStorages;
	PathStorageNode* m_pLastNode;
};