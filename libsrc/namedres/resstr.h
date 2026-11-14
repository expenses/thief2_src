#pragma once

#include <resbase.h>
#include <resistr.h>
#include <hashset.h>

class cStringResIndexHash;

struct sStringTable
{
	cStringResIndexHash* LookupTable;
	char* pStringData;
};

class cStringResEntry
{
public:
	friend class cStringResource;
	friend class cStringResIndexHash;

	cStringResEntry(const char* pName, int nIndex);
	~cStringResEntry();

private:
	char* m_pName;
	int m_nIndex;
};

template cStrIHashSet<cStringResEntry*>;

class cStringResIndexHash : public cStrIHashSet<cStringResEntry*>
{
public:
	cStringResIndexHash();
	~cStringResIndexHash();

	tHashSetKey GetKey(tHashSetNode p) const
	{
		return reinterpret_cast<tHashSetKey>(((cStringResEntry*)(p))->m_pName);
	}
};

class cStringResource : public cResourceBase<IStringRes, &IID_IStringRes>
{
public:
	cStringResource(IStore* pStore,
		const char* pName,
		IResType* pType);

	void* STDMETHODCALLTYPE LoadData(ulong* pSize,
		ulong* pTimestamp,
		IResMemOverride* pResMem);

	BOOL STDMETHODCALLTYPE FreeData(void* pData,
		ulong nSize,
		IResMemOverride* pResMem);

	STDMETHOD_(void, StringPreload)(const char*);
	STDMETHOD_(char*, StringLock)(const char* pStrName);
	STDMETHOD_(void, StringUnlock)(const char* pStrName);
	STDMETHOD_(BOOL, StringExtract)(const char* pStrName, char* pBuf, int nSize);

private:
	int SkipLine(IStoreStream* pStream);
	int SkipWhitespace(IStoreStream* pStream);
	int GetStrName(IStoreStream* pStream, char* pBuf);
};