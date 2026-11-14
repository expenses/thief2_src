#include <memory>

#include <loopapi.h>
#include <loopdisp.h>
#include <lgassert.h>

class cLoopMode : public ILoopMode
{
public:
	DECLARE_UNAGGREGATABLE();

	cLoopMode(const sLoopModeDesc* pDescription);

	virtual ~cLoopMode();

	// Get the info on the loop mode
	STDMETHOD_(const sLoopModeName*, GetName)()
	{
		return &m_desc.name;
	}

	// Create the mode dispatch chain
	STDMETHOD(CreateDispatch)(sLoopModeInitParmList paramList, ILoopDispatch** dispatch);

	// Create a dispatch chain that only handles certain messages
	STDMETHOD(CreatePartialDispatch)(sLoopModeInitParmList paramList, tLoopMessageSet messageSet, ILoopDispatch** dispatch);

	// Describe this mode
	STDMETHOD_(const sLoopModeDesc*, Describe)()
	{
		return &m_desc;
	}

private:
	sLoopModeDesc m_desc;
};

ILoopMode* LGAPI _LoopModeCreate(const sLoopModeDesc* pDescription)
{
	return new cLoopMode( pDescription);
}

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cLoopMode, ILoopMode);

cLoopMode::cLoopMode(const sLoopModeDesc* pDescription)
{
#line 38
	AssertMsg(pDescription->ppClientIDs, "Empty loop modes are not supported");

	m_desc = *pDescription;
	m_desc.ppClientIDs = new tLoopClientID * [m_desc.nClients];
	memcpy(m_desc.ppClientIDs, pDescription->ppClientIDs, sizeof(tLoopClientID*) * pDescription->nClients);
}

cLoopMode::~cLoopMode()
{
	delete m_desc.ppClientIDs;
}

STDMETHODIMP cLoopMode::CreatePartialDispatch(sLoopModeInitParmList paramList, tLoopMessageSet messageSet, ILoopDispatch** dispatch)
{
	*dispatch = new cLoopDispatch(this, paramList, messageSet);
	if (*dispatch)
		return S_OK;
	else
		return E_FAIL;
}

STDMETHODIMP cLoopMode::CreateDispatch(sLoopModeInitParmList paramList, ILoopDispatch** dispatch)
{
	return CreatePartialDispatch(paramList, -1u, dispatch);
}
