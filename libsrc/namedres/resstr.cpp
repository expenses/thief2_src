#include <windows.h>
#include <resstr.h>
#include <resbastm.h>
#include <resistr.h>
#include <resguid.h>
#include <growblk.h>
#include <allocapi.h>
#include <hashset.h>

cStringResEntry::cStringResEntry(const char* pName, int nIndex)
	: m_pName(NULL), m_nIndex(nIndex)
{
	if (!pName)
	{
		Warning(("cStringResEntry created with no name!\n"));
		return;
	}

	m_pName = static_cast<char*>(malloc(strlen(pName) + 1));
	strcpy(m_pName, pName);
}

cStringResEntry::~cStringResEntry()
{
	if (m_pName)
	{
		free(m_pName);
		m_pName = NULL;
	}
}

cStringResIndexHash::cStringResIndexHash()
{
}

cStringResIndexHash::~cStringResIndexHash()
{
	DestroyAll();
}

int cStringResource::SkipLine(IStoreStream* pStream)
{
	short c = pStream->Getc();
	if (c == -1)
		return false;
	while ((char)c != '\n')
	{
		c = pStream->Getc();
		if (c == -1)
			return false;
	}
	return true;
}

int cStringResource::SkipWhitespace(IStoreStream* pStream)
{
	short c = pStream->Getc();
	if (c == -1)
		return false;

	for (;;)
	{
		char ch = (char)c;
		if (!(ch == ' ' || ch == 0x9 || ch == '\n' || ch == '\r') && ch != '/')
		{
			pStream->SetPos(pStream->GetPos() - 1);
			return true;
		}
		if (ch == '/')
		{
			c = pStream->Getc();
			if (c == -1)
				return false;

			if ((char)c == '/')
			{
				do
				{
					c = pStream->Getc();
					if (c == -1)
						return false;
				} while ((char)c != '\n');
			}
			else if ((char)c != '*')
			{
				pStream->SetPos(pStream->GetPos() - 2);
				return true;
			}
			else
			{
				do
				{
					c = pStream->Getc();
					if (c == -1)
						return false;
					while ((char)c != '*')
					{
						c = pStream->Getc();
						if (c == -1)
							return false;
					}
					c = pStream->Getc();
					if (c == -1)
						return false;
				} while ((char)c != '/');
			}
		}

		c = pStream->Getc();
		if (c == -1)
			return false;
	}
}

int cStringResource::GetStrName(IStoreStream* pStream, char* pBuf)
{
	int idx;
	for (;;)
	{
		idx = 0;
		if (!SkipWhitespace(pStream))
			return false;
		short c = pStream->Getc();
		if (c == -1)
			return false;

		while (isalnum((char)c) || (char)c == '_')
		{
			if (idx >= 64)
			{
				Warning(("cStringRes: name too long in %s, near char %ld\n", GetName(), pStream->GetPos()));
				if (SkipLine(pStream))
					goto next;
				return false;
			}

			pBuf[idx++] = (char)c;
			c = pStream->Getc();
			if (c == -1)
				return false;
		}

		pStream->SetPos(pStream->GetPos() - 1);

		if (idx)
		{
			pBuf[idx] = '\0';
			if (!SkipWhitespace(pStream))
				return false;

			c = pStream->Getc();
			if (c == -1)
				return false;

			if ((char)c == ':')
				return true;

			Warning(("cStringRes: GetStrName: expected colon in %s, near char %ld\n", GetName(), pStream->GetPos()));
			if (SkipLine(pStream))
				continue;
			return false;
		}
		else
		{
			Warning(("cStringRes: GetStrName: expected name in %s, near char %ld\n", GetName(), pStream->GetPos()));
			if (SkipLine(pStream))
				continue;
			return false;
		}
next:	;
	}
}

cStringResource::cStringResource(IStore* pStore, const char* pName, IResType* pType)
	: cResourceBase<IStringRes, &IID_IStringRes>(pStore, pName, pType)
{
}

void* cStringResource::LoadData(ulong* pSize, ulong* pTimestamp, IResMemOverride* pResMem)
{
	cAutoResThreadLock lock;

	cGrowingBlock block;

	if (!pResMem)
		return NULL;

	IStoreStream* pStream = OpenStream();
#line 307
	AssertMsg1(pStream, "Unable to open stream: %s", GetName());
	if (!pStream)
		return NULL;

	LGALLOC_PUSH_CREDIT();
	sStringTable* pTable = ((sStringTable*)(pResMem->ResMalloc(sizeof(sStringTable))));
	LGALLOC_POP_CREDIT();
	pTable->LookupTable = new cStringResIndexHash;

	BOOL finished = FALSE;
	char name[64];
parseloop:
	while (!finished)
	{
		if (!GetStrName(pStream, name))
		{
			finished = TRUE;
			continue;
		}

		if (!SkipWhitespace(pStream))
		{
			finished = TRUE;
			continue;
		}

		short ci = pStream->Getc();
		if (ci == -1)
		{
			finished = TRUE;
			goto parseloop;
		}

		char c = (char)ci;
		if (c != '"')
		{
			Warning(("cStringRes: expected quote in %s, near char %ld\n", GetName(), pStream->GetPos()));
			if (!SkipLine(pStream))
				finished = TRUE;
			continue;
		}

		ci = pStream->Getc();
		if (ci == -1)
		{
			finished = TRUE;
			goto parseloop;
		}
		c = (char)ci;

		long stringStart = block.GetSize();
		while (c != '"')
		{
			if (c == '\\')
			{
				ci = pStream->Getc();
				if (ci == -1)
				{
					finished = TRUE;
					goto parseloop;
				}
				c = (char)ci;

				switch (c)
				{
				case '\n':
					break;
				case '\r':
					ci = pStream->Getc();
					if (ci == -1)
					{
						finished = TRUE;
						goto parseloop;
					}
					c = (char)ci;
					break;
				case 't':
					block.Append('\t');
					break;
				case 'n':
					block.Append('\n');
					break;
				default:
					block.Append(c);
					break;
				}
			}
			else
				block.Append(c);

			ci = pStream->Getc();
			if (ci == -1)
			{
				finished = TRUE;
				goto parseloop;
			}
			c = (char)ci;
		}

		block.Append('\0');
		pTable->LookupTable->Insert(new cStringResEntry(name, stringStart));
	}

	LGALLOC_PUSH_CREDIT();
	pTable->pStringData = static_cast<char*>(pResMem->ResMalloc(block.GetSize()));
	LGALLOC_POP_CREDIT();
	memcpy(pTable->pStringData, block.GetBlock(), block.GetSize());

	if (pTimestamp)
		*pTimestamp = pStream->LastModified();

	pStream->Close();
	pStream->Release();

	if (pSize)
		*pSize = block.GetSize();

	return pTable;
}

BOOL cStringResource::FreeData(void* pUntypedData, ulong nSize, IResMemOverride* pResMem)
{
	cAutoResThreadLock lock;

	sStringTable* pData = ((sStringTable*)(pUntypedData));

	pResMem->ResFree(pData->pStringData);
	if (pData->LookupTable)
		delete pData->LookupTable;
	delete pData;

	return TRUE;
}

STDMETHODIMP_(void) cStringResource::StringPreload(const char*)
{
	cAutoResThreadLock lock;
	PreLoad();
}

STDMETHODIMP_(char*) cStringResource::StringLock(const char* pStrName)
{
	cAutoResThreadLock lock;

	sStringTable* pTable = ((sStringTable*)(Lock()));
	if (!pTable)
		return NULL;

	cStringResEntry* pEntry = pTable->LookupTable->Search(pStrName);
	if (!pEntry)
	{
		Unlock();
		return NULL;
	}
	return &pTable->pStringData[pEntry->m_nIndex];
}

STDMETHODIMP_(void) cStringResource::StringUnlock(const char* pStrName)
{
	cAutoResThreadLock lock;

	Unlock();
}

STDMETHODIMP_(BOOL) cStringResource::StringExtract(const char* pStrName, char* pBuf, int nSize)
{
	cAutoResThreadLock lock;


	const char* pStr = StringLock(pStrName);
	if (!pStr)
	{
		*pBuf = '\0';
		return FALSE;
	}

	strncpy(pBuf, pStr, nSize - 1);
	pBuf[nSize - 1] = '\0';

	StringUnlock(pStrName);

	return TRUE;
}

// ── cResourceBase<IStringRes, IID_IStringRes> explicit instantiation ──

template class cResourceBase<IStringRes, &IID_IStringRes>;
