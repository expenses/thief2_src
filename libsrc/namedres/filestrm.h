#pragma once

#include <storeapi.h>
#include <filespec.h>
#include <cstdio>

struct cFileStream : IStoreStream
{
	class cRefCount
	{
	public:
		cRefCount() : ul(1) {}
		ULONG AddRef() {return ++ul;}
		ULONG Release() {return --ul;}
		operator ULONG() {return ul;}
	private:
		ULONG ul;
	};

	cRefCount __m_ulRefs;
	IStore* m_pStorage;
	FILE* m_pFile;
	cFileSpec* m_pFileSpec;
	long m_nLastPos;
	int m_nOpenCount;

	cFileStream(IStore* pStore);
	virtual ~cFileStream();

	// IUnknown
	STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();
	void OnFinalRelease() { delete this; }

	void STDMETHODCALLTYPE SetName(const char* pName);
	BOOL STDMETHODCALLTYPE Open();
	void STDMETHODCALLTYPE Close();
	void STDMETHODCALLTYPE GetName(char** ppName);
	BOOL STDMETHODCALLTYPE SetPos(long nPos);
	long STDMETHODCALLTYPE GetPos();
	long STDMETHODCALLTYPE ReadAbs(long nStartPos, long nEndPos, char* pBuf);
	long STDMETHODCALLTYPE GetSize();
	long STDMETHODCALLTYPE Read(long nNumBytes, char* pBuf);
	int16 STDMETHODCALLTYPE Getc();
	void STDMETHODCALLTYPE ReadBlocks(void* pBuf, long nSize, tStoreStreamBlockCallback callback, void* pData);
	ulong STDMETHODCALLTYPE LastModified();
};