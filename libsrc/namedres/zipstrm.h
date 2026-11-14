#pragma once

#include <comtools.h>
#include <storeapi.h>
#include <resguid.h>
#include <filespec.h>
#include <storezip.h>
#include <storbase.h>

class cZipStream;

class cNamedZipStream : public cNamedStream
{
public:
	friend class cZipStream;

	cNamedZipStream(const char* pName, uint32 nCompressedSize, uint32 nUncompressedSize, uint32 nHeaderOffset, uint16 nCompressionMethod);

private:
	uint32 m_nCompressedSize;
	uint32 m_nUncompressedSize;
	uint32 m_nHeaderOffset;
	uint16 m_nCompressionMethod;
};

class cZipStream : public IStoreStream
{
public:
	DECLARE_UNAGGREGATABLE();

	cZipStream(IStore* pStore, cZipStorage* pMaster, cNamedZipStream* pInfo);
	virtual ~cZipStream();

	void STDMETHODCALLTYPE SetName(const char* pName);
	BOOL STDMETHODCALLTYPE Open();
	void STDMETHODCALLTYPE Close();
	void STDMETHODCALLTYPE GetName(char** pName);
	BOOL STDMETHODCALLTYPE SetPos(long nPos);
	long STDMETHODCALLTYPE GetPos();
	long STDMETHODCALLTYPE ReadAbs(long nStartPos, long nEndPos, char* pBuf);
	long STDMETHODCALLTYPE GetSize();
	long STDMETHODCALLTYPE Read(long nNumByes, char* pBuf);
	short STDMETHODCALLTYPE Getc();
	void STDMETHODCALLTYPE ReadBlocks(void* pBuf, long nSize, tStoreStreamBlockCallback, void* pData);
	ulong STDMETHODCALLTYPE LastModified();

private:
	 IStore* m_pStorage;
	 cZipStorage* m_pMaster;
	 cNamedZipStream* m_pInfo;
	 cFileSpec* m_pFileSpec;
	 long m_nLastPos;
	 char* m_pData;
	 int m_nOpenCount;
};