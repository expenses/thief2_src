#pragma once

#include <resapi.h>
#include <resguid.h>

class cDefResMem : public IResMemOverride
{
public:
	struct cRefCount
	{
		unsigned int ul;
		cRefCount() : ul(1) {}
		unsigned int AddRef();
		unsigned int Release();
		operator unsigned long();
	};

	cRefCount __m_ulRefs;

	STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();
	void OnFinalRelease();

	void* STDMETHODCALLTYPE ResMalloc(ulong nNumBytes);
	void STDMETHODCALLTYPE ResFree(void* pData);
	unsigned long STDMETHODCALLTYPE GetSize(void* pData);
};