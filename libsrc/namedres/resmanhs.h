#pragma once
#include <resapi.h>
#include <dynarray.h>
#include <hashset.h>

class cResMan;
class cResourceData;
class cResourceTypeData;
class cHashByResName;

class cResARQFulfiller;
class cHashByResourceType;

class cResourceTypeData
{
public:
	friend class cResMan;
	friend class cResARQFulfiller;
	friend class cHashByResourceType;

	cResourceTypeData(IResType* pType, cResourceData* pResData, IRes* pRes);
	~cResourceTypeData();
	const char* GetName();

private:
	void* m_pData;
	ulong m_nSize;
	IResMemOverride* m_pResMem;
	int m_nUserLockCount;
	int m_nInternalLockCount;
	IRes* m_pRes;
	IResType* m_pType;
	cResourceData* m_pResourceData;
	cResourceTypeData* m_pProxiedRes;
	int m_Freshed;
};

template cStrIHashSet<cResourceTypeData*>;

class cHashByResourceType : public cStrIHashSet<cResourceTypeData*>
{
public:
	cHashByResourceType();
	~cHashByResourceType();

	tHashSetKey GetKey(tHashSetNode p) const;
};

class cResourceData
{
public:
	friend class cResMan;
	friend class cResourceName;
	friend class cResourceTypeData;
	friend class cHashByResName;

	cResourceData(IStore* pStore, cResourceName* pNameData);
	~cResourceData();

private:
	IStore* m_pStore;
	cResourceName* m_pNameData;
	cHashByResourceType m_ResourceTypeHash;
	cResourceData* m_pNext;
};

class cResourceName
{
public:
	friend class cResMan;
	friend class cResourceTypeData;
	friend class cHashByResName;

	cResourceName(const char* pResName);
	~cResourceName();

private:
	char* m_pName;
	cResourceData* m_pFirstStream;
	cDynArray<char*>* m_ppFullNames;
	uint m_fFlags;
};

template cStrIHashSet<cResourceName*>;

class cHashByResName : public cStrIHashSet<cResourceName*>
{
public:
	friend class cResMan;

	cHashByResName();
	~cHashByResName();

	tHashSetKey GetKey(tHashSetNode p) const
	{
		if (((cResourceName*)p)->m_pName)
			return (tHashSetKey)((cResourceName*)p)->m_pName;
		else
			return NULL;
	}

	cResourceData* FindResData(const char* pName, IStore* pStore, BOOL bCreate);
};

inline const char* cResourceTypeData::GetName()
{
	return m_pResourceData->m_pNameData->m_pName;
}

class cInstalledResTypeHash;

class cResTypeData
{
public:
	friend class cResMan;
	friend class cInstalledResTypeHash;

	cResTypeData(const char* pName, IResType* pType);
	~cResTypeData();

private:
	enum { kNameSize = 20 };

	IResType** m_pFactories;
	int m_nNumFactories;
	char m_Name[kNameSize];
};

template cStrIHashSet<cResTypeData*>;

class cInstalledResTypeHash : public cStrIHashSet<cResTypeData*>
{
public:
	cInstalledResTypeHash();
	~cInstalledResTypeHash();

	tHashSetKey GetKey(tHashSetNode p) const
	{
		return (tHashSetKey)((cResTypeData*)p)->m_Name;
	}
};

class cInstalledResTypeByName;
class cNamedResType
{
public:
	friend class cResMan;
	friend class cInstalledResTypeByName;

	cNamedResType(IResType* pType);
	~cNamedResType();

private:
	IResType* m_pType;
};

template cStrIHashSet<cNamedResType*>;

class cInstalledResTypeByName : public cStrIHashSet<cNamedResType*>
{
public:
	cInstalledResTypeByName();
	~cInstalledResTypeByName();

	tHashSetKey GetKey(tHashSetNode p) const;
};
