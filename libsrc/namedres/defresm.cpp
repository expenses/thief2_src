#include "defresm.h"

// COM includes
#include <malloc.h>
#include <comtools.h>
#include <resguid.h>

inline unsigned int cDefResMem::cRefCount::AddRef()
{
	return ++ul;
}

inline unsigned int cDefResMem::cRefCount::Release()
{
	return --ul;
}

inline cDefResMem::cRefCount::operator unsigned long()
{
	return ul;
}

STDMETHODIMP cDefResMem::QueryInterface(REFIID riid, void **ppvObj)
{
	if (!IsEqualOrIUnknownGUID(riid, IID_IResMemOverride))
	{
        *ppvObj = NULL;
        return E_NOINTERFACE;
	}
	*ppvObj = static_cast<IResMemOverride*>(this);
	AddRef();
	return S_OK;
}

STDMETHODIMP_(ULONG) cDefResMem::AddRef()
{
	return __m_ulRefs.AddRef();
}

STDMETHODIMP_(ULONG) cDefResMem::Release()
{
	if (__m_ulRefs.Release())
		return __m_ulRefs;

	OnFinalRelease();
	return 0;
}

void cDefResMem::OnFinalRelease()
{
}

void* cDefResMem::ResMalloc(ulong nNumBytes)
{
	return malloc(nNumBytes);
}

void cDefResMem::ResFree(void* pData)
{
	if (pData)
		free(pData);
}

unsigned long cDefResMem::GetSize(void* pData)
{
	return pData ? _msize(pData) : 0;
}
