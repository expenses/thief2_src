#include "loopman.h"

#include <lgassert.h>
#include <string.h>
#include <gshelapi.h>
#include <appagg.h>
#include <tmdecl.h>
#include <dbmem.h>

tResult LGAPI _LoopManagerCreate(REFIID, ILoopManager** ppLoopManager, IUnknown* pOuterUnknown, unsigned nMaxModes)
{
	cLoopManager* pLoopManager = new cLoopManager(pOuterUnknown, nMaxModes);

	if (ppLoopManager)
		*ppLoopManager = pLoopManager;

	return pLoopManager ? S_OK : E_FAIL;
}


//
// LoopGetMessageName support
//

static const char* ppszLoopMessageNames[] =
{
  "kMsgNull",
  "kMsgStart",
  "kMsgEnd",
  "kMsgExit",
  "kMsgStartBlock",
  "kMsgBlocked",
  "kMsgEndBlock",
  "kMsgBeginFrame",
  "kMsgNormalFrame",
  "kMsgPauseFrame",
  "kMsgEndFrame",
  "kMsgFrameReserved1",
  "kMsgFrameReserved2",
  "kMsgEnterMode",
  "kMsgSuspendMode",
  "kMsgResumeMode",
  "kMsgExitMode",
  "kMsgMinorModeChange",
  "kMsgModeReserved2",
  "kMsgModeReserved3",
  "kMsgLoad",
  "kMsgSave",
  "kMsgUserReserved1",
  "kMsgUserReserved2",
  "kMsgUserReserved3",
  "kMsgApp1",
  "kMsgApp2",
  "kMsgApp3",
  "kMsgApp4",
  "kMsgApp5",
  "kMsgApp6",
  "kMsgApp7",
  "kMsgApp8"
};

const char* LGAPI LoopGetMessageName(eLoopMessage message)
{
	int iMessage;
	unsigned currentMessage;
	currentMessage = 1;
	iMessage = 0;
	while (currentMessage && (currentMessage & message) == 0)
	{
		++iMessage;
		currentMessage <<= 1;
	}

	if (iMessage == 33)
		iMessage = 0;

	return ppszLoopMessageNames[iMessage];
}

//
// Pre-fab COM implementations
//

IMPLEMENT_AGGREGATION_SELF_DELETE(cLoopManager);

cLoopManager::cLoopManager(IUnknown* pOuterUnknown, unsigned nMaxModes)
	: m_Loop(pOuterUnknown, this),
	m_nMaxModes( nMaxModes),
	m_Factory(),
	m_nLoopModes(),
	m_pBaseMode( NULL)
{
	// Add internal components to outer aggregate...
	INIT_AGGREGATION_2(pOuterUnknown,
		IID_ILoopManager,
		this,
		IID_ILoop,
		&m_Loop,
		kPriorityLibrary - 1,
		NULL);
}

cLoopManager::~cLoopManager()
{
}

HRESULT cLoopManager::AddClient(ILoopClient* pClient, ulong* pCookie)
{
	sLoopClientDesc* pClientDesc = (sLoopClientDesc*)pClient->GetDescription();
#line 146
	AssertMsg(!m_Factory.DoesSupport(pClientDesc->pID), "Double add of loop client");

	*pCookie = reinterpret_cast<ulong>(pClientDesc->pID);

	return m_Factory.AddClient(pClientDesc);
}

HRESULT cLoopManager::RemoveClient(ulong cookie)
{
	return m_Factory.RemoveClient(reinterpret_cast<tLoopClientID*>(cookie));
}

HRESULT cLoopManager::AddClientFactory(ILoopClientFactory* pFactory, ulong* pCookie)
{
	*pCookie = reinterpret_cast<ulong>(pFactory);

	return m_Factory.AddInnerFactory(pFactory);
}

HRESULT cLoopManager::RemoveClientFactory(ulong cookie)
{
	return m_Factory.RemoveInnerFactory(reinterpret_cast<ILoopClientFactory*>(cookie));
}

HRESULT cLoopManager::GetClient(tLoopClientID* pID, tLoopClientData data, ILoopClient** ppResult)
{
	return m_Factory.GetClient(pID, data, ppResult);
}

HRESULT cLoopManager::AddMode(const sLoopModeDesc* pDesc)
{
	ILoopMode* pLoopMode = _LoopModeCreate(pDesc);
#line 201
	AssertMsg(pLoopMode != NULL, "Failed to create loop mode");
	if (!pLoopMode)
	{
		return E_FAIL;
	}

	cInterfaceInfo* pInfo = new cInterfaceInfo(pDesc->name.pID, pLoopMode, NULL);
	m_nLoopModes.Insert(pInfo);

	return S_OK;
}

ILoopMode* cLoopManager::GetMode(tLoopModeID* pID)
{
	cInterfaceInfo* pLoopModeInfo = m_nLoopModes.Search(pID);
	if (pLoopModeInfo)
	{
		ILoopMode* pLoopMode;
		pLoopModeInfo->pUnknown->QueryInterface(IID_ILoopMode, (void**)&pLoopMode);

		return pLoopMode;
	}

	return NULL;
}

HRESULT cLoopManager::RemoveMode(tLoopModeID* pID)
{
	cInterfaceInfo* pLoopModeInfo = m_nLoopModes.Search(pID);
#line 237
	AssertMsg(pLoopModeInfo != NULL, "Attempted to remove mode that was never added");
	if (pLoopModeInfo)
	{
		m_nLoopModes.Remove(pLoopModeInfo);

		SafeRelease(pLoopModeInfo->pUnknown);

		delete pLoopModeInfo;

		return S_OK;
	}

	return E_FAIL;
}

HRESULT cLoopManager::SetBaseMode(tLoopModeID* pID)
{
	cInterfaceInfo* pInfo = m_nLoopModes.Search(pID);



	AssertMsg(pInfo != NULL, "Attempted to set a base mode that was never added");
	if (pInfo)
	{
		m_pBaseMode = pID;

		return S_OK;
	}

	m_pBaseMode = NULL;

	return E_FAIL;
}

ILoopMode* cLoopManager::GetBaseMode()
{
	if (!m_pBaseMode)
		return NULL;

	return ((ILoopManager*)this)->GetMode(m_pBaseMode);
}

HRESULT cLoopManager::End()
{
	m_Factory.ReleaseAll();
	m_nLoopModes.ReleaseAll(TRUE);
	return S_OK;
}

inline sModeData::sModeData(const sModeData& d)
{
	*this = d;
}

IMPLEMENT_DELEGATION(cLoopManager::cLoop);

cLoopManager::cLoop* cLoopManager::cLoop::gm_pLoop = NULL;

cLoopManager::cLoop::~cLoop()
{
	delete m_pLoopStack;
#line 312
	AssertMsg(!m_pCurrentDispatch, "Expected exit of loop manager before destruction!");
}

ILoopMode* cLoopManager::cLoop::GetCurrentMode()
{
	const sLoopModeName* pName = m_pCurrentDispatch->Describe(NULL);
	return m_pLoopManager->GetMode(pName->pID);
}

ILoopDispatch* cLoopManager::cLoop::GetCurrentDispatch()
{
	return m_pCurrentDispatch;
}

int cLoopManager::cLoop::Go(sLoopInstantiator* loop)
{
	sLoopTransition trans;
	tLoopModeID* pID = loop->pID;
	int minorMode = loop->minorMode;
	sLoopModeInitParmList init = loop->init;

	trans.from.pID = &GUID_NULL;
	trans.to = *loop;

	m_pLoopStack->Push(sModeData(m_pCurrentDispatch, m_FrameInfo, kLoopResume));
	cLoopModeStack* oldstack = m_pLoopStack;

	m_pLoopStack = new cLoopModeStack();

	ILoopMode* pMode = m_pLoopManager->GetMode(pID);
	if (!pMode)
	{
#ifndef SHIP
#line 359
		CriticalMsg("Attempted to \"go\" on mode that was never added");
#endif
		return E_FAIL;
	}

	pMode->CreateDispatch(init, &m_pNextDispatch);
	SafeRelease(pMode);

	static bool DoneAtExit = false;
	if (!DoneAtExit)
	{
		atexit(cLoop::OnExit);
		DoneAtExit = true;
	}

	m_fNewMinorMode = minorMode;
	m_fState |= kLoopNewMinorMode;

	AutoAppIPtr(GameShell);

	eLoopMessage frameMessage = kMsgNormalFrame;

	m_fState |= kLoopNewMode | kLoopRunning;
	while (m_fState & kLoopRunning)
	{
		if (m_fState & kLoopNewMode)
		{
			eLoopMessage inmsg = kMsgEnterMode;
			if (m_fState & kLoopResume)
				inmsg = kMsgResumeMode;
			else
				m_pNextDispatch->ProcessQueue();

			m_pCurrentDispatch = m_pNextDispatch;
			m_pNextDispatch = NULL;
#ifndef SHIP
			m_pCurrentDispatch->SetDiagnostics(m_fTempDiagnostics, m_tempDiagnosticSet);
			m_pCurrentDispatch->SetProfile(m_TempProfileSet, m_pTempProfileClientId);
#endif
			m_pCurrentDispatch->SendMessage(inmsg, reinterpret_cast<tLoopMessageData>(&trans), 1);
			
			m_fState &= ~(kLoopResume | kLoopSuspend | kLoopNewMode);
			m_FrameInfo.nTicks = tm_get_millisec();
			m_FrameInfo.dTicks = 0;
		}

		if (m_fState & kLoopNewMinorMode)
		{
			m_FrameInfo.fMinorMode = m_fNewMinorMode;
			m_pCurrentDispatch->SendMessage(kMsgMinorModeChange, reinterpret_cast<tLoopMessageData>(m_FrameInfo.fMinorMode), kDispatchForward);
			m_fState &= ~kLoopNewMinorMode;
		}

		if (pGameShell != NULL)
			pGameShell->BeginFrame();

		m_pCurrentDispatch->SendMessage(kMsgBeginFrame, reinterpret_cast<tLoopMessageData>(&m_FrameInfo), kDispatchForward);
		if (pGameShell != NULL)
			pGameShell->PumpEvents(0);

		m_pCurrentDispatch->SendMessage(frameMessage, reinterpret_cast<tLoopMessageData>(&m_FrameInfo), kDispatchForward);
		if (pGameShell != NULL)
			pGameShell->PumpEvents(0);

		m_pCurrentDispatch->ProcessQueue();
		m_pCurrentDispatch->SendMessage(kMsgEndFrame, reinterpret_cast<tLoopMessageData>(&m_FrameInfo), kDispatchReverse);
		if (pGameShell != NULL)
			pGameShell->EndFrame();

		++m_FrameInfo.nCount;

		long ticks = tm_get_millisec();
		m_FrameInfo.dTicks = ticks - m_FrameInfo.nTicks;
		m_FrameInfo.nTicks = ticks;

		if (m_fState & (kLoopUnpauseRequest | kLoopPauseRequest))
		{
			if (m_fState & kLoopPauseRequest)
				frameMessage = kMsgPauseFrame;
			else
				frameMessage = kMsgNormalFrame;

			m_fState &= ~(kLoopUnpauseRequest | kLoopPauseRequest);
			m_fState |= kLoopPaused;
		}

		if ((m_fState & kLoopNewMode) || !(m_fState & kLoopRunning))
		{
			eLoopMessage outmsg = kMsgExitMode;
			if (m_fState & kLoopSuspend)
				outmsg = kMsgSuspendMode;

			trans.from.pID = m_pCurrentDispatch->Describe(&trans.from.init)->pID;
			trans.from.minorMode = m_FrameInfo.fMinorMode;

			if (m_pNextDispatch)
			{
				trans.to.pID = m_pNextDispatch->Describe(&trans.to.init)->pID;
				trans.to.minorMode = m_fNewMinorMode;
			}
			else
			{
				trans.to.pID = &GUID_NULL;
			}

			m_pCurrentDispatch->SendMessage(outmsg, reinterpret_cast<tLoopMessageData>(&trans), 2);
#ifndef SHIP
			m_pCurrentDispatch->GetDiagnostics(&m_fTempDiagnostics, &m_tempDiagnosticSet);
			m_pCurrentDispatch->GetProfile(&m_TempProfileSet, &m_pTempProfileClientId);
#endif
			if (!(m_fState & kLoopSuspend))
				m_pCurrentDispatch->Release();
		}
	}

	delete m_pLoopStack;
	m_pLoopStack = oldstack;

	sModeData old = oldstack->Pop();

	m_pCurrentDispatch = old.dispatch;
	m_FrameInfo = old.frame;

	return m_fGoReturn;
}

void cLoopModeStack::Push(sModeData data)
{
	cLoopModeElem* pElem = new cLoopModeElem(&data);
	Prepend(pElem);
}

HRESULT cLoopManager::cLoop::EndAllModes(int goRetVal)
{
	if (!m_pCurrentDispatch)
		return S_FALSE;

	m_fGoReturn = goRetVal;

	sModeData mode;
	mode = m_pLoopStack->Pop();

	while (mode.dispatch)
	{
		sLoopTransition trans;

		trans.from.pID = mode.dispatch->Describe(&trans.from.init)->pID;
		trans.from.minorMode = mode.frame.fMinorMode;
		trans.to.pID = &GUID_NULL;

		mode.dispatch->SendMessage(kMsgExitMode, reinterpret_cast<tLoopMessageData>(&trans), 2);
		mode.dispatch->Release();

		mode = m_pLoopStack->Pop();
	}

	m_fState &= ~kLoopRunning;

	return S_OK;
}

HRESULT cLoopManager::cLoop::Terminate()
{
	if (m_pCurrentDispatch)
	{
		sLoopTransition trans;
		trans.from.pID = m_pCurrentDispatch->Describe(&trans.from.init)->pID;
		trans.from.minorMode = m_FrameInfo.fMinorMode;
		trans.to.pID = &GUID_NULL;

		m_pCurrentDispatch->SendMessage(kMsgExitMode, reinterpret_cast<tLoopMessageData>(&trans), 2);
		if (m_pCurrentDispatch)
			m_pCurrentDispatch->Release();
		m_pCurrentDispatch = NULL;

		EndAllModes(0);
	}

	return S_OK;
}

const sLoopFrameInfo* cLoopManager::cLoop::GetFrameInfo(void)
{
	return &m_FrameInfo;
}

HRESULT cLoopManager::cLoop::ChangeMode(eLoopModeChangeKind kind, sLoopInstantiator* loop)
{
#line 615
	AssertMsg(m_pCurrentDispatch, "Changing modes outside GO");

	ILoopMode* next = m_pLoopManager->GetMode(loop->pID);
	AssertMsg(next, "Change to unknown loopmode");
	ILoopDispatch* dispatch = NULL;

	if (m_fState & kLoopNewMode)
	{
		if (!m_pNextDispatch || *loop->pID == *m_pNextDispatch->Describe(NULL)->pID)
			return S_FALSE;
	}

	if (!kind && (m_fState & (kLoopResume | kLoopNewMode)) == (kLoopResume | kLoopNewMode))
		return E_FAIL;

	if (m_fState & kLoopNewMode)
	{
		if (!kind)
		{
			sLoopFrameInfo push_info = m_FrameInfo;
			push_info.fMinorMode = m_fNewMinorMode;

			m_pLoopStack->Push(sModeData(m_pNextDispatch, push_info, 0));

			kind = kLoopModeSwitch;
		}
		else
		{
			if (m_fState & kLoopSuspend)
			{
				sModeData result = m_pLoopStack->Pop();
				m_fState &= ~kLoopSuspend;
			}

			SafeRelease(m_pNextDispatch);
		}
	}


	if (kind == kLoopModeUnwindTo)
	{
		sModeData mode;
		sLoopTransition trans;
		trans.to.pID = next->GetName()->pID;
		trans.to.minorMode = 0;
		trans.to.init = NULL;

		mode = m_pLoopStack->Pop();

		while (mode.dispatch)
		{
			trans.from.pID = mode.dispatch->Describe(&trans.from.init)->pID;

			if (*trans.from.pID == *trans.to.pID)
			{
				dispatch = mode.dispatch;

				m_fNewMinorMode = mode.frame.fMinorMode;
				m_fState |= kLoopResume;

				break;
			}

			trans.from.minorMode = mode.frame.fMinorMode;
			mode.dispatch->SendMessage(kMsgExitMode, reinterpret_cast<tLoopMessageData>(&trans), 2);
			mode.dispatch->Release();

			mode = m_pLoopStack->Pop();
		}
	}

	if (!dispatch)
	{
		next->CreateDispatch(loop->init, &dispatch);
		m_fNewMinorMode = loop->minorMode;
	}

	m_pNextDispatch = dispatch;
	m_fState |= (kLoopNewMode | kLoopNewMinorMode | kLoopUnpauseRequest);

	if (!kind)
	{
		m_pLoopStack->Push(sModeData(m_pCurrentDispatch, m_FrameInfo, kLoopResume));
		m_fState |= kLoopSuspend;
	}

	return S_OK;
}

HRESULT cLoopManager::cLoop::EndMode(int goRetVal)
{
#line 719
	AssertMsg(m_pCurrentDispatch, "No loop mode to end!");

	m_fGoReturn = goRetVal;

	sModeData next = m_pLoopStack->Pop();

	if (next.dispatch)
	{
		m_pNextDispatch = next.dispatch;
		m_fNewMinorMode = next.frame.fMinorMode;
		m_fState |= next.flags | (kLoopNewMode | kLoopNewMinorMode);
	}
	else
	{
		m_fState &= ~kLoopRunning;
		m_fState |= kLoopNewMode;
		m_pNextDispatch = NULL;
	}

	return S_OK;
}

void cLoopManager::cLoop::Pause(BOOL fPause)
{
	BOOL paused = m_fState & kLoopPaused;
	if (fPause)
	{
		if (!paused)
			m_fState |= kLoopPauseRequest;
	}
	else if (paused)
		m_fState |= kLoopUnpauseRequest;
}

BOOL cLoopManager::cLoop::IsPaused(void)
{
	return (m_fState >> 1) & 1;
}

HRESULT cLoopManager::cLoop::ChangeMinorMode(int minorMode)
{
	if (minorMode != m_FrameInfo.fMinorMode)
	{
		m_fNewMinorMode = minorMode;
		m_fState |= kLoopNewMinorMode;
		return S_OK;
	}

	return S_FALSE;
}

int cLoopManager::cLoop::GetMinorMode(void)
{
	return m_FrameInfo.fMinorMode;
}

HRESULT cLoopManager::cLoop::SendMessage(eLoopMessage message, tLoopMessageData hData, int flags)
{
	if (m_pCurrentDispatch)
	{
		HRESULT result = m_pCurrentDispatch->SendMessage(message, hData, flags);
		if (!result)
			return result;
	}
#line 794
	AssertMsg(m_pCurrentDispatch, "Cannot dispatch messages: no loop mode");
	return E_FAIL;
}

HRESULT cLoopManager::cLoop::SendSimpleMessage(eLoopMessage message)
{
	if (m_pCurrentDispatch)
	{
		HRESULT result = m_pCurrentDispatch->SendSimpleMessage(message);
		if (!result)
			return result;
	}
#line 803
	AssertMsg(m_pCurrentDispatch, "Cannot dispatch messages: no loop mode");
	return E_FAIL;
}

HRESULT cLoopManager::cLoop::PostMessage(eLoopMessage message, tLoopMessageData hData, int flags)
{
	if (m_pCurrentDispatch)
	{
		HRESULT result = m_pCurrentDispatch->PostMessage(message, hData, flags);
		if (!result)
			return result;
	}
#line 812
	AssertMsg(m_pCurrentDispatch, "Cannot dispatch messages: no loop mode");
	return E_FAIL;
}

HRESULT cLoopManager::cLoop::PostSimpleMessage(eLoopMessage message)
{
	if (m_pCurrentDispatch)
	{
		HRESULT result = m_pCurrentDispatch->PostSimpleMessage(message);
		if (!result)
			return result;
	}
#line 821
	AssertMsg(m_pCurrentDispatch, "Cannot dispatch messages: no loop mode");
	return E_FAIL;
}

HRESULT cLoopManager::cLoop::ProcessQueue(void)
{
	if (m_pCurrentDispatch)
	{
		HRESULT result = m_pCurrentDispatch->ProcessQueue();
		if (!result)
			return result;
	}
#line 830
	AssertMsg(m_pCurrentDispatch, "Cannot dispatch messages: no loop mode");
	return E_FAIL;
}

#ifndef SHIP
void cLoopManager::cLoop::SetDiagnostics(uint fDiagnostics, tLoopMessageSet messages)
{
	if (m_pCurrentDispatch)
		m_pCurrentDispatch->SetDiagnostics(fDiagnostics, messages);
	else
	{

		m_fTempDiagnostics = fDiagnostics;
		m_tempDiagnosticSet = messages;
	}
}

void cLoopManager::cLoop::GetDiagnostics(uint* pfDiagnostics, tLoopMessageSet* pMessages)
{
	if (m_pCurrentDispatch)
		m_pCurrentDispatch->GetDiagnostics(pfDiagnostics, pMessages);
	else
	{

		*pfDiagnostics = m_fTempDiagnostics;
		*pMessages = m_tempDiagnosticSet;
	}
}

void cLoopManager::cLoop::SetProfile(tLoopMessageSet messages, tLoopClientID* pClientId)
{
	if (m_pCurrentDispatch)
		m_pCurrentDispatch->SetProfile(messages, pClientId);
	else
	{
		m_TempProfileSet = messages;
		m_pTempProfileClientId = pClientId;
	}
}

void cLoopManager::cLoop::GetProfile(tLoopMessageSet* pMessages, tLoopClientID** ppClientId)
{
	if (m_pCurrentDispatch)
		m_pCurrentDispatch->GetProfile(pMessages, ppClientId);
	else
	{
		*pMessages = m_TempProfileSet;
		*ppClientId = m_pTempProfileClientId;
	}
}
#endif

void cLoopManager::cLoop::OnExit()
{
	if (gm_pLoop && gm_pLoop->m_pCurrentDispatch)
	{
		gm_pLoop->m_pCurrentDispatch->Release();
		gm_pLoop->m_pCurrentDispatch = NULL;
	}
}