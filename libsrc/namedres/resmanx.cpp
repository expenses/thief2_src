#include <resman.h>
#include <dbg.h>

#include <storeapi.h>
#include <resthred.h>
#include <mprintf.h>
#include <memall.h>

#ifdef SHIP
#define ResStat(x)
#else
#define ResStat(x) x
#endif

///////////////////////////////////////

cResourceTypeData* cResMan::GetResourceTypeData(IRes* pRes)
{
	IResControl* pResControl;
	if (FAILED(pRes->QueryInterface(IID_IResControl, (void**)&pResControl)))
	{
#line 46
	CriticalMsg("Resource without an IResControl!");
	return NULL;
	}
	else
	{
	ulong manData = pResControl->GetManData();
	pResControl->Release();

	if (manData == -1)
	{
		Warning(("Resource without any Manager Data!\n"));
		return NULL;
	}

	return (cResourceTypeData*)manData;
	}
}

void cResMan::InstallResourceType(const char* pExt, IResType* pType)
{
	if (pType)
	{
		if (pExt == NULL)
		{
		}
		else
		{
			cResTypeData* pData = m_ResTypeHash.Search(pExt);

			if (pData)
			{
				++pData->m_nNumFactories;
#line 498
				pData->m_pFactories = ((IResType**)(Realloc(pData->m_pFactories, sizeof(IResType*) * pData->m_nNumFactories)));
				pData->m_pFactories[pData->m_nNumFactories - 1] = pType;
				pType->AddRef();
			}
			else
			{
				pData = new cResTypeData(pExt, pType);
				m_ResTypeHash.Insert(pData);
			}
		}
	}
}

///////////////////////////////////////

void cResMan::RemoveResourceType(const char* pExt, IResType* pType)
{
	cResTypeData* pData;
	int i;

	if (pType)
	{
		if (pExt == NULL)
		{
		}
		else
		{
			pData = m_ResTypeHash.Search(pExt);

			if (pData)
			{
				for (i = 0; i < pData->m_nNumFactories; i++)
				{
					if (pData->m_pFactories[i] == pType)
						break;
				}

				if (i != pData->m_nNumFactories)
				{
					if (pData->m_nNumFactories == 1)
						m_ResTypeHash.Remove(pData);
					else
					{
						pData->m_pFactories[pData->m_nNumFactories - 1]->Release();
						if (i < pData->m_nNumFactories - 1)
							memmove(&pData->m_pFactories[i], &pData->m_pFactories[i + 1], sizeof(IResType*) * (pData->m_nNumFactories - (i + 1)));
						--pData->m_nNumFactories;
					}
				}
			}
		}
	}
}

///////////////////////////////////////

int cResMan::VerifyStorage(IStore* pStorage)
{
	if (pStorage == NULL || !pStorage->Exists())
	return 0;

	return 1;
}

///////////////////////////////////////

void cResMan::MungePaths(const char* pPathName, const char* pExpRelPath, char** ppRelPath, char** ppName, char* pOldSlash, int* pbCombinedPaths)
{
	const char* pImpRelPath = NULL;
	*pbCombinedPaths = 0;

#line 173
	AssertMsg(pPathName != NULL && ppRelPath != NULL && ppName != NULL && pOldSlash != NULL && pbCombinedPaths != NULL, "SplitPath called with missing arguments!");

	*ppRelPath = NULL;
	*ppName = NULL;
	*pOldSlash = '\0';

	size_t nNameSize = strlen(pPathName);

	int i = nNameSize - 1;
	while (i >= 0 && pPathName[i] != '\\' && pPathName[i] != '/')
	i--;

	if ((signed int)(nNameSize - i) > 32 || (signed int)nNameSize > 512)
	{
	Warning(("SplitPath: name too long: %s\n", pPathName));
	}
	else
	{
	int nImpLen;

	if (i < 0)
	{
		*ppName = (char*)pPathName;
	}
	else
	{
		char* pNamePos = const_cast<char*>(&pPathName[i]);
		*pOldSlash = *pNamePos;
		*pNamePos = '\0';
		pImpRelPath = pPathName;
		*ppName = (char*)(pNamePos + 1);
		nImpLen = i;
	}

	if (pExpRelPath)
	{
		if (pImpRelPath)
		{
			*pbCombinedPaths = 1;

			int needSlash;
			size_t nExpLen = strlen(pExpRelPath);
			if (pExpRelPath[nExpLen - 1] != '/' && pExpRelPath[nExpLen - 1] != '\\')
			{
				needSlash = 1;
				++nExpLen;
			}

			*ppRelPath = static_cast<char*>(Malloc(nExpLen + nImpLen + 1));
			strcpy(*ppRelPath, pExpRelPath);

			if (needSlash)
				strcat(*ppRelPath, "\\");
			strcat(*ppRelPath, pImpRelPath);
		}
		else
		{
			*ppRelPath = (char*)pExpRelPath;
		}
	}
	else
	{
		if (pImpRelPath)
			*ppRelPath = (char*)pImpRelPath;
	}
	}
}

///////////////////////////////////////

void cResMan::RestorePath(char* pPath, char* pName, char OldSlash, int bCombinedPaths)
{
	if (OldSlash)
	*(pName - 1) = OldSlash;

	if (bCombinedPaths)
	Free(pPath);
}

///////////////////////////////////////

IRes* cResMan::GetResource(const char* pName, const char* pTypeName, IStore* pStore)
{
#line 294
	AssertMsg(pStore != NULL, "Trying to GetResource without a storage!");
	AssertMsg(pTypeName != NULL, "Trying to GetResource without a type!");

	cResourceData* pData = m_ResTable.FindResData(pName, pStore, 0);
	if (pData == NULL)
	{
	return NULL;
	}

	cResourceTypeData* pTypeData = pData->m_ResourceTypeHash.Search(pTypeName);
	if (pTypeData)
	{
	pTypeData->m_pRes->AddRef();
	return pTypeData->m_pRes;
	}

	return NULL;
}

///////////////////////////////////////

IRes* cResMan::CreateResource(IStore* pStorage, const char* pName, const char* pExt, const char* pTypeName, uint)
{
	char cEmpty = '\0';
	IResMemOverride* pResMem = NULL;
	IRes* pResource = NULL;
	cResourceData* pData = NULL;
	cResourceTypeData* pTypeData = NULL;

#line 339
	AssertMsg(pTypeName != NULL, "CreateResource called without a type!");

	if (pStorage == NULL || pName == NULL)
	return NULL;

	if (pExt == NULL)
	pExt = &cEmpty;

	IResType* pType = GetResType(pTypeName);
	if (pType == NULL)
	{
	CriticalMsg("Caller asked to create unknown type of resource!");
	return NULL;
	}

	pResource = pType->CreateRes(pStorage, pName, pExt, &pResMem);

	if (pResource)
	{
	pData = m_ResTable.FindResData(pName, pStorage, 1);



	AssertMsg(pData != NULL, "Couldn't create entry in global resource table!");

	pTypeData = new cResourceTypeData(pType, pData, pResource);
	pData->m_ResourceTypeHash.Insert(pTypeData);

	pTypeData->m_pResMem = pResMem;
	pTypeData->m_Freshed = m_FreshStamp;
	IResControl* pResControl;
	if (!FAILED(pResource->QueryInterface(IID_IResControl, (void**)&pResControl)))
	{
		pResControl->SetManData(reinterpret_cast<DWORD>(pTypeData));
		pResControl->Release();
	}
	}
	return pResource;
}
///////////////////////////////////////
cResourceTypeData* cResMan::FindResourceTypeData(IStore* pStore, const char* pResName, IResType* pType)
{
	cResourceData* pData = m_ResTable.FindResData(pResName, pStore, 0);
	if (pData)
	{
	const char* pTypeName = pType->GetName();
	cResourceTypeData* pResult = pData->m_ResourceTypeHash.Search(pTypeName);
	return pResult;
	}
	return NULL;
}
///////////////////////////////////////
void cResMan::FreeData(cResourceTypeData* pData, BOOL bTestUser)
{
	IResMemOverride* pResMem;
	cResourceTypeData* pTestData;
	cAutoResThreadLock lock;
	if (!pData)
		return;
	if (pData->m_pProxiedRes)
		pTestData = pData->m_pProxiedRes;
	else
		pTestData = pData;

	AssertMsg2(bTestUser == 0 || pTestData->m_nUserLockCount == 0, "ResMan FreeData callback ignored. UserLock count = %d for %s\n", pTestData->m_nUserLockCount, pTestData->GetName());





	AssertMsg2(pTestData->m_nInternalLockCount == 1, "ResMan FreeData callback ignored. Internal Lock count = %d for %s\n", pTestData->m_nInternalLockCount, pTestData->GetName());

	if (bTestUser && pTestData->m_nUserLockCount || pTestData->m_nInternalLockCount != 1)
	{

		CriticalMsg("Not Freeing locked data");
		return;
	}

	pData->m_nInternalLockCount = 0;
	if (pData->m_pProxiedRes == 0)
	{
		IResControl* pResControl;

		if (pData->m_pResMem)
			pResMem = pData->m_pResMem;
		else
			pResMem = &m_DefResMem;

		if (SUCCEEDED(pData->m_pRes->QueryInterface(IID_IResControl, (void**)&pResControl)))
		{
#ifndef SHIP
			if (g_fResPrintDrops)
			{
				char* pathName;
				pData->m_pRes->GetCanonPathName(&pathName);
				mprintf("cResMan::FreeData(): Freeing %s\n", pathName);

				if (pathName)
				{
#line 439
					Free(pathName);
				}
			}
#endif

#line 443
			if (!pResControl->FreeData(pData->m_pData, pData->m_nSize, pResMem))
				Warning(("cResMan::FreeData -- couldn't free resource!\n"));

			ResStat(m_pResStats->LogStatRes(pData->m_pRes, MemFreed));
			pResControl->Release();
		}
	}
	else
	{
		pData->m_pProxiedRes = NULL;
	}

	pData->m_pData = NULL;
	pData->m_pRes->Release();
}

///////////////////////////////////////

IResType* cResMan::GetResType(const char* pTypeName)
{
	cNamedResType* pTypeEntry = m_ResTypeByNameHash.Search(pTypeName);
	if (!pTypeEntry)
	{
	Warning(("GetResType called with unknown type %s!\n", pTypeName));
	return NULL;
	}

	return pTypeEntry->m_pType;
}
