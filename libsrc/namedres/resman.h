#pragma once

#include "resapi.h"

#include <reshelp.h>
#include <resstat.h>
#include <resarq.h>
#include <resmanhs.h>
#include <defresm.h>

#include <cacheapi.h>
#include <aggmemb.h>
#include <hashset.h>


class cResMan :
	public cCTDelegating<IResMan>,
	public cCTDelegating<IResStats>,
	public cCTDelegating<IResMem>,
	public cCTDelegating<IResManHelper>,
	public cCTAggregateMemberControl<kCTU_Default>
{
public:
	cResMan(IUnknown* pOuter);
	~cResMan();

	HRESULT STDMETHODCALLTYPE Init();
	HRESULT STDMETHODCALLTYPE End();

	ISearchPath* STDMETHODCALLTYPE NewSearchPath(const char* pNewPath);
	void STDMETHODCALLTYPE SetDefaultPath(ISearchPath* pPath);
	void STDMETHODCALLTYPE SetGlobalContext(ISearchPath* pPath);
	void STDMETHODCALLTYPE SetDefaultVariants(ISearchPath* pPath);

	IRes* STDMETHODCALLTYPE Bind(const char* pRelativePathname, const char* pTypeName, ISearchPath* pPath, const char* pExpRelPath, const uint fBindFlags);
	void STDMETHODCALLTYPE BindAll(const char* pPattern, const char* pTypeName, ISearchPath* pPath, void(*callback)(IRes*, IStore*, void*), void* pClientData, const char* pRelPath, const uint fBindFlags);
	IRes* STDMETHODCALLTYPE BindSpecific(const char* pName, const char* pTypeName, IStore* pStore, IStore* pCanonStore, const uint fBindFlags);
	IRes* STDMETHODCALLTYPE Retype(IRes* pOldRes, const char* pTypeName, const uint fBindFlags);
	IRes* STDMETHODCALLTYPE Lookup(const char* pRelativePathname, const char* pTypeName, const char* pCanonPath);

	IStore* STDMETHODCALLTYPE GetStore(const char* pPathName);
	BOOL STDMETHODCALLTYPE RegisterResType(IResType* pType);
	BOOL STDMETHODCALLTYPE RegisterStoreFactory(IStoreFactory* pStoreFactory);
	void STDMETHODCALLTYPE UnregisterResType(IResType* pType);
	void STDMETHODCALLTYPE MarkForRefresh(IRes* pRes);
	void STDMETHODCALLTYPE GlobalRefresh();

	void STDMETHODCALLTYPE SetMode(const eResStatMode mode, BOOL bTurnOn);
	void STDMETHODCALLTYPE Dump(const char* pFile);
	void STDMETHODCALLTYPE DumpSnapshot(const char* pFile);
	void STDMETHODCALLTYPE EnablePaging(BOOL bEnable);
	void STDMETHODCALLTYPE Compact();

	void* STDMETHODCALLTYPE LockResource(IRes* pRes);
	void* STDMETHODCALLTYPE ExtractResource(IRes* pRes, void* pBuf);
	void* STDMETHODCALLTYPE FindResource(IRes* pRes, long* pSize);
	void STDMETHODCALLTYPE UnlockResource(IRes* pData);
	uint STDMETHODCALLTYPE GetResourceLockCount(IRes* pRes);
	int STDMETHODCALLTYPE DropResource(IRes* pRes);
	long STDMETHODCALLTYPE GetResourceSize(IRes* pRes);
	void STDMETHODCALLTYPE UnregisterResource(IRes* pRes, ulong ManData);

	int STDMETHODCALLTYPE AsyncLock(IRes* pRes, int nPriority);
	int STDMETHODCALLTYPE AsyncExtract(IRes* pRes, int nPriority, void* pBuf, long bufSize);
	int STDMETHODCALLTYPE AsyncPreload(IRes* pRes);
	int STDMETHODCALLTYPE IsAsyncFulfilled(IRes* pRes);
	long STDMETHODCALLTYPE AsyncKill(IRes* pRes);
	long STDMETHODCALLTYPE GetAsyncResult(IRes* pRes, void** ppResult);

	void InstallResourceType(const char* pExt, IResType* pType);
	void RemoveResourceType(const char* pExt, IResType* pType);
	void FreeData(cResourceTypeData* pData, BOOL bTestUser);
	cResourceTypeData* GetResourceTypeData(IRes* pRes);

private:
	void CleanResources();
	IRes* DoBind(const char* pName, const char* pTypeName, ISearchPath* pPath, const char* pRelPath, uint fBindFlags);
	IRes* DoLookup(const char* pName, const char* pTypeName, const char* pRawCanonPath);

	BOOL DropResourceData(cResourceTypeData* pData);

	void CacheAdd(cResourceTypeData* pData);
	BOOL CacheRemove(cResourceTypeData* pData);

	void* DoLockResource(IRes*, cResourceTypeData* pData);
	void DoUnlockResource(cResourceTypeData* pData);

	IRes* WalkNameChain(cResourceData* pData, const char*, const char* pTypeName, const char* pCanonPath);

	void DoDumpSnapshot(FILE* fp);
	int VerifyStorage(IStore* pStorage);
	IRes* GetResource(const char* pName, const char* pTypeName, IStore* pStore);
	static void MungePaths(const char* pPathName, const char* pExpRelPath, char** ppRelPath, char** ppName, char* pOldSlash, int* pbCombinedPaths);
	static void RestorePath(char* pPath, char* pName, char OldSlash, int bCombinedPaths);
	cResourceTypeData* FindResourceTypeData(IStore* pStore, const char* pResName, IResType* pType);
	IRes* CreateResource(IStore* pStorage, const char* pName, const char* pExt, const char* pTypeName, uint);
	IResType* STDMETHODCALLTYPE GetResType(const char* pTypeName);

private:
	cResManARQ m_ResManARQ;
	IStoreManager* m_pStoreMan;
	ISearchPath* m_pDefSearchPath;
	friend void ResRefreshStorage();
	friend IStoreStream* ResFindStoreStream(const char* pName, const char* pRelPath, IStore** ppStore);
	friend IStoreStream* ResOpenConfigFile(const char* pPathName, const char* pRelPath, IStore** ppStore);
	ISearchPath* m_pGlobalContext;
	ISearchPath* m_pDefVariants;
	cInstalledResTypeHash m_ResTypeHash;
	cInstalledResTypeByName m_ResTypeByNameHash;
	cHashByResName m_ResTable;
	ISharedCache* m_pSharedCache;
	ICache* m_pCache;
	int m_bPagingEnabled;
	IResType** m_ppResFactories;
	int m_nResFactories;
	int m_FreshStamp;
#ifndef SHIP
	cResStats* m_pResStats;
#endif
	int m_fDidInit;
	cDefResMem m_DefResMem;
};

#pragma pack()
