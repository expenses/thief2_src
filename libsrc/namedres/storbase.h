#pragma once

#include <comtools.h>
#include <storeapi.h>
#include <resguid.h>
#include <hashset.h>

#define STORAGE_CONTEXT_ROOT 0x01

class cStorageBase : public IStore, public IStoreHierarchy
{
public:
	cStorageBase();
	virtual ~cStorageBase();
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();
	STDMETHODIMP QueryInterface(REFIID id, void **ppI);

private:
	ULONG m_ulRefs;
};

class cZipSubstorage;
class cDirectoryStorage;
class cStorageHashByName;
class cNamedStorage
{
public:
	friend class cZipSubstorage;
	friend class cDirectoryStorage;
	friend class cStorageHashByName;

	cNamedStorage(const char* pName);
	cNamedStorage(IStore* pStore);
	~cNamedStorage();

private:
	IStore* m_pStore;
	char* m_pName;
};

template cStrIHashSet<cNamedStorage*>;

class cStorageHashByName : public cStrIHashSet<cNamedStorage*>
{
public:
	cStorageHashByName();
	~cStorageHashByName();
	tHashSetKey GetKey(tHashSetNode p) const;
};

class cStreamHashByName;
class cNamedStream
{
public:
	friend class cZipSubstorage;
	friend class cDirectoryStorage;
	friend class cStreamHashByName;

	cNamedStream(const char* pName, BOOL bExists);
	~cNamedStream();

private:
	char* m_pName;
	BOOL m_bExists;
};

template cStrIHashSet<cNamedStream*>;

class cStreamHashByName : public cStrIHashSet<cNamedStream*>
{
public:
	cStreamHashByName();
	~cStreamHashByName();
	tHashSetKey GetKey(tHashSetNode p) const;
};