#include <loopapi.h>
#include <aggmemb.h>

class cGenericLoopClient : public ILoopClient
{
	DECLARE_UNAGGREGATABLE();

public:
	cGenericLoopClient(tLoopClientReceiveMessageFunc pCallback,
		void* pContext,
		const sLoopClientDesc* pClientDesc);

	//
	// Get the version of the loop client interface
	//
	STDMETHOD_(short, GetVersion)()
	{
		return 1;
	}

	//
	// Get the loop client information
	//
	STDMETHOD_(const sLoopClientDesc*, GetDescription)()
	{
		return m_pDesc;
	}

	// Handle a message from the owning loop mode dispatcher
	STDMETHOD_(eLoopMessageResult, ReceiveMessage)(eLoopMessage hMessage, tLoopMessageData hData)
	{
		return m_pCallback(m_pContext, hMessage, hData);
	}

private:
	tLoopClientReceiveMessageFunc m_pCallback;
	const sLoopClientDesc* m_pDesc;
	void* m_pContext;
};

ILoopClient* LGAPI CreateSimpleLoopClient(tLoopClientReceiveMessageFunc pCallback, void* pContext, const sLoopClientDesc* pClientDesc)
{
	return new cGenericLoopClient(pCallback, pContext, pClientDesc);
}

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cGenericLoopClient, ILoopClient);

cGenericLoopClient::cGenericLoopClient(tLoopClientReceiveMessageFunc pCallback,
	void* pContext,
	const sLoopClientDesc* pClientDesc)
	: m_pCallback(pCallback)
{
	m_pDesc = pClientDesc;
	m_pContext = pContext;
}
