#include "defresm.h"

// COM includes
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
	return Malloc(nNumBytes);
}

void cDefResMem::ResFree(void* pData)
{
	Free(pData);
}

unsigned long cDefResMem::GetSize(void* pData)
{
	return MSize(pData);
}
