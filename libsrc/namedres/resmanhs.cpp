#include <resmanhs.h>
#include <storeapi.h>
#include <filespec.h>
#include <str.h>

cResourceTypeData::cResourceTypeData(IResType* pType, cResourceData* pResData, IRes* pRes)
{
	m_pData = NULL;
	m_nSize = 0;
	m_pResMem = NULL;
	m_nUserLockCount = 0;
	m_nInternalLockCount = 0;
	m_pRes = NULL;
	m_pType = NULL;
	m_pResourceData = pResData;
	m_pProxiedRes = NULL;
	m_Freshed = 0;

#line 47
	AssertMsg(pResData, "cResourceTypeData created without a pResData!");

	if (pType)
	{
		m_pType = pType;
		m_pType->AddRef();
	}

	if (pRes)
		m_pRes = pRes;
}

cResourceTypeData::~cResourceTypeData()
{
#line 68
	AssertMsg1(m_pData == NULL, "Not freeing Data for resource %s\n", GetName());

	if (m_pResMem)
	{
		m_pResMem->Release();
		m_pResMem = NULL;
	}

	if (m_pType)
	{
		m_pType->Release();
	}

	if (m_pRes)
	{
		m_pRes = NULL;
	}
}

cHashByResourceType::cHashByResourceType()
{
}

cHashByResourceType::~cHashByResourceType()
{
	DestroyAll();
}

tHashSetKey cHashByResourceType::GetKey(tHashSetNode p) const
{
	if (((cResourceTypeData*)p)->m_pType)
		return (tHashSetKey)((cResourceTypeData*)p)->m_pType->GetName();
	else
		return NULL;
}


cResourceData::cResourceData(IStore* pStore, cResourceName* pNameData)
	: m_pStore(pStore), m_pNameData(pNameData), m_ResourceTypeHash(), m_pNext(0)
{
	if (pStore)
		pStore->AddRef();
}

cResourceData::~cResourceData()
{
	m_ResourceTypeHash.DestroyAll();

	SafeRelease(m_pStore);
}

cResourceName::cResourceName(const char* pResName)
{
	m_pFirstStream = NULL;
	m_ppFullNames = NULL;
	m_fFlags = 0;

	if (pResName)
	{
		size_t len = strlen(pResName);
		m_pName = static_cast<char*>(Malloc(len + 1));
		memmove(m_pName, pResName, len + 1);
		strlwr(m_pName);
	}
	else
	{
		m_pName = NULL;
	}
}

cResourceName::~cResourceName()
{
	if (m_fFlags & 1)
	{
		for (int i = 0; i < m_ppFullNames->Size(); ++i)
			Free((*m_ppFullNames)[i]);

		delete m_ppFullNames;
	}
	else
	{
		cResourceData* pData = m_pFirstStream;
		while (pData)
		{
			cResourceData* pNext = pData->m_pNext;
			delete pData;
			pData = pNext;
		}
	}

	m_pFirstStream = NULL;
	if (m_pName)
	{
		Free(m_pName);
		m_pName = NULL;
	}
}



cHashByResName::cHashByResName()
{
}

cHashByResName::~cHashByResName()
{
	DestroyAll();
}

cResourceData* cHashByResName::FindResData(const char* pName, IStore* pStore, BOOL bCreate)
{
	if (bCreate && !pStore)
	{
#line 194
		CriticalMsg("FindResData: Can't create without pStore!");
		return NULL;
	}

	if (!pName || *pName == '\0')
	{
		Warning(("FindResData: Empty name!"));
		return NULL;
	}

	cResourceData* pData = NULL;
	cResourceName* pNameData = Search(pName);
	if (pNameData)
	{
		cResourceData* pPrev;

		if (pNameData->m_fFlags & 1)
		{
			for (int i = 0; i < pNameData->m_ppFullNames->Size(); ++i)
			{
				pData = FindResData((*pNameData->m_ppFullNames)[i], pStore, FALSE);
				if (pData)
					return pData;
			}

			return NULL;
		}
		else
		{
			pData = pNameData->m_pFirstStream;
			pPrev = pData;
			if (pStore == NULL && pData)
				return pData;

			while (pData)
			{
				if (pData->m_pStore == pStore)
					return pData;
				pPrev = pData;
				pData = pData->m_pNext;
			}

			if (bCreate)
			{





				pData = new cResourceData(pStore, pNameData);
				if (pPrev)
					pPrev->m_pNext = pData;
				else
					pNameData->m_pFirstStream = pData;
			}

			return pData;
		}
	}
	else if (bCreate)
	{
		pNameData = new cResourceName(pName);
		pData = new cResourceData(pStore, pNameData);
		pNameData->m_pFirstStream = pData;

		Insert(pNameData);

		{
			cFileSpec fileSpec(pName);
			cStr root;

			fileSpec.GetFileRoot(root);
			if (strcmpi(pName, root) != 0)
			{
				pNameData = Search(root);
				if (!pNameData)
				{
					pNameData = new cResourceName(root);
					pNameData->m_fFlags |= 1;

					pNameData->m_ppFullNames = new cDynArray<char*>();
					Insert(pNameData);
				}

				char* pNameCopy = static_cast<char*>(Malloc(strlen(pName) + 1));
				strcpy(pNameCopy, pName);
				pNameData->m_ppFullNames->Append(pNameCopy);
			}
			else
				Warning(("FindResData: Created real entry without extension!\n"));
		}
	}

	return pData;
}

cResTypeData::cResTypeData(const char* pName, IResType* pType)
{
	if (pName && pType)
	{
		strncpy(m_Name, pName, kNameSize);
		m_Name[kNameSize - 1] = '\0';
		strlwr(m_Name);

		pType->AddRef();

		m_pFactories = ((IResType**)(Malloc(sizeof(IResType*))));
		*m_pFactories = pType;
		m_nNumFactories = 1;
	}
	else
	{
		m_Name[0] = '\0';
		m_pFactories = 0;
		m_nNumFactories = 0;
	}
}

cResTypeData::~cResTypeData()
{
	if (m_nNumFactories != 0)
	{
		for (int i = 0; i < m_nNumFactories; ++i)
			m_pFactories[i]->Release();

		Free(m_pFactories);
		m_pFactories = NULL;

		m_nNumFactories = 0;
	}
}

cInstalledResTypeHash::cInstalledResTypeHash()
{
}

cInstalledResTypeHash::~cInstalledResTypeHash()
{
	DestroyAll();
}

cNamedResType::cNamedResType(IResType* pType)
{





	AssertMsg(pType != NULL, "Creating a cNamedResType with a NULL type!");

	m_pType = pType;
	pType->AddRef();
}

cNamedResType::~cNamedResType()
{
	m_pType->Release();
	m_pType = NULL;
}

cInstalledResTypeByName::cInstalledResTypeByName()
{
}

cInstalledResTypeByName::~cInstalledResTypeByName()
{
	DestroyAll();
}

tHashSetKey cInstalledResTypeByName::GetKey(tHashSetNode p) const
{
	return (tHashSetKey)((cNamedResType*)p)->m_pType->GetName();
}
