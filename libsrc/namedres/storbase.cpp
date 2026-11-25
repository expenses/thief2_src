#line 32
#include <windows.h>
#include <storbase.h>
#include <hshsttem.h>

cNamedStorage::cNamedStorage(IStore* pStore)
	: m_pStore(NULL), m_pName(NULL)
{
	if (pStore)
	{
		m_pStore = pStore;
		pStore->AddRef();
		const char* pName = pStore->GetName();
		m_pName = (char*)operator new(strlen(pName) + 1);
		strcpy(m_pName, pName);
	}
}

cNamedStorage::cNamedStorage(const char* pName)
	: m_pStore(NULL), m_pName(NULL)
{
#line 45
	AssertMsg(pName, "cNamedStorage being created without a name!");

	m_pName = static_cast<char*>(operator new(strlen(pName) + 1));
	strcpy(m_pName, pName);
}

cNamedStorage::~cNamedStorage()
{
	if (this->m_pStore)
	{
		m_pStore->Release();
		m_pStore = NULL;
	}

	if (m_pName)
	{
		operator delete(m_pName);
		m_pName = NULL;
	}
}
cStorageHashByName::cStorageHashByName()
{
}
cStorageHashByName::~cStorageHashByName()
{
	DestroyAll();
}
tHashSetKey cStorageHashByName::GetKey(tHashSetNode p) const
{
	return (tHashSetKey)(((cNamedStorage*)p)->m_pName);
}
cNamedStream::cNamedStream(const char* pName, BOOL bExists)
	: m_pName(NULL), m_bExists(bExists)
{

	AssertMsg(pName, "cNamedStream being created without a name!");
	m_pName = static_cast<char*>(operator new(strlen(pName) + 1));
	strcpy(m_pName, pName);
}

cNamedStream::~cNamedStream()
{
	if (m_pName)
	{
		operator delete(m_pName);
		m_pName = NULL;
	}
}

cStreamHashByName::cStreamHashByName()
{
}

cStreamHashByName::~cStreamHashByName()
{
	DestroyAll();
}

tHashSetKey cStreamHashByName::GetKey(tHashSetNode p) const
{
	return (tHashSetKey)(((cNamedStream*)p)->m_pName);
}

cStorageBase::cStorageBase()
	: m_ulRefs(1)
	, m_pNDBase(NULL)
{
}

cStorageBase::~cStorageBase()
{
}

STDMETHODIMP cStorageBase::QueryInterface(REFIID id, void **ppI)
{
	if (IsEqualOrIUnknownGUID(id, IID_IStore))
	{
		*ppI = (IStore*)this;
		AddRef();
		return S_OK;
	}
	if (!IsEqualGUID(id, IID_IStoreHierarchy))
	{
		*ppI = NULL;
		return E_NOINTERFACE;
	}
	*ppI = (IStoreHierarchy*)this;
	AddRef();
	return S_OK;
}

STDMETHODIMP_(ULONG) cStorageBase::AddRef()
{
	return InterlockedIncrement(&m_ulRefs);
}

STDMETHODIMP_(ULONG) cStorageBase::Release()
{
	LONG refs = InterlockedDecrement(&m_ulRefs);
	if (refs)
		return refs;

	delete this;
	return 0;
}
