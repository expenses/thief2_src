#include <loopfact.h>

// implement hash sets
#define HASHSET_TEMPLATE_NO_DELETE
#include <hashset.h>
#include <hshsttem.h>

template class cCTUnaggregated<ILoopClientFactory, &IID_ILoopClientFactory, kCTU_Default>;

tHashSetKey cLoopClientDescTable::GetKey(tHashSetNode node) const
{
	return (tHashSetKey)(((sLoopClientDesc*)(node))->pID);
}

//
// Pre-fab COM implementations
//
// IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cLoopClientFactory, ILoopClientFactory);

cLoopClientFactory::cLoopClientFactory()
	: m_ClientDescs(), m_InnerFactories() { }

tLoopClientID** cLoopClientFactory::QuerySupport()
{
	cDynArray<tLoopClientID*> clientGuids;
	tHashSetHandle h;

	for (const sLoopClientDesc* pClientDesc = m_ClientDescs.GetFirst(h); pClientDesc != NULL; pClientDesc = m_ClientDescs.GetNext(h))
		clientGuids.Append(pClientDesc->pID);

	clientGuids.Append(NULL);

	return clientGuids.Detach();
}

BOOL cLoopClientFactory::DoesSupport(tLoopClientID* clientGuid)
{
	if (m_ClientDescs.Search(clientGuid))
		return TRUE;
	return FALSE;
}

HRESULT cLoopClientFactory::GetClient(tLoopClientID* pID, tLoopClientData data, ILoopClient** ppResult)
{
	const sLoopClientDesc* pClientDesc = m_ClientDescs.Search(pID);

	*ppResult = NULL;
	if (pClientDesc)
	{
		switch (pClientDesc->factoryType)
		{
		case kLCF_None:
			return ppResult != NULL ? S_OK : E_FAIL;
		case kLCF_Singleton:
			*ppResult = pClientDesc->pClient;
			(*ppResult)->AddRef();
			break;
		case kLCF_Callback:
			*ppResult = pClientDesc->pfnFactory(pClientDesc, data);
			break;
		case kLCF_FactObj:
			pClientDesc->pFactory->GetClient(pID, data, ppResult);
			break;
		default:
#line 107
			CriticalMsg("Invalid factory type");
			break;
		}
	}
	else
	{
		for (int i = 0; !*ppResult && i < m_InnerFactories.Size(); ++i)
			m_InnerFactories[i]->GetClient(pID, data, ppResult);
	}

	return ppResult != NULL ? S_OK : E_FAIL;
}

HRESULT cLoopClientFactory::AddInnerFactory(ILoopClientFactory* pFactory)
{
	m_InnerFactories.Append(pFactory);
	pFactory->AddRef();

	return S_OK;
}

HRESULT cLoopClientFactory::RemoveInnerFactory(ILoopClientFactory* pFactory)
{
	for (int i = 0; i < m_InnerFactories.Size(); ++i)
	{
		if (m_InnerFactories[i] == pFactory)
		{
			pFactory->Release();
			m_InnerFactories.FastDeleteItem(i);

			return S_OK;
		}
	}

	return E_FAIL;
}

void cLoopClientFactory::ReleaseAll()
{
	for (int i = 0; i < m_InnerFactories.Size(); ++i)
		m_InnerFactories[i]->Release();
}

HRESULT cLoopClientFactory::AddClient(const sLoopClientDesc* pClientDesc)
{
#line 160
	AssertMsg(m_ClientDescs.Search(pClientDesc->pID) == NULL, "Double add of loop client");
	m_ClientDescs.Insert(pClientDesc);

	switch (pClientDesc->factoryType) {
	case kLCF_Singleton:
		pClientDesc->pClient->AddRef();
		break;
	case kLCF_FactObj:
		pClientDesc->pFactory->AddRef();
		break;
	}

	return S_OK;
}

HRESULT cLoopClientFactory::AddClients(const sLoopClientDesc** ppClientDesc)
{
	int result = S_OK;

	for (;*ppClientDesc != NULL; ++ppClientDesc)
	{
		if (AddClient(*ppClientDesc) != S_OK)
			result = E_FAIL;
	}

	return result;
}

HRESULT cLoopClientFactory::RemoveClient(tLoopClientID* pID)
{
	const sLoopClientDesc* pClientDesc = m_ClientDescs.RemoveByKey(pID);
#line 199
	AssertMsg(pClientDesc, "Client to remove from simple factory is not present");
	if (pClientDesc)
	{
		switch (pClientDesc->factoryType)
		{
		case kLCF_Singleton:
			pClientDesc->pClient->Release();
			break;
		case kLCF_FactObj:
			pClientDesc->pFactory->Release();
			break;
		}
	}

	return pClientDesc != NULL ? S_OK : E_FAIL;
}

short cLoopClientFactory::GetVersion()
{
	return 1;
}

ILoopClientFactory* LGAPI CreateLoopFactory(const sLoopClientDesc** descs)
{
	cLoopClientFactory* pLoopClientFactory = new cLoopClientFactory();
	pLoopClientFactory->AddClients(descs);
	return pLoopClientFactory;
}
