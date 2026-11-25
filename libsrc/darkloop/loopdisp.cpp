#include "loopdisp.h"
#include "loopman.h"
#include "loopguid.h"

#include <loopapi.h>
#include <appagg.h>
#include <mprintf.h>
#include <lgassert.h>

IMPLEMENT_UNAGGREGATABLE_NO_FINAL_RELEASE(cLoopDispatch, ILoopDispatch);
static void TableAddClientConstraints(ConstraintTable& table, const sLoopClientDesc* desc);
static unsigned MessageToIndex(eLoopMessage message);
#ifndef SHIP
static char LoopTrack(uint message, const char* pszClient);
static char LoopTrackClear();
#endif

cLoopDispatch::cLoopDispatch(ILoopMode* loop, sLoopModeInitParmList parmList, tLoopMessageSet msgs)
	: m_msgs( msgs | 3),
	m_Queue(),
	m_DispatchLists(),
	m_pLoopMode( loop),
	m_aClientInfo(),
#ifndef SHIP
	m_fDiagnostics( 0),
	m_diagnosticSet( 0),
	m_ProfileSet( 0),
	m_pProfileClientId( 0),
	m_AverageFrameTimer(),
	m_TotalModeTime(),
#endif
	m_Parms( parmList)
{
	ConstraintTable constraints;
	AddClientsFromMode(loop, constraints);

	AutoAppIPtr(LoopManager);

	if (pLoopManager != NULL)
	{
		ILoopMode* pBaseMode = pLoopManager->GetBaseMode();
		AddClientsFromMode(pBaseMode, constraints);
		if (pBaseMode)
			pBaseMode->Release();
	}

	SortClients(constraints);
	SafeRelease(pLoopManager);

	SendSimpleMessage(kMsgStart);
}

void cLoopDispatch::SortClients(ConstraintTable& constraints)
{
	for (int i = 0; i < ConstraintTable::kConstraintTableSize; ++i)
		m_DispatchLists[i].Sort(constraints[i].AsPointer(), constraints[i].Size());
}

static void TableAddClientConstraints(ConstraintTable& table, const sLoopClientDesc* desc)
{
	for (const sLoopConstraint* pRel = desc->dispatchConstraints; pRel->constraint.kind != kNullConstraint; ++pRel)
	{
#line 147
	    AssertMsg(!(pRel->constraint.kind != kConstrainBefore) || !(pRel->constraint.kind != kConstrainAfter), "Bad constraint");

		sAbsoluteConstraint absoluteConstraint;
		MakeAbsolute(pRel->constraint, desc->pID, absoluteConstraint);

		unsigned currentMessage = 1;
		unsigned i = 0;
		while (currentMessage)
		{
			if (currentMessage & pRel->messages)
				table[i].Append(absoluteConstraint);

			++i;
			currentMessage <<= 1;
		}
	}
}

void cLoopDispatch::AddClientsFromMode(ILoopMode* pLoop, ConstraintTable& constraints)
{
	if (!pLoop)
		return;

	AutoAppIPtr(LoopManager);

	if (!pLoopManager)
#line 179
		CriticalMsg("Failed to locate ILoopManager implementation");

	BOOL fFirstWarning = TRUE;
	const sLoopModeDesc* desc = pLoop->Describe();

	tLoopClientID** ppIDs = desc->ppClientIDs;
	tLoopClientID** ppLimit = &ppIDs[desc->nClients];

	cInitParmTable& initParmTable = m_Parms;

	for (; ppIDs < ppLimit; ++ppIDs)
	{
		tLoopClientID* pID = *ppIDs;

		if (*pID == GUID_NULL)
			continue;

		bool next_client = false;
		for (int idx = 0; idx < m_aClientInfo.Size(); ++idx)
			if (*pID == *m_aClientInfo[idx].priIntInfo.pID)
			{
				next_client = true;
				break;
			}

		if (next_client)
			continue;

		sLoopModeInitParm* pInitParms = initParmTable.Search(pID);

		ILoopClient* pClient = NULL;
		pLoopManager->GetClient(pID, pInitParms != NULL ? pInitParms->data : NULL, &pClient);
		if (!pClient)
		{
			ILoopMode* mode = pLoopManager->GetMode(pID);
			if (mode)
			{
				AddClientsFromMode(mode, constraints);

				SafeRelease(mode);
			}
			else if (fFirstWarning)
			{
				Warning(("Loop mode \"%s\" did not find all expected clients\n", (const char*)(desc->name.szName)));
				fFirstWarning = FALSE;
			}
			continue;
		}

		const sLoopClientDesc* pClientDesc = pClient->GetDescription();
		int infoidx = m_aClientInfo.Append(sClientInfo());

		sClientInfo& info = m_aClientInfo[infoidx];
#ifndef SHIP
		info.timer.SetName(pClientDesc->szName);
#endif
		info.priIntInfo.pID = pID;
		info.priIntInfo.pInterface = pClient;
		info.priIntInfo.priority = pClientDesc->priority;
		info.priIntInfo.nameStr = pClientDesc->szName;
		info.priIntInfo.pData = &m_aClientInfo[infoidx];
		info.interests = pClientDesc->interests;

		if (info.interests & m_msgs)
		{
			unsigned msg, i;
			for (msg = 1, i = 0; msg > 0; i++, msg <<= 1)
			{
				if (info.interests & msg & m_msgs)
					m_DispatchLists[i].Append(&info.priIntInfo);
			}
		}
		TableAddClientConstraints(constraints, pClientDesc);
	}
}

cLoopDispatch::~cLoopDispatch()
{
	SendMessage(kMsgEnd, NULL, 2);

	for (int i = 0; i < m_aClientInfo.Size(); ++i)
#line 170
		SafeRelease(m_aClientInfo[i].priIntInfo.pInterface);
}

#ifndef SHIP
extern "C" eLoopMessageResult LoopProfileSend(ILoopClient* pClient, eLoopMessage message, tLoopMessageData hData)
{
	return pClient->ReceiveMessage(message, hData);
}

void cLoopDispatch::ClearTimers()
{
	m_TotalModeTime.Clear();
	m_AverageFrameTimer.Clear();

	for (int i = 0; i < m_aClientInfo.Size(); ++i)
		m_aClientInfo[i].timer.Clear();
}

static int TimerSortFunc(const void* pLeft, const void* pRight)
{
	double result = (*(cAverageTimer**)pRight)->GetResult() - (*(cAverageTimer**)pLeft)->GetResult();

	if (result > 0.0)
		return 1;
	else if (result < 0.0)
		return -1;
	return 0;
}

void cLoopDispatch::DumpTimerInfo()
{
	cAnsiStr msg;
	int i;
	BOOL was_active = m_AverageFrameTimer.IsActive();

	if (was_active)
	{
		m_AverageFrameTimer.Stop();

		for (i = 0; i < m_aClientInfo.Size(); ++i)
			m_aClientInfo[i].timer.Stop();
	}

	if (m_AverageFrameTimer.GetIters() >= 10)
	{
		msg.FmtStr(
			1280,
			"+----------------------------------------------------------------------------+\n"
			"|                       FRAME TIMINGS: %-16s                      |\n"
			"+----------------------------------------------------------------------------+\n"
			"\n"
			"Frames sampled:     %12d frames\n"
			" \n"
			"In-loop time:       %12.0f ms    Total mode time:    %12.0f ms\n"
			"In-loop frame avg:  %12.2f ms    Total out-of-loop:  %12.0f ms\n"
			"In-loop framerate:  %12.2f fps   Mode framerate:     %12.2f fps\n"
			" \n",
			m_pLoopMode->GetName()->szName,
			m_AverageFrameTimer.GetIters(),
			(double)m_AverageFrameTimer.GetTotalTime(),
			m_TotalModeTime.GetResult() - 2500.0,
			m_AverageFrameTimer.GetResult(),
			m_TotalModeTime.GetResult() - 2500.0 - static_cast<double>(m_AverageFrameTimer.GetTotalTime()),
			1000.0 / m_AverageFrameTimer.GetResult(),
			1000.0 / ((m_TotalModeTime.GetResult() - 2500.0) / static_cast<double>(m_AverageFrameTimer.GetIters())));
	}
	else
	{
		msg.FmtStr("+----------------------------------------------------------------------------+\n"
			"[ Insufficient samples to calculate frame timings ]\n");
	}

	mprint(msg);
	if (m_AverageFrameTimer.GetIters() >= 10)
	{
		mprintf(" Loop Client                Average Time     %% of Frame  Max Time  Min Time\n"
			" ----------------------------------------------------------------------------\n");

		cDynArray<cAverageTimer*> timers;
		for (i = 0; i < m_aClientInfo.Size(); ++i)
		{
			timers.Append(&m_aClientInfo[i].timer);
			timers.Sort((cDynArray<cAverageTimer*>::tCompareFunc)TimerSortFunc);
		}

		double clientAvg;
		double clientPctOfFrame;
		double frameAvg = m_AverageFrameTimer.GetResult();
		double sumAvg = 0.0;

		for (i = 0; i < timers.Size(); ++i)
		{
			clientAvg = timers[i]->GetResult();
			clientPctOfFrame = clientAvg / frameAvg * 100.0;
			if (clientAvg > 1.0)
			{
				sumAvg += clientAvg;

				msg.FmtStr(" %-25s %#12.4f %#12.2f%% %#10lu %#10lu\n",
					timers[i]->GetName(), clientAvg, clientPctOfFrame, timers[i]->GetMaxTime(), timers[i]->GetMinTime());

				mprint(msg);
			}
		}

		clientAvg = frameAvg - sumAvg;
		if (clientAvg > 0.0)
		{
			msg.FmtStr(" %-25s %#12.4f %#12.2f%%\n", "Other", clientAvg, clientAvg / frameAvg * 100.0);
			mprint(msg);
		}
	}

	mprintf("+----------------------------------------------------------------------------+\n");

	if (was_active)
	{
		m_AverageFrameTimer.Start();

		for (i = 0; i < m_aClientInfo.Size(); ++i)
			m_aClientInfo[i].timer.Start();
	}
}
#endif

HRESULT cLoopDispatch::SendMessage(eLoopMessage message, tLoopMessageData hData, int flags)
{
	if (message & m_msgs)
	{
		HRESULT result = S_OK;

#ifndef SHIP
		BOOL fLoopTrack = (m_fDiagnostics & kLoopDiagTracking) && (m_diagnosticSet & message);
		uint fLoopTime = m_fDiagnostics & kLoopDiagTimings;
		uint fFrameHeapchk = m_fDiagnostics & kLoopDiagFrameHeapchk;
		uint fClientHeapchk = m_fDiagnostics & kLoopDiagClientHeapchk;
		BOOL fPassedSkipTime;
		BOOL fLoopTimeThisMsg;

		if (message & (kMsgResumeMode | kMsgStart))
			m_TotalModeTime.Start();

		if (m_TotalModeTime.IsActive())
		{
			m_TotalModeTime.Stop();
			fPassedSkipTime = m_TotalModeTime.GetResult() > 2500.0;
			m_TotalModeTime.Start();
			fLoopTimeThisMsg = fPassedSkipTime && fLoopTime && (message & kMsgsFrame);
		}
		else
			fLoopTimeThisMsg = FALSE;

		if (fLoopTimeThisMsg)
			m_AverageFrameTimer.Start();
#endif

		cPriIntArray<ILoopClient>& targetList = m_DispatchLists[MessageToIndex(message)];

		int iLast = static_cast<int>(targetList.Size()) - 1;
		for (int i = 0; i <= iLast; ++i)
		{
			int idx = (flags & kDispatchForward) ? i : iLast - i;
			ILoopClient* pClient = targetList[idx]->pInterface;
#ifndef SHIP
			const char* pszClient = targetList[idx]->nameStr;

			if (fLoopTrack)
				LoopTrack(message, pszClient);

			cAverageTimer* pTimer;
			if (fLoopTimeThisMsg)
			{
				pTimer = &reinterpret_cast<sClientInfo*>(targetList[idx]->pData)->timer;
				pTimer->Start();
			}

#endif

			eLoopMessageResult clientResult;
#ifndef SHIP
			if (message & m_ProfileSet)
				clientResult = LoopProfileSend(pClient, message, hData);
			else
#endif
				clientResult = pClient->ReceiveMessage(message, hData);

			BOOL fContinue = clientResult != kLoopDispatchHalt;

#ifndef SHIP
			if (fLoopTimeThisMsg)
				pTimer->Stop();

			if (fClientHeapchk)
				_heapchk();
#endif

			if (!fContinue)
			{
				result = E_FAIL;
				break;
			}
		}

#ifndef SHIP
		if (fFrameHeapchk)
			_heapchk();

		if (message & (kMsgSuspendMode | kMsgEnd))
			m_TotalModeTime.Stop();

		if (fLoopTime)
		{
			if (fLoopTimeThisMsg)
				m_AverageFrameTimer.Stop();

			if (fPassedSkipTime && (message & kMsgEndFrame))
			{
				m_AverageFrameTimer.Mark();
				for (i = 0; i < m_aClientInfo.Size(); ++i)
					m_aClientInfo[i].timer.Mark();
			}

			if (message & (kMsgSuspendMode | kMsgEnd))
			{
				if (m_AverageFrameTimer.IsActive())
				{
					m_AverageFrameTimer.Stop();
					for (i = 0; i < m_aClientInfo.Size(); ++i)
						m_aClientInfo[i].timer.Stop();
				}

				DumpTimerInfo();
				ClearTimers();
			}
		}

		if (fLoopTrack)
			LoopTrackClear();
#endif

		return result;
	}
	else
	{
		if (message && !(message & m_msgs))
			Warning(("Message 0x%02X not supported by this dispatch chain\n", message));

		return S_FALSE;
	}
}

#ifndef SHIP
static char LoopTrackClear()
{
	mono_setwin(1u);
	mono_clear();
	return mono_setwin(2u);
}

static char LoopTrack(uint message, const char* pszClient)
{
	static int fSplitMono = 0;
	if (!fSplitMono)
	{
		mono_split(1, 2);
		mono_setwin(2);
		fSplitMono = 1;
	}

	int oldx, oldy;
	mono_getxy(&oldx, &oldy);

	mono_setwin(1);

	mono_setxy(0, 0);
	mprintf("%20s", LoopGetMessageName(message));

	mono_setxy(21, 0);
	mprintf("--> %-20s          ", pszClient);

	mono_setwin(2);

	return mono_setxy(oldx, oldy);
}
#endif

static unsigned MessageToIndex(eLoopMessage message)
{
	unsigned currentMessage = 1;
	unsigned i = 0;

	while (currentMessage && !(currentMessage & message))
	{
		++i;
		currentMessage <<= 1;
	}

	return i;
}
HRESULT cLoopDispatch::SendSimpleMessage(eLoopMessage message)
{
	return SendMessage(message, NULL, 1);
}

HRESULT cLoopDispatch::PostMessage(eLoopMessage message, tLoopMessageData hData, int flags)
{
	sLoopQueueMessage queueMessage;
	queueMessage.message = message;
	queueMessage.hData = hData;
	queueMessage.flags = flags;

	m_Queue.Append(queueMessage);

	return S_OK;
}

HRESULT cLoopDispatch::PostSimpleMessage(eLoopMessage message)
{
	sLoopQueueMessage queueMessage;
	queueMessage.message = message;
	queueMessage.hData = NULL;
	queueMessage.flags = 0;

	m_Queue.Append(queueMessage);

	return S_OK;
}

HRESULT cLoopDispatch::ProcessQueue()
{
	sLoopQueueMessage message;
	HRESULT result = S_OK;

	while (m_Queue.GetMessage(&message))
	{
		result = SendMessage(message.message, message.hData, message.flags);
		if (result != S_OK)
			break;
	}

	return result;
}

const sLoopModeName* cLoopDispatch::Describe(sLoopModeInitParmList* list)
{
	const sLoopModeName* name = m_pLoopMode->GetName();
	if (list)
		*list = m_Parms.List();

	return name;
}

#ifndef SHIP
void cLoopDispatch::SetDiagnostics(unsigned fDiagnostics, tLoopMessageSet messages)
{
	m_fDiagnostics = fDiagnostics;
	m_diagnosticSet = messages;
}

void cLoopDispatch::GetDiagnostics(unsigned* pfDiagnostics, tLoopMessageSet* pMessages)
{
	*pfDiagnostics = m_fDiagnostics;
	*pMessages = m_diagnosticSet;
}

void cLoopDispatch::SetProfile(tLoopMessageSet messages, tLoopClientID* pClientId)
{
	m_ProfileSet = messages;
	m_pProfileClientId = pClientId;
}

void cLoopDispatch::GetProfile(tLoopMessageSet* pMessages, tLoopClientID** ppClientId)
{
	*pMessages = m_ProfileSet;
	*ppClientId = m_pProfileClientId;
}
#endif

bool cLoopDispatch::DispatchNormalFrame(ILoopClient* pClient, tLoopMessageData hData)
{
	return pClient->ReceiveMessage(kMsgNormalFrame, hData) != kLoopDispatchHalt;
}

inline void cLoopQueue::Append(const sLoopQueueMessage& message)
{
	if (m_nInsertPoint < kQueueSize)
	{
		m_Messages[m_nInsertPoint] = message;
		++m_nInsertPoint;
	}
	else
	{
		PackAppend(message);
	}
}

inline BOOL cLoopQueue::GetMessage(sLoopQueueMessage* pMessage)
{
	if (m_nRemovePoint == m_nInsertPoint)
		return FALSE;

	*pMessage = m_Messages[m_nRemovePoint];
	++m_nRemovePoint;

	return TRUE;
}
